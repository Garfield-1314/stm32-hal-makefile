#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

/* LED task: toggles the PC13 LED every 500 ms. */
static void led_task(void *argument)
{
    (void)argument;
    for (;;)
    {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        vTaskDelay(pdMS_TO_TICKS(500U));
    }
}

/* Heartbeat task: higher priority, increments a counter every second.
 * Demonstrates preemption of led_task and shared (volatile) state. */
static volatile uint32_t heartbeat_count;

static void heartbeat_task(void *argument)
{
    (void)argument;
    for (;;)
    {
        heartbeat_count++;
        vTaskDelay(pdMS_TO_TICKS(1000U));
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = LED_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_Port, &gpio);

    /* Many Blue Pill boards wire the PC13 LED active-low. */
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

    xTaskCreate(led_task, "led", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(heartbeat_task, "beat", configMINIMAL_STACK_SIZE, NULL, 2, NULL);

    vTaskStartScheduler();

    /* Should never reach here: the scheduler only returns on error. */
    Error_Handler();
    return 0;
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    /* HSI (8 MHz) / 2 * 16 = 64 MHz; no external crystal required. */
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
    osc.PLL.PLLMUL = RCC_PLL_MUL16;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK)
    {
        Error_Handler();
    }

    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }

    /* ADC clock is configured separately on F1: PCLK2 / 6 (64 MHz / 6 ~ 10.67 MHz). */
    __HAL_RCC_ADC_CONFIG(RCC_ADCPCLK2_DIV6);
}

/* SysTick_Handler is mapped to xPortSysTickHandler via FreeRTOSConfig.h,
 * so the HAL timebase is maintained from the FreeRTOS tick hook instead. */
void vApplicationTickHook(void)
{
    HAL_IncTick();
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    Error_Handler();
}

void vApplicationMallocFailedHook(void)
{
    Error_Handler();
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        /* Stay here for debugger inspection. */
    }
}
