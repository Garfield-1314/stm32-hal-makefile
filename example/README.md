# 例程

本目录用于存放 STM32F103C8T6 的外设与功能例程,例如 GPIO、UART、I2C、SPI 和 FreeRTOS 示例。

每个例程使用独立子目录,并附带自己的 `README.md`:

```text
example/
├── led/
│   ├── README.md
│   ├── led.c
│   └── led.h
└── uart_echo/
    ├── README.md
    └── main.c
```

`led` 例程已接入根目录 `Makefile` 的默认构建。新增例程时,请在其 `README.md` 中说明接线、配置和构建方法,并将需要随固件编译的源文件加入 `C_SOURCES`。
