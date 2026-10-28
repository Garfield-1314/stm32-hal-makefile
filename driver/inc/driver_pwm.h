#ifndef DRIVER_PWM_H
#define DRIVER_PWM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

typedef struct
{
    TIM_TypeDef *instance;
    uint32_t channel;
    uint32_t prescaler;
    uint32_t period;
    uint32_t initial_pulse;
} Driver_PWM_Config;

typedef struct
{
    TIM_HandleTypeDef handle;
    uint32_t channel;
} Driver_PWM;

HAL_StatusTypeDef Driver_PWM_Init(Driver_PWM *pwm, const Driver_PWM_Config *config);
HAL_StatusTypeDef Driver_PWM_Start(Driver_PWM *pwm);
HAL_StatusTypeDef Driver_PWM_Stop(Driver_PWM *pwm);

/* Set duty from 0 (always low) to 1000 (always high). */
HAL_StatusTypeDef Driver_PWM_SetDutyPermille(Driver_PWM *pwm, uint32_t duty_permille);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_PWM_H */
