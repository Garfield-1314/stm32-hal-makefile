#include "driver_spi.h"

static HAL_StatusTypeDef Driver_SPI_EnableClock(SPI_TypeDef *instance)
{
    if (instance == SPI1)
    {
        __HAL_RCC_SPI1_CLK_ENABLE();
    }
    else if (instance == SPI2)
    {
        __HAL_RCC_SPI2_CLK_ENABLE();
    }
    else
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

static uint8_t Driver_SPI_IsPrescalerValid(uint32_t prescaler)
{
    switch (prescaler)
    {
    case SPI_BAUDRATEPRESCALER_2:
    case SPI_BAUDRATEPRESCALER_4:
    case SPI_BAUDRATEPRESCALER_8:
    case SPI_BAUDRATEPRESCALER_16:
    case SPI_BAUDRATEPRESCALER_32:
    case SPI_BAUDRATEPRESCALER_64:
    case SPI_BAUDRATEPRESCALER_128:
    case SPI_BAUDRATEPRESCALER_256:
        return 1U;
    default:
        return 0U;
    }
}

static uint8_t Driver_SPI_IsValid(const Driver_SPI *spi)
{
    return ((spi != NULL) && (spi->handle.Instance != NULL)) ? 1U : 0U;
}

static uint8_t Driver_SPI_IsTransferValid(const Driver_SPI *spi,
                                          const void *data,
                                          uint16_t size)
{
    return ((Driver_SPI_IsValid(spi) != 0U) && (data != NULL) && (size > 0U)) ? 1U : 0U;
}

HAL_StatusTypeDef Driver_SPI_Init(Driver_SPI *spi, const Driver_SPI_Config *config)
{
    if ((spi == NULL) || (config == NULL) || (config->instance == NULL) ||
        (Driver_SPI_IsPrescalerValid(config->baud_rate_prescaler) == 0U) ||
        ((config->clock_polarity != SPI_POLARITY_LOW) &&
         (config->clock_polarity != SPI_POLARITY_HIGH)) ||
        ((config->clock_phase != SPI_PHASE_1EDGE) &&
         (config->clock_phase != SPI_PHASE_2EDGE)))
    {
        return HAL_ERROR;
    }

    if (Driver_SPI_EnableClock(config->instance) != HAL_OK)
    {
        return HAL_ERROR;
    }

    spi->handle = (SPI_HandleTypeDef){0};
    spi->handle.Instance = config->instance;
    spi->handle.Init.Mode = SPI_MODE_MASTER;
    spi->handle.Init.Direction = SPI_DIRECTION_2LINES;
    spi->handle.Init.DataSize = SPI_DATASIZE_8BIT;
    spi->handle.Init.CLKPolarity = config->clock_polarity;
    spi->handle.Init.CLKPhase = config->clock_phase;
    spi->handle.Init.NSS = SPI_NSS_SOFT;
    spi->handle.Init.BaudRatePrescaler = config->baud_rate_prescaler;
    spi->handle.Init.FirstBit = SPI_FIRSTBIT_MSB;
    spi->handle.Init.TIMode = SPI_TIMODE_DISABLE;
    spi->handle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    spi->handle.Init.CRCPolynomial = 7U;

    if (HAL_SPI_Init(&spi->handle) != HAL_OK)
    {
        spi->handle.Instance = NULL;
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef Driver_SPI_Transmit(Driver_SPI *spi,
                                      const uint8_t *data,
                                      uint16_t size,
                                      uint32_t timeout_ms)
{
    if (Driver_SPI_IsTransferValid(spi, data, size) == 0U)
    {
        return HAL_ERROR;
    }

    return HAL_SPI_Transmit(&spi->handle, data, size, timeout_ms);
}

HAL_StatusTypeDef Driver_SPI_Receive(Driver_SPI *spi,
                                     uint8_t *data,
                                     uint16_t size,
                                     uint32_t timeout_ms)
{
    if (Driver_SPI_IsTransferValid(spi, data, size) == 0U)
    {
        return HAL_ERROR;
    }

    return HAL_SPI_Receive(&spi->handle, data, size, timeout_ms);
}

HAL_StatusTypeDef Driver_SPI_TransmitReceive(Driver_SPI *spi,
                                             const uint8_t *tx_data,
                                             uint8_t *rx_data,
                                             uint16_t size,
                                             uint32_t timeout_ms)
{
    if ((Driver_SPI_IsTransferValid(spi, tx_data, size) == 0U) ||
        (Driver_SPI_IsTransferValid(spi, rx_data, size) == 0U))
    {
        return HAL_ERROR;
    }

    return HAL_SPI_TransmitReceive(&spi->handle, tx_data, rx_data, size, timeout_ms);
}
