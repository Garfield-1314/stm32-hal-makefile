#include "driver_io.h"

static uint8_t Driver_IO_IsValid(const Driver_IO *io)
{
    if ((io == NULL) || (io->port == NULL) || (io->pin == 0U))
    {
        return 0U;
    }

    /* The public IO API controls one pin at a time. */
    return ((uint16_t)(io->pin & (uint16_t)(io->pin - 1U)) == 0U) ? 1U : 0U;
}

static HAL_StatusTypeDef Driver_IO_EnableClock(GPIO_TypeDef *port)
{
    if (port == GPIOA)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    }
    else if (port == GPIOB)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    }
    else if (port == GPIOC)
    {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    }
    else
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef Driver_IO_Init(Driver_IO *io,
                                 GPIO_TypeDef *port,
                                 uint16_t pin,
                                 Driver_IO_Mode mode,
                                 uint32_t pull)
{
    if ((io == NULL) || (port == NULL) || (pin == 0U) ||
        ((uint16_t)(pin & (uint16_t)(pin - 1U)) != 0U) ||
        ((pull != GPIO_NOPULL) && (pull != GPIO_PULLUP) && (pull != GPIO_PULLDOWN)))
    {
        return HAL_ERROR;
    }

    GPIO_InitTypeDef gpio = {0};

    switch (mode)
    {
    case DRIVER_IO_MODE_INPUT:
        gpio.Mode = GPIO_MODE_INPUT;
        break;
    case DRIVER_IO_MODE_OUTPUT_PP:
        gpio.Mode = GPIO_MODE_OUTPUT_PP;
        break;
    case DRIVER_IO_MODE_OUTPUT_OD:
        gpio.Mode = GPIO_MODE_OUTPUT_OD;
        break;
    default:
        return HAL_ERROR;
    }

    if (Driver_IO_EnableClock(port) != HAL_OK)
    {
        return HAL_ERROR;
    }

    gpio.Pin = pin;
    gpio.Pull = pull;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(port, &gpio);

    io->port = port;
    io->pin = pin;
    return HAL_OK;
}

HAL_StatusTypeDef Driver_IO_Write(const Driver_IO *io, GPIO_PinState state)
{
    if ((Driver_IO_IsValid(io) == 0U) ||
        ((state != GPIO_PIN_RESET) && (state != GPIO_PIN_SET)))
    {
        return HAL_ERROR;
    }

    HAL_GPIO_WritePin(io->port, io->pin, state);
    return HAL_OK;
}

HAL_StatusTypeDef Driver_IO_Read(const Driver_IO *io, GPIO_PinState *state)
{
    if ((Driver_IO_IsValid(io) == 0U) || (state == NULL))
    {
        return HAL_ERROR;
    }

    *state = HAL_GPIO_ReadPin(io->port, io->pin);
    return HAL_OK;
}

HAL_StatusTypeDef Driver_IO_Toggle(const Driver_IO *io)
{
    if (Driver_IO_IsValid(io) == 0U)
    {
        return HAL_ERROR;
    }

    HAL_GPIO_TogglePin(io->port, io->pin);
    return HAL_OK;
}
