#include "debug_uart.h"
#include "main.h"
#include "driver_io.h"
#include "driver_uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Board mapping: Blue Pill USART1 on PA9 (TX) / PA10 (RX).
 * DMA1 Channel 4 = USART1_TX, DMA1 Channel 5 = USART1_RX (RM0008 Table 27).
 * Both channels are taken, so SPI2/I2C2 DMA cannot be used concurrently. */
#define DEBUG_UART_INSTANCE       USART1
#define DEBUG_UART_BAUD_RATE      115200U
#define DEBUG_UART_TX_PORT        GPIOA
#define DEBUG_UART_TX_PIN         GPIO_PIN_9
#define DEBUG_UART_RX_PORT        GPIOA
#define DEBUG_UART_RX_PIN         GPIO_PIN_10
#define DEBUG_UART_DMA_TX_CHANNEL DMA1_Channel4
#define DEBUG_UART_DMA_RX_CHANNEL DMA1_Channel5

/* Must be numerically >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5):
 * FreeRTOS FromISR APIs are only safe from interrupts at or below (numerically
 * >=) that level. 6 gives the debug console lower urgency than the kernel. */
#define DEBUG_UART_IRQ_PRIORITY   6U

/* Sizes are powers of two so index masking is a single AND. */
#define DEBUG_TX_RING_SIZE        512U
#define DEBUG_TX_RING_MASK        (DEBUG_TX_RING_SIZE - 1U)
#define DEBUG_RX_RING_SIZE        256U
#define DEBUG_RX_RING_MASK        (DEBUG_RX_RING_SIZE - 1U)
#define DEBUG_PRINTF_BUF_SIZE     128U
#define DEBUG_LINE_BUF_SIZE       (2U * DEBUG_PRINTF_BUF_SIZE)

static Driver_UART debug_uart;

/* --- TX ring --------------------------------------------------------------
 * Producers: task context (DebugUart_Write/Printf/_write). Consumer: DMA,
 * drained chunk by chunk. head/tail are free-running counters; the live
 * chunk is only released (tail advanced) from the DMA completion ISR, so a
 * chunk being transmitted is never overwritten. */
static uint8_t tx_ring[DEBUG_TX_RING_SIZE];
static volatile uint32_t tx_head;
static volatile uint32_t tx_tail;
static volatile uint8_t tx_busy;
static volatile uint16_t tx_chunk_len;
static volatile uint32_t tx_dropped;

/* --- RX ring --------------------------------------------------------------
 * DMA writes circularly; rx_head (ISR) tracks the DMA write offset via the
 * NDTR register, rx_tail is the reader cursor (task context). */
static uint8_t rx_ring[DEBUG_RX_RING_SIZE];
static volatile uint32_t rx_head;
static volatile uint16_t rx_last_offset;
static uint32_t rx_tail;

static SemaphoreHandle_t tx_mutex;
static SemaphoreHandle_t rx_sem;

/* Formatting scratch buffers; only touched with tx_mutex held, so a single
 * static copy serves all tasks. */
static char printf_buf[DEBUG_PRINTF_BUF_SIZE];
static char line_buf[DEBUG_LINE_BUF_SIZE];

/* Start the next TX chunk if the wire is idle and the ring holds data.
 * Safe from both task and ISR context: the critical section only raises
 * BASEPRI, which neither context can be corrupted by. */
static void tx_pump(void)
{
    UBaseType_t saved = taskENTER_CRITICAL_FROM_ISR();

    if (tx_busy == 0U)
    {
        uint32_t used = tx_head - tx_tail;
        if (used > 0U)
        {
            uint32_t offset = (uint32_t)tx_tail & DEBUG_TX_RING_MASK;
            uint32_t chunk = DEBUG_TX_RING_SIZE - offset; /* contiguous run */
            if (chunk > used)
            {
                chunk = used;
            }

            tx_chunk_len = (uint16_t)chunk;
            tx_busy = 1U;
            if (Driver_UART_Transmit_DMA(&debug_uart,
                                         &tx_ring[offset],
                                         (uint16_t)chunk) != HAL_OK)
            {
                tx_busy = 0U;
            }
        }
    }

    taskEXIT_CRITICAL_FROM_ISR(saved);
}

static void on_tx_done(Driver_UART *uart)
{
    (void)uart;
    tx_tail += tx_chunk_len;
    tx_busy = 0U;
    tx_pump(); /* keep the wire busy while data is queued */
}

static void on_rx_event(Driver_UART *uart, uint16_t size)
{
    (void)size;

    /* Half/full/idle event: resynchronize from NDTR so no bookkeeping is
     * lost regardless of which event type fired. */
    uint16_t offset = (uint16_t)(DEBUG_RX_RING_SIZE - Driver_UART_DMA_RX_Remaining(uart));
    uint32_t delta = (uint32_t)((uint16_t)(offset - rx_last_offset));
    rx_head += delta;
    rx_last_offset = offset;

    BaseType_t higher_priority_woken = pdFALSE;
    (void)xSemaphoreGiveFromISR(rx_sem, &higher_priority_woken);
    portYIELD_FROM_ISR(higher_priority_woken);
}

static void on_uart_error(Driver_UART *uart)
{
    /* The HAL treats UART errors during DMA reception as blocking: it has
     * already aborted RX (RxState back to READY) before this callback. The
     * circular buffer restarts at offset 0; drop the reader cursor to the
     * current head so no stale bytes are reported. */
    uint16_t offset = (uint16_t)(DEBUG_RX_RING_SIZE - Driver_UART_DMA_RX_Remaining(uart));
    uint32_t delta = (uint32_t)((uint16_t)(offset - rx_last_offset));
    rx_head += delta;
    rx_last_offset = 0U;
    rx_tail = rx_head;

    if (Driver_UART_ReceiveToIdle_DMA(uart, rx_ring, DEBUG_RX_RING_SIZE) != HAL_OK)
    {
        Error_Handler();
    }

    BaseType_t higher_priority_woken = pdFALSE;
    (void)xSemaphoreGiveFromISR(rx_sem, &higher_priority_woken);
    portYIELD_FROM_ISR(higher_priority_woken);
}

/* Enqueue into the TX ring; drops the whole message when it does not fit.
 * Task context only. */
static int tx_ring_write(const uint8_t *data, uint32_t size)
{
    if (size == 0U)
    {
        return 0;
    }

    taskENTER_CRITICAL();
    uint32_t used = tx_head - tx_tail;
    uint32_t free_space = DEBUG_TX_RING_SIZE - used;
    if (size > free_space)
    {
        tx_dropped += size;
        taskEXIT_CRITICAL();
        return 0;
    }

    uint32_t offset = tx_head & DEBUG_TX_RING_MASK;
    uint32_t first = DEBUG_TX_RING_SIZE - offset;
    if (first > size)
    {
        first = size;
    }
    memcpy(&tx_ring[offset], data, first);
    if (size > first)
    {
        memcpy(&tx_ring[0], &data[first], size - first);
    }
    tx_head += size;
    taskEXIT_CRITICAL();

    tx_pump();
    return (int)size;
}

void DebugUart_Init(void)
{
    Driver_IO tx_io;
    Driver_IO rx_io;

    if ((Driver_IO_Init(&tx_io, DEBUG_UART_TX_PORT, DEBUG_UART_TX_PIN,
                        DRIVER_IO_MODE_AF_PP, GPIO_NOPULL) != HAL_OK) ||
        (Driver_IO_Init(&rx_io, DEBUG_UART_RX_PORT, DEBUG_UART_RX_PIN,
                        DRIVER_IO_MODE_INPUT, GPIO_NOPULL) != HAL_OK))
    {
        Error_Handler();
    }

    const Driver_UART_Config config = {
        .instance = DEBUG_UART_INSTANCE,
        .baud_rate = DEBUG_UART_BAUD_RATE,
        .word_length = UART_WORDLENGTH_8B,
        .stop_bits = UART_STOPBITS_1,
        .parity = UART_PARITY_NONE,
        .mode = UART_MODE_TX_RX,
        .hardware_flow_control = UART_HWCONTROL_NONE,
    };
    if (Driver_UART_Init(&debug_uart, &config) != HAL_OK)
    {
        Error_Handler();
    }

    debug_uart.tx_done = on_tx_done;
    debug_uart.rx_event = on_rx_event;
    debug_uart.error = on_uart_error;

    if (Driver_UART_AttachDma(&debug_uart,
                              DEBUG_UART_DMA_TX_CHANNEL,
                              DEBUG_UART_DMA_RX_CHANNEL) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, DEBUG_UART_IRQ_PRIORITY, 0U);
    HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
    HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, DEBUG_UART_IRQ_PRIORITY, 0U);
    HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
    HAL_NVIC_SetPriority(USART1_IRQn, DEBUG_UART_IRQ_PRIORITY, 0U);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    tx_mutex = xSemaphoreCreateMutex();
    rx_sem = xSemaphoreCreateBinary();
    if ((tx_mutex == NULL) || (rx_sem == NULL))
    {
        Error_Handler();
    }

    if (Driver_UART_ReceiveToIdle_DMA(&debug_uart, rx_ring, DEBUG_RX_RING_SIZE) != HAL_OK)
    {
        Error_Handler();
    }
}

int DebugUart_Write(const void *data, uint16_t size)
{
    if (data == NULL)
    {
        return 0;
    }

    return tx_ring_write((const uint8_t *)data, size);
}

int DebugUart_Printf(const char *fmt, ...)
{
    if (fmt == NULL)
    {
        return 0;
    }

    if (xSemaphoreTake(tx_mutex, portMAX_DELAY) != pdTRUE)
    {
        return 0;
    }

    va_list args;
    va_start(args, fmt);
    int formatted = vsnprintf(printf_buf, sizeof(printf_buf), fmt, args);
    va_end(args);

    /* Expand '\n' to "\r\n" so terminals render lines correctly. */
    uint32_t line_len = 0U;
    int limit = (formatted < (int)sizeof(printf_buf)) ? formatted : (int)(sizeof(printf_buf) - 1U);
    for (int i = 0; i < limit; i++)
    {
        if (printf_buf[i] == '\n')
        {
            line_buf[line_len++] = '\r';
        }
        line_buf[line_len++] = printf_buf[i];
    }

    (void)tx_ring_write((const uint8_t *)line_buf, line_len);
    (void)xSemaphoreGive(tx_mutex);
    return formatted;
}

uint16_t DebugUart_Read(uint8_t *data, uint16_t size, uint32_t timeout_ms)
{
    if ((data == NULL) || (size == 0U))
    {
        return 0U;
    }

    TickType_t start = xTaskGetTickCount();
    TickType_t timeout_ticks = (timeout_ms == UINT32_MAX)
                                   ? portMAX_DELAY
                                   : pdMS_TO_TICKS(timeout_ms);

    for (;;)
    {
        uint16_t copied = 0U;

        taskENTER_CRITICAL();
        uint32_t head = rx_head;
        uint32_t available = head - rx_tail;
        if (available > DEBUG_RX_RING_SIZE)
        {
            /* Reader fell a full ring behind: oldest bytes were overwritten. */
            rx_tail = head - DEBUG_RX_RING_SIZE;
            available = DEBUG_RX_RING_SIZE;
        }
        if (available > 0U)
        {
            uint32_t offset = rx_tail & DEBUG_RX_RING_MASK;
            uint32_t first = DEBUG_RX_RING_SIZE - offset;
            if (first > available)
            {
                first = available;
            }
            if (first > size)
            {
                first = size;
            }
            memcpy(data, &rx_ring[offset], first);
            rx_tail += first;
            copied = (uint16_t)first;
        }
        taskEXIT_CRITICAL();

        if (copied > 0U)
        {
            return copied;
        }

        TickType_t waited = xTaskGetTickCount() - start;
        if ((timeout_ticks != portMAX_DELAY) && (waited >= timeout_ticks))
        {
            return 0U;
        }
        TickType_t remaining = (timeout_ticks == portMAX_DELAY)
                                   ? portMAX_DELAY
                                   : (timeout_ticks - waited);
        (void)xSemaphoreTake(rx_sem, remaining);
    }
}

uint16_t DebugUart_Available(void)
{
    uint16_t available;

    taskENTER_CRITICAL();
    uint32_t bytes = rx_head - rx_tail;
    if (bytes > DEBUG_RX_RING_SIZE)
    {
        bytes = DEBUG_RX_RING_SIZE;
    }
    available = (uint16_t)bytes;
    taskEXIT_CRITICAL();

    return available;
}

uint32_t DebugUart_TxDropped(void)
{
    return tx_dropped;
}

/* printf retargeting: --specs=nosys.specs provides a weak _write stub, the
 * strong definition here wins over the library archive. Output goes through
 * the same non-blocking ring. Note newlib's printf itself is not reentrant:
 * concurrent printf() calls from several tasks may interleave at the format
 * stage; use DebugUart_Printf for atomic lines. */
int _write(int fd, char *ptr, int len)
{
    (void)fd;
    if ((ptr == NULL) || (len <= 0))
    {
        return 0;
    }

    return tx_ring_write((const uint8_t *)ptr, (uint32_t)len);
}

/* --- IRQ vectors ---------------------------------------------------------- */
void DMA1_Channel4_IRQHandler(void)
{
    Driver_UART_OnDmaTxIrq(&debug_uart);
}

void DMA1_Channel5_IRQHandler(void)
{
    Driver_UART_OnDmaRxIrq(&debug_uart);
}

void USART1_IRQHandler(void)
{
    Driver_UART_OnUsartIrq(&debug_uart);
}
