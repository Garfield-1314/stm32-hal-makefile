#include "main.h"
#include "led.h"
#include "driver_io.h"
#include "FreeRTOS.h"
#include "task.h"

/* Device mapping: many Blue Pill boards connect the LED to PC13, active-low. */
#define LED_GPIO_Port GPIOC
#define LED_Pin       GPIO_PIN_13

static Driver_IO led;

/* Toggle the LED every 500 ms. */
static void led_task(void *argument)
{
    (void)argument;
    for (;;)
    {
        (void)Driver_IO_Toggle(&led);
        vTaskDelay(pdMS_TO_TICKS(500U));
    }
}

void LED_Init(void)
{
    if (Driver_IO_Init(&led, LED_GPIO_Port, LED_Pin,
                       DRIVER_IO_MODE_OUTPUT_PP, GPIO_NOPULL) != HAL_OK)
    {
        Error_Handler();
    }

    /* Active-low: start with the LED off. */
    (void)Driver_IO_Write(&led, GPIO_PIN_SET);
}

void LED_Start(void)
{
    if (xTaskCreate(led_task, "led", configMINIMAL_STACK_SIZE, NULL, 1, NULL) != pdPASS)
    {
        Error_Handler();
    }
}
