#ifndef DRIVER_UART_H
#define DRIVER_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

typedef struct
{
    USART_TypeDef *instance;
    uint32_t baud_rate;
    uint32_t word_length;
    uint32_t stop_bits;
    uint32_t parity;
    uint32_t mode;
    uint32_t hardware_flow_control;
} Driver_UART_Config;

typedef struct Driver_UART Driver_UART;

/* DMA/IRQ completion callbacks. They are invoked from ISR context and must
 * stay short. Register them by assigning the fields after Driver_UART_Init. */
typedef void (*Driver_UART_TxDoneCallback)(Driver_UART *uart);
typedef void (*Driver_UART_RxEventCallback)(Driver_UART *uart, uint16_t size);
typedef void (*Driver_UART_ErrorCallback)(Driver_UART *uart);

struct Driver_UART
{
    UART_HandleTypeDef handle;
    DMA_HandleTypeDef tx_dma;
    DMA_HandleTypeDef rx_dma;
    Driver_UART_TxDoneCallback tx_done;
    Driver_UART_RxEventCallback rx_event;
    Driver_UART_ErrorCallback error;
};

/* Configure USART1-USART3 using the caller-provided TX/RX GPIO pins. */
HAL_StatusTypeDef Driver_UART_Init(Driver_UART *uart, const Driver_UART_Config *config);

HAL_StatusTypeDef Driver_UART_Write(Driver_UART *uart,
                                    const uint8_t *data,
                                    uint16_t size,
                                    uint32_t timeout_ms);

HAL_StatusTypeDef Driver_UART_Read(Driver_UART *uart,
                                   uint8_t *data,
                                   uint16_t size,
                                   uint32_t timeout_ms);

HAL_StatusTypeDef Driver_UART_WriteByte(Driver_UART *uart,
                                        uint8_t data,
                                        uint32_t timeout_ms);

HAL_StatusTypeDef Driver_UART_ReadByte(Driver_UART *uart,
                                       uint8_t *data,
                                       uint32_t timeout_ms);

/* --- Optional DMA support ----------------------------------------------
 * The caller selects the DMA channels (F103 mapping: USART1_TX = DMA1
 * Channel 4, USART1_RX = DMA1 Channel 5; check RM0008 Table 27 for the
 * other USARTs). Either channel pointer may be NULL for one-way DMA.
 * The RX channel is configured in circular mode so ReceiveToIdle can run
 * continuously; the TX channel runs normal (one chunk per start).
 * The driver enables the DMA1 clock but does not touch NVIC: priority
 * setup and IRQ vector implementations stay with the caller. */

HAL_StatusTypeDef Driver_UART_AttachDma(Driver_UART *uart,
                                        DMA_Channel_TypeDef *tx_channel,
                                        DMA_Channel_TypeDef *rx_channel);

/* Non-blocking: returns HAL_BUSY while a previous TX chunk is in flight. */
HAL_StatusTypeDef Driver_UART_Transmit_DMA(Driver_UART *uart,
                                           const uint8_t *data,
                                           uint16_t size);

/* Continuous reception framed by idle-line detection. Events are reported
 * through the rx_event callback (half/full/idle, circular mode keeps
 * running). Use Driver_UART_DMA_RX_Remaining to locate written data. */
HAL_StatusTypeDef Driver_UART_ReceiveToIdle_DMA(Driver_UART *uart,
                                                uint8_t *data,
                                                uint16_t size);

HAL_StatusTypeDef Driver_UART_AbortDmaReceive(Driver_UART *uart);

/* Raw DMA remaining-count register (NDTR); 0 when RX DMA is not attached. */
uint16_t Driver_UART_DMA_RX_Remaining(const Driver_UART *uart);

/* IRQ forwarding: implement the vector functions in board-level code and
 * call these. On F1, a normal-mode DMA TX reports completion through the
 * USART TC interrupt, so the USART IRQ must be enabled as well. */
void Driver_UART_OnDmaTxIrq(Driver_UART *uart);
void Driver_UART_OnDmaRxIrq(Driver_UART *uart);
void Driver_UART_OnUsartIrq(Driver_UART *uart);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_UART_H */
