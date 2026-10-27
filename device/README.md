# 设备模块

`device/` 用于存放面向具体硬件设备的模块,例如 LED、按键、传感器和执行器。

- `led/`:PC13 LED 的设备接口与 FreeRTOS 闪烁任务。
- `debug_uart/`:USART1(PA9/PA10)调试控制台,DMA 环形发送、阻塞读接收、`printf` 重定向。

通用外设封装位于 `driver/`,设备模块应通过 `driver/` 控制硬件,避免直接重复实现 HAL 底层操作。
