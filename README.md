# STM32F103C8T6 HAL + Makefile 起步工程(WSL2 / VS Code)

本工程是一个不依赖 CubeMX 的 STM32F103C8T6 起步模板,使用 ST 官方 HAL/CMSIS 库、GNU Arm Embedded Toolchain、GNU Make 和 OpenOCD。

## 内置内容

- 基于 HSI 的 64 MHz 时钟配置(无需外部晶振)。
- **FreeRTOS(V11)双任务示例**:LED 闪烁任务 + 心跳计数任务,演示抢占与共享状态。
- 手写的 Makefile,生成 ELF、HEX 和 BIN 产物。
- 面向 64 KB Flash / 20 KB RAM 的 GNU 链接脚本。
- VS Code 构建任务、C/C++ IntelliSense 及 Cortex-Debug/OpenOCD 调试配置。
- 依赖脚本:仅从 ST 官方 STM32CubeF1 仓库拉取所需的 CMSIS 设备和 HAL 子模块,从 FreeRTOS 官方仓库拉取内核。
- GitHub Actions CI:PR 及合并到 main 时自动构建固件并上传产物(见 `.github/workflows/build.yml`)。

## 1. 在 WSL Ubuntu 中安装工具

```bash
sudo apt update
sudo apt install -y build-essential gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi gdb-multiarch openocd git
```

在 Windows 上安装 VS Code 和 Microsoft 的 WSL 扩展。请从 WSL 终端打开本工程,不要从 Windows 挂载目录打开:

```bash
cd ~/stm32-hal-makefile
code .
```

## 2. 获取 ST 依赖

在工程根目录执行:

```bash
make deps
```

该脚本会克隆官方 `STMicroelectronics/STM32CubeF1` 仓库,并仅初始化以下子模块:

- `Drivers/CMSIS/Device/ST/STM32F1xx`
- `Drivers/STM32F1xx_HAL_Driver`

CMSIS 核心头文件由父仓库直接提供。此步骤需要能够访问 GitHub。ST 源码不随本工程分发,而是从官方仓库拉取,以保持工程体积小巧并保留上游的许可与版本信息。

官方源地址:https://github.com/STMicroelectronics/STM32CubeF1

`make deps` 还会调用 `scripts/get_freertos.sh`,浅克隆官方 FreeRTOS 内核仓库(`FreeRTOS/FreeRTOS-Kernel`)到 `external/FreeRTOS-Kernel`。两个目录均已被 `.gitignore` 忽略。

## 3. 构建

```bash
make -j8
```

预期输出文件:

- `build/stm32f103c8.elf`
- `build/stm32f103c8.hex`
- `build/stm32f103c8.bin`
- `build/stm32f103c8.map`

其他常用命令:

```bash
make clean       # 清除构建产物
make size        # 构建并打印各段大小
make help        # 显示所有可用目标
```

## 4. 在 WSL 中通过 ST-Link 烧录

按照你的适配器要求连接 ST-Link 的 SWDIO、SWCLK、GND 和目标电压参考。如果 WSL2 内看不到 ST-Link USB 设备,请使用微软的 `usbipd-win` 工具将其透传进来。

然后执行:

```bash
make flash
```

该命令会调用 OpenOCD(`interface/stlink.cfg` + `target/stm32f1x.cfg`),烧写 ELF、校验、复位并退出。如果愿意,也可以在 WSL 中构建,然后用 Windows 版 STM32CubeProgrammer 烧写 HEX 文件。

手动启动 OpenOCD 服务器以便调试:

```bash
make debug-server
```

然后在 VS Code 的"运行和调试"面板中选择 `STM32F103C8T6 - OpenOCD`。需要在 WSL 的 VS Code 环境中安装 `Cortex-Debug` 扩展。

## 硬件假设

- 目标 MCU:STM32F103C8T6,Cortex-M3,中密度器件(`STM32F103xB`)。
- 链接内存:64 KB Flash 位于 `0x08000000`,20 KB SRAM 位于 `0x20000000`。部分第三方芯片/开发板可能不同,修改前请先确认具体型号。
- 系统时钟:HSI 8 MHz / 2 * 16 = 64 MHz。这样可以不依赖外部 HSE 晶振。HSI 精度低于晶振,若对 UART 波特率时序或时钟精度有严格要求,请改为匹配开发板 HSE 的时钟配置。
- LED:PC13,大多数 Blue Pill 开发板为低电平点亮。其他开发板请修改 `example/led/led.c` 中的 LED 引脚和有效电平。

## 工程结构

```text
.
├── Core/Inc/main.h
├── Core/Inc/stm32f1xx_hal_conf.h
├── Core/Inc/FreeRTOSConfig.h
├── Core/Src/main.c
├── BSP/README.md
├── App/README.md
├── example/                       # 外设与功能例程
│   ├── README.md
│   └── led/
│       ├── README.md
│       ├── led.c
│       └── led.h
├── linker/STM32F103C8Tx_FLASH.ld
├── scripts/get_dependencies.sh
├── scripts/get_freertos.sh
├── .github/workflows/build.yml   # CI:PR / push 自动构建
├── .vscode/
│   ├── c_cpp_properties.json
│   ├── launch.json
│   ├── settings.json
│   └── tasks.json
├── Makefile
└── external/README.md
```

## FreeRTOS

内核版本由 `external/FreeRTOS-Kernel` 浅克隆决定(当前为 V11.x),`GCC/ARM_CM3` 移植层直接适用 Cortex-M3。

**关键配置**(`Core/Inc/FreeRTOSConfig.h`):

- `configCPU_CLOCK_HZ = 64 MHz`、`configTICK_RATE_HZ = 1 kHz`
- `configTOTAL_HEAP_SIZE = 10 KB`(20 KB RAM,堆 + 两个任务栈约 12 KB)
- `configCHECK_FOR_STACK_OVERFLOW = 2`、`configUSE_MALLOC_FAILED_HOOK = 1`:栈溢出/分配失败时进入 `Error_Handler`
- 中断优先级:`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5`,HAL 的 SysTick 优先级(`TICK_INT_PRIORITY = 15`)在安全区内,可在 tick 钩子中调用 HAL API
- `vPortSVCHandler` / `xPortPendSVHandler` / `xPortSysTickHandler` 通过宏映射到启动文件使用的 CMSIS 名字(`SVC_Handler` / `PendSV_Handler` / `SysTick_Handler`),无需在 `main.c` 重复定义;HAL 时基改由 `vApplicationTickHook()` 中的 `HAL_IncTick()` 维护

**应用结构**(`Core/Src/main.c` 与 `example/led/led.c`):

- `led_task`(优先级 1,位于 `example/led/led.c`):每 500 ms 翻转 PC13
- `heartbeat_task`(优先级 2):每秒自增 `heartbeat_count`(演示优先级抢占与共享 volatile 变量)
- `main()`:HAL 初始化 → 时钟配置 → `LED_Init()` / `LED_Start()` → 创建心跳任务 → `vTaskStartScheduler()`

**新增外设时**:在 `FreeRTOSConfig.h` 中按需开启相关宏(如 `configUSE_TIMERS` 已启用),任务间同步用 `queue`/`semaphore`/`mutex`(内核源文件已在 Makefile 的 `FREERTOS_SOURCES` 中,无需改动)。

## 持续集成(CI)

推送 PR(目标为 main)或合并到 main 时,GitHub Actions 会自动:

1. 安装 ARM 工具链;
2. 获取 ST 依赖(带缓存);
3. 执行 `make` 构建固件;
4. 上传 `stm32f103c8.elf/.hex/.bin` 作为构建产物,可在 Actions 页面直接下载。

## 添加 HAL 外设

本起步工程只编译时钟、GPIO、Flash、电源和 Cortex-M 支持所需的 HAL 模块。添加外设时,需要将其对应的 `stm32f1xx_hal_<peripheral>.c` 加入 Makefile 的 `HAL_SOURCES`,并在 `Core/Inc/stm32f1xx_hal_conf.h` 中启用相应的模块宏。例如使用 UART 需要加入 HAL UART 源文件并定义 `HAL_UART_MODULE_ENABLED`。

本工程不需要任何 CubeMX 生成的文件。请将板级配置和应用逻辑放在你自己的文件中,保持工程完全由 Makefile 驱动。
