#include "main.h"
#include "led.h"
#include "FreeRTOS.h"
#include "task.h"

/* Many Blue Pill boards connect the user LED to PC13, active-low. */
#define LED_GPIO_Port GPIOC
#define LED_Pin       GPIO_PIN_13

/* Toggle the LED every 500 ms. */
static void led_task(void *argument)
{
    (void)argument;
    for (;;)
    {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        vTaskDelay(pdMS_TO_TICKS(500U));
    }
}

void LED_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = LED_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_Port, &gpio);

    /* Active-low: start with the LED off. */
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
}

void LED_Start(void)
{
    if (xTaskCreate(led_task, "led", configMINIMAL_STACK_SIZE, NULL, 1, NULL) != pdPASS)
    {
        Error_Handler();
    }
}
