#include "driver_uart.h"

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
