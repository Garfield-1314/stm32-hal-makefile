#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "stm32f1xx.h"

/*-----------------------------------------------------------
 * STM32F103C8T6 @ 64 MHz (HSI + PLL), 20 KB RAM.
 *----------------------------------------------------------*/

#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      ( 64000000UL )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                    5
#define configMINIMAL_STACK_SIZE                ( ( uint16_t ) 128 )
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 10 * 1024 ) )
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_QUEUE_SETS                    0
#define configUSE_TIMERS                        1
#define configTIMER_TASK_STACK_DEPTH            ( configMINIMAL_STACK_SIZE * 2 )
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 )

/* Hooks: keep minimal, enable failure detection only.
 * The tick hook drives the HAL timebase because SysTick_Handler is
 * mapped directly to xPortSysTickHandler (see below). */
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     1

/* Co-routines / trace / stats: off. */
#define configUSE_CO_ROUTINES                   0
#define configUSE_TRACE_FACILITY                0
#define configUSE_STATS_FORMATTING_FUNCTIONS    0
#define configGENERATE_RUN_TIME_STATS           0

/* Software timer / event groups / stream buffers. */
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0
#define configUSE_APPLICATION_TASK_TAG          0

/* Optional API inclusion. */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskCleanUpResources           0
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          0
#define INCLUDE_eTaskGetState                   0
#define INCLUDE_xTimerPendFunctionCall          0
#define INCLUDE_xTaskAbortDelay                 0
#define INCLUDE_xTaskGetHandle                  0
#define INCLUDE_xTaskResumeFromISR              0

/* Assert hook: disable interrupts and spin for debugger inspection.
 * Required to enable the kernel's internal sanity checks (handler
 * installation, interrupt priority validation, heap integrity) and also
 * resolves the unused pxVectorTable warning in the CM3 port. */
void vAssertCalled(const char *file, int line);
#define configASSERT( x )   if( ( x ) == 0 ) vAssertCalled( __FILE__, __LINE__ )

/* Cortex-M3 NVIC configuration.
 * HAL's SysTick priority (TICK_INT_PRIORITY = 15) must be at or below
 * configMAX_SYSCALL_INTERRUPT_PRIORITY so FreeRTOS API is safe to call
 * from the tick ISR. */
#define configPRIO_BITS                         4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5

#define configKERNEL_INTERRUPT_PRIORITY         ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

/* Map FreeRTOS interrupt handlers to the CMSIS names used by the
 * STM32F103 startup file (weak symbols get overridden in main.c). */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */
