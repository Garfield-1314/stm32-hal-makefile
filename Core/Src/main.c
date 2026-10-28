#include "main.h"
#include "led.h"
#include "debug_uart.h"
#include "FreeRTOS.h"
#include "task.h"

/* Heartbeat task: higher priority, increments a counter every second and
 * logs it on the debug UART. Also echoes received bytes so the DMA RX path
 * can be exercised from a serial terminal. */
static volatile uint32_t heartbeat_count;

static void heartbeat_task(void *argument)
{
    (void)argument;
    for (;;)
    {
        heartbeat_count++;
        (void)DebugUart_Printf("heartbeat: %lu\n",
                               (unsigned long)heartbeat_count);

        uint8_t echo[16];
        uint16_t received = DebugUart_Read(echo, sizeof(echo), 100U);
        if (received > 0U)
        {
            (void)DebugUart_Write(echo, received);
        }

        vTaskDelay(pdMS_TO_TICKS(1000U));
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    LED_Init();
    DebugUart_Init();
    LED_Start();

    (void)DebugUart_Printf("debug console up\n");

    /* 256 words (1 KB): newlib-nano vsnprintf needs more than the minimal
     * stack, see device/debug_uart/README.md. */
    xTaskCreate(heartbeat_task, "beat", 256, NULL, 2, NULL);

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

void vAssertCalled(const char *file, int line)
{
    (void)file;
    (void)line;
    __disable_irq();
    for (;;)
    {
        /* Break here in a debugger to inspect file/line. */
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        /* Stay here for debugger inspection. */
    }
}
