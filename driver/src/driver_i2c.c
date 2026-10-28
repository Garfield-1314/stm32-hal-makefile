#include "driver_i2c.h"

#define DRIVER_I2C_MAX_SPEED_HZ 400000U

static HAL_StatusTypeDef Driver_I2C_EnableClock(I2C_TypeDef *instance)
{
    if (instance == I2C1)
    {
        __HAL_RCC_I2C1_CLK_ENABLE();
    }
    else if (instance == I2C2)
    {
        __HAL_RCC_I2C2_CLK_ENABLE();
    }
    else
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

static uint8_t Driver_I2C_IsValid(const Driver_I2C *i2c)
{
    return ((i2c != NULL) && (i2c->handle.Instance != NULL)) ? 1U : 0U;
}

static uint8_t Driver_I2C_IsTransferValid(const Driver_I2C *i2c,
                                          const uint8_t *data,
                                          uint16_t size)
{
    return ((Driver_I2C_IsValid(i2c) != 0U) && (data != NULL) && (size > 0U)) ? 1U : 0U;
}

HAL_StatusTypeDef Driver_I2C_Init(Driver_I2C *i2c, const Driver_I2C_Config *config)
{
    if ((i2c == NULL) || (config == NULL) || (config->instance == NULL) ||
        (config->clock_speed_hz == 0U) ||
        (config->clock_speed_hz > DRIVER_I2C_MAX_SPEED_HZ))
    {
        return HAL_ERROR;
    }

    if (Driver_I2C_EnableClock(config->instance) != HAL_OK)
    {
        return HAL_ERROR;
    }

    i2c->handle = (I2C_HandleTypeDef){0};
    i2c->handle.Instance = config->instance;
    i2c->handle.Init.ClockSpeed = config->clock_speed_hz;
    i2c->handle.Init.DutyCycle = I2C_DUTYCYCLE_2;
    i2c->handle.Init.OwnAddress1 = 0U;
    i2c->handle.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    i2c->handle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    i2c->handle.Init.OwnAddress2 = 0U;
    i2c->handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    i2c->handle.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&i2c->handle) != HAL_OK)
    {
        i2c->handle.Instance = NULL;
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef Driver_I2C_Write(Driver_I2C *i2c,
                                   uint16_t device_address,
                                   const uint8_t *data,
                                   uint16_t size,
                                   uint32_t timeout_ms)
{
    if (Driver_I2C_IsTransferValid(i2c, data, size) == 0U)
    {
        return HAL_ERROR;
    }

    return HAL_I2C_Master_Transmit(&i2c->handle, device_address,
                                   (uint8_t *)data, size, timeout_ms);
}

HAL_StatusTypeDef Driver_I2C_Read(Driver_I2C *i2c,
                                  uint16_t device_address,
                                  uint8_t *data,
                                  uint16_t size,
                                  uint32_t timeout_ms)
{
    if (Driver_I2C_IsTransferValid(i2c, data, size) == 0U)
    {
        return HAL_ERROR;
    }

    return HAL_I2C_Master_Receive(&i2c->handle, device_address, data, size, timeout_ms);
}

HAL_StatusTypeDef Driver_I2C_WriteMem(Driver_I2C *i2c,
                                      uint16_t device_address,
                                      uint16_t memory_address,
                                      uint16_t memory_address_size,
                                      const uint8_t *data,
                                      uint16_t size,
                                      uint32_t timeout_ms)
{
    if ((Driver_I2C_IsTransferValid(i2c, data, size) == 0U) ||
        ((memory_address_size != I2C_MEMADD_SIZE_8BIT) &&
         (memory_address_size != I2C_MEMADD_SIZE_16BIT)))
    {
        return HAL_ERROR;
    }

    return HAL_I2C_Mem_Write(&i2c->handle, device_address, memory_address,
                             memory_address_size, (uint8_t *)data, size, timeout_ms);
}

HAL_StatusTypeDef Driver_I2C_ReadMem(Driver_I2C *i2c,
                                     uint16_t device_address,
                                     uint16_t memory_address,
                                     uint16_t memory_address_size,
                                     uint8_t *data,
                                     uint16_t size,
                                     uint32_t timeout_ms)
{
    if ((Driver_I2C_IsTransferValid(i2c, data, size) == 0U) ||
        ((memory_address_size != I2C_MEMADD_SIZE_8BIT) &&
         (memory_address_size != I2C_MEMADD_SIZE_16BIT)))
    {
        return HAL_ERROR;
    }

    return HAL_I2C_Mem_Read(&i2c->handle, device_address, memory_address,
                            memory_address_size, data, size, timeout_ms);
}
