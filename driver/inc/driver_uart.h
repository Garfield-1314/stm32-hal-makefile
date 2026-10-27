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

typedef struct
{
    UART_HandleTypeDef handle;
} Driver_UART;

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

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_UART_H */
