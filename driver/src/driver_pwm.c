#include "driver_pwm.h"

static uint8_t Driver_PWM_IsValidChannel(uint32_t channel)
{
    switch (channel)
    {
    case TIM_CHANNEL_1:
    case TIM_CHANNEL_2:
    case TIM_CHANNEL_3:
    case TIM_CHANNEL_4:
        return 1U;
    default:
        return 0U;
    }
}

static HAL_StatusTypeDef Driver_PWM_EnableClock(TIM_TypeDef *instance)
{
    if (instance == TIM1)
    {
        __HAL_RCC_TIM1_CLK_ENABLE();
    }
    else if (instance == TIM2)
    {
        __HAL_RCC_TIM2_CLK_ENABLE();
    }
    else if (instance == TIM3)
    {
        __HAL_RCC_TIM3_CLK_ENABLE();
    }
    else if (instance == TIM4)
    {
        __HAL_RCC_TIM4_CLK_ENABLE();
    }
    else
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

static uint8_t Driver_PWM_IsValid(const Driver_PWM *pwm)
{
    return ((pwm != NULL) && (pwm->handle.Instance != NULL) &&
            (Driver_PWM_IsValidChannel(pwm->channel) != 0U)) ? 1U : 0U;
}

HAL_StatusTypeDef Driver_PWM_Init(Driver_PWM *pwm, const Driver_PWM_Config *config)
{
    if ((pwm == NULL) || (config == NULL) || (config->instance == NULL) ||
        (config->period == 0U) || (config->period > 0xFFFFU) ||
        (config->prescaler > 0xFFFFU) || (config->initial_pulse > config->period) ||
        (Driver_PWM_IsValidChannel(config->channel) == 0U))
    {
        return HAL_ERROR;
    }

    if (Driver_PWM_EnableClock(config->instance) != HAL_OK)
    {
        return HAL_ERROR;
    }

    pwm->handle = (TIM_HandleTypeDef){0};
    pwm->handle.Instance = config->instance;
    pwm->handle.Init.Prescaler = config->prescaler;
    pwm->handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    pwm->handle.Init.Period = config->period;
    pwm->handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    pwm->handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    pwm->channel = config->channel;

    if (HAL_TIM_PWM_Init(&pwm->handle) != HAL_OK)
    {
        pwm->handle.Instance = NULL;
        return HAL_ERROR;
    }

    TIM_OC_InitTypeDef output = {0};
    output.OCMode = TIM_OCMODE_PWM1;
    output.Pulse = config->initial_pulse;
    output.OCPolarity = TIM_OCPOLARITY_HIGH;
    output.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(&pwm->handle, &output, pwm->channel) != HAL_OK)
    {
        pwm->handle.Instance = NULL;
        return HAL_ERROR;
    }

    return Driver_PWM_Start(pwm);
}

HAL_StatusTypeDef Driver_PWM_Start(Driver_PWM *pwm)
{
    if (Driver_PWM_IsValid(pwm) == 0U)
    {
        return HAL_ERROR;
    }

    return HAL_TIM_PWM_Start(&pwm->handle, pwm->channel);
}

HAL_StatusTypeDef Driver_PWM_Stop(Driver_PWM *pwm)
{
    if (Driver_PWM_IsValid(pwm) == 0U)
    {
        return HAL_ERROR;
    }

    return HAL_TIM_PWM_Stop(&pwm->handle, pwm->channel);
}

HAL_StatusTypeDef Driver_PWM_SetDutyPermille(Driver_PWM *pwm, uint32_t duty_permille)
{
    if ((Driver_PWM_IsValid(pwm) == 0U) || (duty_permille > 1000U))
    {
        return HAL_ERROR;
    }

    uint32_t pulse = ((pwm->handle.Init.Period + 1U) * duty_permille) / 1000U;

    if (pulse > pwm->handle.Init.Period)
    {
        pulse = pwm->handle.Init.Period;
    }

    __HAL_TIM_SET_COMPARE(&pwm->handle, pwm->channel, pulse);
    return HAL_OK;
}
