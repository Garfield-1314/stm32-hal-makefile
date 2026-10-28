# 基础驱动

本目录提供 STM32F103 HAL 的基础封装,供 `device/` 中的设备模块复用。

| 模块 | 头文件 | 功能 |
| --- | --- | --- |
| IO | `driver_io.h` | 单引脚输入/输出初始化、读写、翻转 |
| PWM | `driver_pwm.h` | TIM1-TIM4 PWM 初始化、启停、千分比占空比设置 |
| I2C | `driver_i2c.h` | 7 位地址主机模式、普通收发、寄存器读写 |
| SPI | `driver_spi.h` | 8 位全双工主机模式、收发和同步全双工传输 |
| UART | `driver_uart.h` | USART1-USART3 初始化、阻塞收发、单字节收发,可选 DMA 附加(非阻塞发送、空闲定界接收) |

驱动会开启对应外设时钟,但不会配置引脚复用、GPIO 重映射或 I2C/SPI 片选。调用方需要在初始化前完成板级 GPIO 配置。

## IO

```c
Driver_IO led;

Driver_IO_Init(&led, GPIOC, GPIO_PIN_13,
               DRIVER_IO_MODE_OUTPUT_PP, GPIO_NOPULL);
Driver_IO_Write(&led, GPIO_PIN_SET);
Driver_IO_Toggle(&led);
```

模式可选 `DRIVER_IO_MODE_INPUT`、`DRIVER_IO_MODE_OUTPUT_PP`、`DRIVER_IO_MODE_OUTPUT_OD` 和 `DRIVER_IO_MODE_AF_PP`(复用推挽,如 UART TX)。F1 的 GPIO 复用配置不绑定具体外设,引脚通道选择仍由 AFIO 决定。

## PWM

以下配置在 64 MHz 定时器时钟下产生 1 kHz PWM:

```c
Driver_PWM pwm;
const Driver_PWM_Config config = {
    .instance = TIM2,
    .channel = TIM_CHANNEL_1,
    .prescaler = 63U,
    .period = 999U,
    .initial_pulse = 0U,
};

Driver_PWM_Init(&pwm, &config);
Driver_PWM_SetDutyPermille(&pwm, 250U); /* 25% */
```

还需将对应通道引脚配置为复用推挽输出。

## I2C

I2C1 默认引脚为 PB6(SCL)和 PB7(SDA),应配置为复用开漏模式并启用上拉。`device_address` 使用包含读写位的 8 位 HAL 地址,例如 7 位地址 `0x3C` 传入 `0x3C << 1`。

```c
Driver_I2C i2c;
const Driver_I2C_Config config = {
    .instance = I2C1,
    .clock_speed_hz = 100000U,
};
uint8_t value;

Driver_I2C_Init(&i2c, &config);
Driver_I2C_ReadMem(&i2c, 0x3CU << 1, 0x0FU,
                   I2C_MEMADD_SIZE_8BIT, &value, 1U, 10U);
```

## SPI

SPI1 默认使用 PA5(SCK)、PA6(MISO)、PA7(MOSI)。驱动使用软件 NSS,片选引脚应通过 `Driver_IO` 单独控制。`baud_rate_prescaler` 可填 `SPI_BAUDRATEPRESCALER_2` 到 `SPI_BAUDRATEPRESCALER_256`。

```c
Driver_SPI spi;
const Driver_SPI_Config config = {
    .instance = SPI1,
    .baud_rate_prescaler = SPI_BAUDRATEPRESCALER_8,
    .clock_polarity = SPI_POLARITY_LOW,
    .clock_phase = SPI_PHASE_1EDGE,
};
uint8_t tx[] = {0x9FU, 0x00U};
uint8_t rx[sizeof(tx)];

Driver_SPI_Init(&spi, &config);
Driver_SPI_TransmitReceive(&spi, tx, rx, sizeof(tx), 10U);
```

## UART

USART1 默认使用 PA9(TX)和 PA10(RX)。TX 应配置为复用推挽输出,RX 配置为浮空输入或上拉输入。以下示例使用 115200 8N1:

```c
Driver_UART uart;
const Driver_UART_Config config = {
    .instance = USART1,
    .baud_rate = 115200U,
    .word_length = UART_WORDLENGTH_8B,
    .stop_bits = UART_STOPBITS_1,
    .parity = UART_PARITY_NONE,
    .mode = UART_MODE_TX_RX,
    .hardware_flow_control = UART_HWCONTROL_NONE,
};
const uint8_t message[] = "hello\r\n";
uint8_t received;

Driver_UART_Init(&uart, &config);
Driver_UART_Write(&uart, message, sizeof(message) - 1U, 100U);
Driver_UART_ReadByte(&uart, &received, 100U);
```

所有传输接口的最后一个参数都是超时时间,单位为毫秒。在 FreeRTOS 任务中建议使用有限超时,避免总线异常时无限等待。

I2C、SPI 和 UART 驱动对象不是线程安全的。多个任务共享同一外设时,应由调用方使用互斥量串行化访问。

### UART DMA(可选)

`Driver_UART_AttachDma` 为 UART 附加 DMA 通道(通道由调用方按 RM0008 表 27 选择,可只附一个方向)。驱动使能 DMA1 时钟并配置通道,但不设置 NVIC 优先级、不实现中断向量——这些留在板级代码。

```c
Driver_UART uart;
/* ... Driver_UART_Init(&uart, &config); ... */

/* USART1: TX = DMA1 通道 4,RX = DMA1 通道 5 */
if (Driver_UART_AttachDma(&uart, DMA1_Channel4, DMA1_Channel5) != HAL_OK) { /* ... */ }

/* 完成事件回调(ISR 上下文,由驱动内的 HAL 弱回调分发) */
uart.tx_done = on_tx_done;          /* 一块发送完成 */
uart.rx_event = on_rx_event;        /* 半满/全满/空闲事件 */
uart.error = on_uart_error;         /* 接收错误(HAL 已中止 DMA) */

/* 中断向量由板级实现,内部转发给 HAL */
void DMA1_Channel4_IRQHandler(void) { Driver_UART_OnDmaTxIrq(&uart); }
void DMA1_Channel5_IRQHandler(void) { Driver_UART_OnDmaRxIrq(&uart); }
void USART1_IRQHandler(void)        { Driver_UART_OnUsartIrq(&uart); }

/* 非阻塞发送一块(上一块未完成时返回 HAL_BUSY) */
Driver_UART_Transmit_DMA(&uart, data, size);

/* 循环模式 + 空闲线定界接收;在 rx_event 中用
 * Driver_UART_DMA_RX_Remaining(&uart) 计算已写入字节数 */
Driver_UART_ReceiveToIdle_DMA(&uart, buf, sizeof(buf));
```

注意:

- F1 上 Normal 模式的 DMA 发送,其完成回调要经过 **USART 的 TC 中断**触发,因此 `USART1_IRQn` 必须使能,否则 `tx_done` 永远不会执行;
- 接收错误(噪声/过载)时 HAL 会中止 DMA 并调用 `error` 回调,需要在回调中重新调用 `Driver_UART_ReceiveToIdle_DMA` 挂回;
- 若中断里要调用 FreeRTOS `FromISR` API,NVIC 优先级数值必须 ≥ `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`(本工程为 5)。

F103 DMA1 通道与外设固定映射,一个通道同一时间只能服务一个外设,选用时注意冲突。下表为与串行外设相关的节选(完整表见 RM0008 Table 27;不同容量型号的 USART3_RX 等个别条目有差异):

| DMA1 通道 | 外设请求(节选) |
| --- | --- |
| 1 | ADC1、USART3_RX(中容量) |
| 2 | SPI1_RX、USART3_TX、TIM1_CH1 |
| 3 | SPI1_TX、USART3_RX(高容量)、TIM1_CH2 |
| 4 | **USART1_TX**、SPI2_RX、I2C2_RX、TIM1_CH4 |
| 5 | **USART1_RX**、SPI2_TX、I2C2_TX、TIM1_UP |
| 6 | USART2_RX、I2C1_RX、TIM1_CH3 |
| 7 | USART2_TX、I2C1_TX |

本工程的调试串口(`device/debug_uart/`)占用通道 4/5,因此 SPI2/I2C2 的 DMA 不可再用(反之亦然);需要同时使用时应选择不冲突的组合,或为其中之一改用阻塞/中断模式。
