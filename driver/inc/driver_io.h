#ifndef DRIVER_IO_H
#define DRIVER_IO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

typedef enum
{
    DRIVER_IO_MODE_INPUT = 0,
    DRIVER_IO_MODE_OUTPUT_PP,
    DRIVER_IO_MODE_OUTPUT_OD
} Driver_IO_Mode;

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
} Driver_IO;

/* pin must contain exactly one GPIO_PIN_x flag. */
HAL_StatusTypeDef Driver_IO_Init(Driver_IO *io,
                                 GPIO_TypeDef *port,
                                 uint16_t pin,
                                 Driver_IO_Mode mode,
                                 uint32_t pull);

HAL_StatusTypeDef Driver_IO_Write(const Driver_IO *io, GPIO_PinState state);
HAL_StatusTypeDef Driver_IO_Read(const Driver_IO *io, GPIO_PinState *state);
HAL_StatusTypeDef Driver_IO_Toggle(const Driver_IO *io);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_IO_H */
