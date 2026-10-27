#include "driver_uart.h"

#define DRIVER_UART_MAX_INSTANCES 3U

/* The HAL completion callbacks (HAL_UART_TxCpltCallback, ...) are global
 * weak symbols, so instances that opt into DMA register here to be found
 * from the UART handle inside those callbacks. */
static Driver_UART *dma_instances[DRIVER_UART_MAX_INSTANCES];

static void Driver_UART_Register(Driver_UART *uart)
{
    for (uint32_t i = 0U; i < DRIVER_UART_MAX_INSTANCES; i++)
    {
        if (dma_instances[i] == uart)
        {
            return;
        }
    }
    for (uint32_t i = 0U; i < DRIVER_UART_MAX_INSTANCES; i++)
    {
        if (dma_instances[i] == NULL)
        {
            dma_instances[i] = uart;
            return;
        }
    }
}

static Driver_UART *Driver_UART_FindByHandle(const UART_HandleTypeDef *handle)
{
    for (uint32_t i = 0U; i < DRIVER_UART_MAX_INSTANCES; i++)
    {
        if ((dma_instances[i] != NULL) && (&dma_instances[i]->handle == handle))
        {
            return dma_instances[i];
        }
    }
    return NULL;
}

/* --- HAL weak callback dispatch (DMA completion events) --- */

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    Driver_UART *uart = Driver_UART_FindByHandle(huart);
    if ((uart != NULL) && (uart->tx_done != NULL))
    {
        uart->tx_done(uart);
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
    Driver_UART *uart = Driver_UART_FindByHandle(huart);
    if ((uart != NULL) && (uart->rx_event != NULL))
    {
        uart->rx_event(uart, size);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    Driver_UART *uart = Driver_UART_FindByHandle(huart);
    if ((uart != NULL) && (uart->error != NULL))
    {
        uart->error(uart);
    }
}

static HAL_StatusTypeDef Driver_UART_EnableClock(USART_TypeDef *instance)
{
    if (instance == USART1)
    {
        __HAL_RCC_USART1_CLK_ENABLE();
    }
    else if (instance == USART2)
    {
        __HAL_RCC_USART2_CLK_ENABLE();
    }
    else if (instance == USART3)
    {
        __HAL_RCC_USART3_CLK_ENABLE();
    }
    else
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

static uint8_t Driver_UART_IsConfigValid(const Driver_UART_Config *config)
{
    if ((config == NULL) || (config->instance == NULL) || (config->baud_rate == 0U))
    {
        return 0U;
    }

    if ((config->word_length != UART_WORDLENGTH_8B) &&
        (config->word_length != UART_WORDLENGTH_9B))
    {
        return 0U;
    }

    if ((config->stop_bits != UART_STOPBITS_1) &&
        (config->stop_bits != UART_STOPBITS_2))
    {
        return 0U;
    }

    if ((config->parity != UART_PARITY_NONE) &&
        (config->parity != UART_PARITY_EVEN) &&
        (config->parity != UART_PARITY_ODD))
    {
        return 0U;
    }

    if ((config->mode != UART_MODE_TX) &&
        (config->mode != UART_MODE_RX) &&
        (config->mode != UART_MODE_TX_RX))
    {
        return 0U;
    }

    if ((config->hardware_flow_control != UART_HWCONTROL_NONE) &&
        (config->hardware_flow_control != UART_HWCONTROL_RTS) &&
        (config->hardware_flow_control != UART_HWCONTROL_CTS) &&
        (config->hardware_flow_control != UART_HWCONTROL_RTS_CTS))
    {
        return 0U;
    }

    return 1U;
}

static uint8_t Driver_UART_IsValid(const Driver_UART *uart)
{
    return ((uart != NULL) && (uart->handle.Instance != NULL)) ? 1U : 0U;
}

static uint8_t Driver_UART_IsTransferValid(const Driver_UART *uart,
                                           const void *data,
                                           uint16_t size)
{
    return ((Driver_UART_IsValid(uart) != 0U) && (data != NULL) && (size > 0U)) ? 1U : 0U;
}

HAL_StatusTypeDef Driver_UART_Init(Driver_UART *uart, const Driver_UART_Config *config)
{
    if ((uart == NULL) || (Driver_UART_IsConfigValid(config) == 0U))
    {
        return HAL_ERROR;
    }

    if (Driver_UART_EnableClock(config->instance) != HAL_OK)
    {
        return HAL_ERROR;
    }

    uart->handle = (UART_HandleTypeDef){0};
    uart->handle.Instance = config->instance;
    uart->handle.Init.BaudRate = config->baud_rate;
    uart->handle.Init.WordLength = config->word_length;
    uart->handle.Init.StopBits = config->stop_bits;
    uart->handle.Init.Parity = config->parity;
    uart->handle.Init.Mode = config->mode;
    uart->handle.Init.HwFlowCtl = config->hardware_flow_control;
    uart->handle.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&uart->handle) != HAL_OK)
    {
        uart->handle.Instance = NULL;
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef Driver_UART_Write(Driver_UART *uart,
                                    const uint8_t *data,
                                    uint16_t size,
                                    uint32_t timeout_ms)
{
    if (Driver_UART_IsTransferValid(uart, data, size) == 0U)
    {
        return HAL_ERROR;
    }

    return HAL_UART_Transmit(&uart->handle, data, size, timeout_ms);
}

HAL_StatusTypeDef Driver_UART_Read(Driver_UART *uart,
                                   uint8_t *data,
                                   uint16_t size,
                                   uint32_t timeout_ms)
{
    if (Driver_UART_IsTransferValid(uart, data, size) == 0U)
    {
        return HAL_ERROR;
    }

    return HAL_UART_Receive(&uart->handle, data, size, timeout_ms);
}

HAL_StatusTypeDef Driver_UART_WriteByte(Driver_UART *uart,
                                        uint8_t data,
                                        uint32_t timeout_ms)
{
    return Driver_UART_Write(uart, &data, 1U, timeout_ms);
}

HAL_StatusTypeDef Driver_UART_ReadByte(Driver_UART *uart,
                                       uint8_t *data,
                                       uint32_t timeout_ms)
{
    return Driver_UART_Read(uart, data, 1U, timeout_ms);
}

/* --- DMA support --- */

static void Driver_UART_ConfigureTxChannel(DMA_HandleTypeDef *dma)
{
    /* One-shot TX chunk: normal mode, memory increment, byte alignment. */
    dma->Init.Direction = DMA_MEMORY_TO_PERIPH;
    dma->Init.PeriphInc = DMA_PINC_DISABLE;
    dma->Init.MemInc = DMA_MINC_ENABLE;
    dma->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    dma->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    dma->Init.Mode = DMA_NORMAL;
    dma->Init.Priority = DMA_PRIORITY_HIGH;
}

static void Driver_UART_ConfigureRxChannel(DMA_HandleTypeDef *dma)
{
    /* Circular RX so idle-framed reception keeps running across chunks. */
    dma->Init.Direction = DMA_PERIPH_TO_MEMORY;
    dma->Init.PeriphInc = DMA_PINC_DISABLE;
    dma->Init.MemInc = DMA_MINC_ENABLE;
    dma->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    dma->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    dma->Init.Mode = DMA_CIRCULAR;
    dma->Init.Priority = DMA_PRIORITY_VERY_HIGH;
}

HAL_StatusTypeDef Driver_UART_AttachDma(Driver_UART *uart,
                                        DMA_Channel_TypeDef *tx_channel,
                                        DMA_Channel_TypeDef *rx_channel)
{
    if ((Driver_UART_IsValid(uart) == 0U) || ((tx_channel == NULL) && (rx_channel == NULL)))
    {
        return HAL_ERROR;
    }

    __HAL_RCC_DMA1_CLK_ENABLE();

    if (tx_channel != NULL)
    {
        uart->tx_dma = (DMA_HandleTypeDef){0};
        uart->tx_dma.Instance = tx_channel;
        Driver_UART_ConfigureTxChannel(&uart->tx_dma);
        if (HAL_DMA_Init(&uart->tx_dma) != HAL_OK)
        {
            return HAL_ERROR;
        }
        __HAL_LINKDMA(&uart->handle, hdmatx, uart->tx_dma);
    }

    if (rx_channel != NULL)
    {
        uart->rx_dma = (DMA_HandleTypeDef){0};
        uart->rx_dma.Instance = rx_channel;
        Driver_UART_ConfigureRxChannel(&uart->rx_dma);
        if (HAL_DMA_Init(&uart->rx_dma) != HAL_OK)
        {
            return HAL_ERROR;
        }
        __HAL_LINKDMA(&uart->handle, hdmarx, uart->rx_dma);
    }

    Driver_UART_Register(uart);
    return HAL_OK;
}

HAL_StatusTypeDef Driver_UART_Transmit_DMA(Driver_UART *uart,
                                           const uint8_t *data,
                                           uint16_t size)
{
    if ((Driver_UART_IsTransferValid(uart, data, size) == 0U) ||
        (uart->handle.hdmatx == NULL))
    {
        return HAL_ERROR;
    }

    /* HAL_BUSY while the previous chunk is still on the wire. */
    return HAL_UART_Transmit_DMA(&uart->handle, (uint8_t *)data, size);
}

HAL_StatusTypeDef Driver_UART_ReceiveToIdle_DMA(Driver_UART *uart,
                                                uint8_t *data,
                                                uint16_t size)
{
    if ((Driver_UART_IsTransferValid(uart, data, size) == 0U) ||
        (uart->handle.hdmarx == NULL))
    {
        return HAL_ERROR;
    }

    return HAL_UARTEx_ReceiveToIdle_DMA(&uart->handle, data, size);
}

HAL_StatusTypeDef Driver_UART_AbortDmaReceive(Driver_UART *uart)
{
    if (Driver_UART_IsValid(uart) == 0U)
    {
        return HAL_ERROR;
    }

    return HAL_UART_AbortReceive(&uart->handle);
}

uint16_t Driver_UART_DMA_RX_Remaining(const Driver_UART *uart)
{
    if ((Driver_UART_IsValid(uart) == 0U) ||
        (uart->handle.hdmarx == NULL) ||
        (uart->handle.hdmarx->Instance == NULL))
    {
        return 0U;
    }

    return (uint16_t)__HAL_DMA_GET_COUNTER(uart->handle.hdmarx);
}

void Driver_UART_OnDmaTxIrq(Driver_UART *uart)
{
    if ((uart != NULL) && (uart->handle.hdmatx != NULL))
    {
        HAL_DMA_IRQHandler(&uart->tx_dma);
    }
}

void Driver_UART_OnDmaRxIrq(Driver_UART *uart)
{
    if ((uart != NULL) && (uart->handle.hdmarx != NULL))
    {
        HAL_DMA_IRQHandler(&uart->rx_dma);
    }
}

void Driver_UART_OnUsartIrq(Driver_UART *uart)
{
    if (Driver_UART_IsValid(uart) != 0U)
    {
        HAL_UART_IRQHandler(&uart->handle);
    }
}
