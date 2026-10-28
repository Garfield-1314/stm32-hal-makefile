#ifndef DRIVER_I2C_H
#define DRIVER_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

typedef struct
{
    I2C_TypeDef *instance;
    uint32_t clock_speed_hz;
} Driver_I2C_Config;

typedef struct
{
    I2C_HandleTypeDef handle;
} Driver_I2C;

/* Configure a 7-bit master using the caller-provided SCL/SDA GPIO pins. */
HAL_StatusTypeDef Driver_I2C_Init(Driver_I2C *i2c, const Driver_I2C_Config *config);

HAL_StatusTypeDef Driver_I2C_Write(Driver_I2C *i2c,
                                   uint16_t device_address,
                                   const uint8_t *data,
                                   uint16_t size,
                                   uint32_t timeout_ms);

HAL_StatusTypeDef Driver_I2C_Read(Driver_I2C *i2c,
                                  uint16_t device_address,
                                  uint8_t *data,
                                  uint16_t size,
                                  uint32_t timeout_ms);

HAL_StatusTypeDef Driver_I2C_WriteMem(Driver_I2C *i2c,
                                      uint16_t device_address,
                                      uint16_t memory_address,
                                      uint16_t memory_address_size,
                                      const uint8_t *data,
                                      uint16_t size,
                                      uint32_t timeout_ms);

HAL_StatusTypeDef Driver_I2C_ReadMem(Driver_I2C *i2c,
                                     uint16_t device_address,
                                     uint16_t memory_address,
                                     uint16_t memory_address_size,
                                     uint8_t *data,
                                     uint16_t size,
                                     uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_I2C_H */
