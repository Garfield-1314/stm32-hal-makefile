# LED 设备模块

本模块通过 `driver/inc/driver_io.h` 控制 PC13 LED,并创建一个 FreeRTOS 任务,每 500 ms 翻转一次 LED。

## 硬件

- LED:PC13
- 有效电平:低电平点亮(常见 Blue Pill 开发板)

如开发板接线不同,请修改 `led.c` 中的 `LED_GPIO_Port`、`LED_Pin` 以及初始化时的输出电平。

## 接口

- `LED_Init()`:通过 `Driver_IO_Init()` 配置 PC13 为推挽输出,并关闭 LED。
- `LED_Start()`:创建 LED 闪烁任务。

根目录 `Makefile` 已编译本模块,当前入口在 `Core/Src/main.c` 中调用这两个接口。
