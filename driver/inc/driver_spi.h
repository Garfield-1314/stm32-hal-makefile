#ifndef DRIVER_SPI_H
#define DRIVER_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

typedef struct
{
    SPI_TypeDef *instance;
    uint32_t baud_rate_prescaler;
    uint32_t clock_polarity;
    uint32_t clock_phase;
} Driver_SPI_Config;

typedef struct
{
    SPI_HandleTypeDef handle;
} Driver_SPI;

/* Configure an 8-bit, MSB-first, full-duplex SPI master with software NSS. */
HAL_StatusTypeDef Driver_SPI_Init(Driver_SPI *spi, const Driver_SPI_Config *config);

HAL_StatusTypeDef Driver_SPI_Transmit(Driver_SPI *spi,
                                      const uint8_t *data,
                                      uint16_t size,
                                      uint32_t timeout_ms);

HAL_StatusTypeDef Driver_SPI_Receive(Driver_SPI *spi,
                                     uint8_t *data,
                                     uint16_t size,
                                     uint32_t timeout_ms);

HAL_StatusTypeDef Driver_SPI_TransmitReceive(Driver_SPI *spi,
                                             const uint8_t *tx_data,
                                             uint8_t *rx_data,
                                             uint16_t size,
                                             uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_SPI_H */
