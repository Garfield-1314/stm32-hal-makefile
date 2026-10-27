# 基础驱动

本目录提供 STM32F103 HAL 的基础封装,供 `device/` 中的设备模块复用。

| 模块 | 头文件 | 功能 |
| --- | --- | --- |
| IO | `driver_io.h` | 单引脚输入/输出初始化、读写、翻转 |
| PWM | `driver_pwm.h` | TIM1-TIM4 PWM 初始化、启停、千分比占空比设置 |
| I2C | `driver_i2c.h` | 7 位地址主机模式、普通收发、寄存器读写 |
| SPI | `driver_spi.h` | 8 位全双工主机模式、收发和同步全双工传输 |
| UART | `driver_uart.h` | USART1-USART3 初始化、块收发和单字节收发 |

驱动会开启对应外设时钟,但不会配置引脚复用、GPIO 重映射或 I2C/SPI 片选。调用方需要在初始化前完成板级 GPIO 配置。

## IO

```c
Driver_IO led;

Driver_IO_Init(&led, GPIOC, GPIO_PIN_13,
               DRIVER_IO_MODE_OUTPUT_PP, GPIO_NOPULL);
Driver_IO_Write(&led, GPIO_PIN_SET);
Driver_IO_Toggle(&led);
```

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
