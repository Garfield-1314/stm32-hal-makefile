#ifndef DEVICE_DEBUG_UART_H
#define DEVICE_DEBUG_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Board debug console: USART1 on PA9 (TX, AF_PP) / PA10 (RX, floating),
 * 115200 8N1. TX is a non-blocking ring drained by DMA1 Channel 4; RX is
 * DMA1 Channel 5 in circular mode framed by idle-line detection, exposed
 * as a blocking read.
 *
 * IRQ priorities are fixed at 6 (numerically below
 * configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY=5 is NOT allowed for FreeRTOS
 * FromISR calls; 6 is safely above it), see debug_uart.c. */

void DebugUart_Init(void);

/* Enqueue a formatted line and return immediately; '\n' is expanded to
 * "\r\n". Thread-safe: the whole line is atomic. If the ring is full the
 * message is dropped as a whole and counted (see DebugUart_TxDropped).
 * Returns the number of bytes enqueued (before CRLF expansion). */
int DebugUart_Printf(const char *fmt, ...);

/* Enqueue raw bytes and return immediately. Returns the number of bytes
 * accepted (0 if the message did not fit into the free ring space). */
int DebugUart_Write(const void *data, uint16_t size);

/* Block until at least one byte is available (or timeout_ms elapses, pass
 * UINT32_MAX to wait forever), then copy up to size bytes. Returns the
 * number of bytes copied (0 on timeout). */
uint16_t DebugUart_Read(uint8_t *data, uint16_t size, uint32_t timeout_ms);

/* Bytes currently buffered and readable without blocking. */
uint16_t DebugUart_Available(void);

/* Bytes dropped since boot because the TX ring was full. */
uint32_t DebugUart_TxDropped(void);

#ifdef __cplusplus
}
#endif

#endif /* DEVICE_DEBUG_UART_H */
