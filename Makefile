SHELL := /bin/bash
.DEFAULT_GOAL := all

TARGET      := stm32f103c8
BUILD_DIR   := build
CUBE_DIR    := external/STM32CubeF1
HAL_DIR     := $(CUBE_DIR)/Drivers/STM32F1xx_HAL_Driver
CMSIS_DIR   := $(CUBE_DIR)/Drivers/CMSIS
DEVICE_DIR  := $(CMSIS_DIR)/Device/ST/STM32F1xx
FREERTOS_DIR := external/FreeRTOS-Kernel
FREERTOS_PORT_DIR := $(FREERTOS_DIR)/portable/GCC/ARM_CM3
LDSCRIPT    := linker/STM32F103C8Tx_FLASH.ld

CC       := arm-none-eabi-gcc
OBJCOPY  := arm-none-eabi-objcopy
SIZE     := arm-none-eabi-size
OPENOCD  := openocd

CPU       := -mcpu=cortex-m3 -mthumb
C_DEFS    := -DSTM32F103xB -DUSE_HAL_DRIVER
C_INCLUDES := -ICore/Inc \
              -I$(HAL_DIR)/Inc \
              -I$(HAL_DIR)/Inc/Legacy \
              -I$(DEVICE_DIR)/Include \
              -I$(CMSIS_DIR)/Include \
              -I$(FREERTOS_DIR)/include \
              -I$(FREERTOS_PORT_DIR)

OPT ?= -Og
CFLAGS := $(CPU) $(C_DEFS) $(C_INCLUDES) $(OPT) -g3 -std=gnu11 \
          -Wall -Wextra -ffunction-sections -fdata-sections -fno-common -MMD -MP
ASFLAGS := $(CPU) $(C_DEFS) $(C_INCLUDES) -g3 -x assembler-with-cpp
LDFLAGS := $(CPU) -T$(LDSCRIPT) --specs=nano.specs --specs=nosys.specs \
           -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
           -Wl,--print-memory-usage -static \
           -Wl,--start-group -lc -lm -Wl,--end-group

HAL_SOURCES := \
    $(HAL_DIR)/Src/stm32f1xx_hal.c \
    $(HAL_DIR)/Src/stm32f1xx_hal_cortex.c \
    $(HAL_DIR)/Src/stm32f1xx_hal_gpio.c \
    $(HAL_DIR)/Src/stm32f1xx_hal_rcc.c \
    $(HAL_DIR)/Src/stm32f1xx_hal_rcc_ex.c \
    $(HAL_DIR)/Src/stm32f1xx_hal_flash.c \
    $(HAL_DIR)/Src/stm32f1xx_hal_flash_ex.c \
    $(HAL_DIR)/Src/stm32f1xx_hal_pwr.c

FREERTOS_SOURCES := \
    $(FREERTOS_DIR)/tasks.c \
    $(FREERTOS_DIR)/queue.c \
    $(FREERTOS_DIR)/list.c \
    $(FREERTOS_DIR)/event_groups.c \
    $(FREERTOS_DIR)/timers.c \
    $(FREERTOS_DIR)/portable/MemMang/heap_4.c \
    $(FREERTOS_PORT_DIR)/port.c

C_SOURCES := \
    Core/Src/main.c \
    $(DEVICE_DIR)/Source/Templates/system_stm32f1xx.c \
    $(HAL_SOURCES) \
    $(FREERTOS_SOURCES)

ASM_SOURCES := $(DEVICE_DIR)/Source/Templates/gcc/startup_stm32f103xb.s
OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES)) \
           $(patsubst %.s,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
DEPS := $(OBJECTS:.o=.d)
ELF := $(BUILD_DIR)/$(TARGET).elf
HEX := $(BUILD_DIR)/$(TARGET).hex
BIN := $(BUILD_DIR)/$(TARGET).bin

.PHONY: all build _build check-deps deps clean size flash debug-server help

all: build

# Run a dependency check before invoking the sub-make that builds objects.
build: check-deps
	$(MAKE) --no-print-directory _build

_build: $(ELF) $(HEX) $(BIN)

check-deps:
	@test -f "$(HAL_DIR)/Inc/stm32f1xx_hal.h" -a -f "$(DEVICE_DIR)/Include/stm32f1xx.h" -a -f "$(FREERTOS_DIR)/include/FreeRTOS.h" || { \
		echo "Dependencies are missing. Run: make deps"; exit 1; }
	@command -v $(CC) >/dev/null 2>&1 || { echo "Missing $(CC). Install gcc-arm-none-eabi."; exit 1; }

$(ELF): $(OBJECTS) $(LDSCRIPT)
	@mkdir -p $(dir $@)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

-include $(DEPS)

deps:
	./scripts/get_dependencies.sh
	./scripts/get_freertos.sh

clean:
	rm -rf $(BUILD_DIR)

size: build
	$(SIZE) $(ELF)

flash: build
	$(OPENOCD) -f interface/stlink.cfg -f target/stm32f1x.cfg \
		-c "program $(ELF) verify reset exit"

debug-server: check-deps
	$(OPENOCD) -f interface/stlink.cfg -f target/stm32f1x.cfg

help:
	@echo "Targets: deps, build (default), clean, size, flash, debug-server, help"
	@echo "Build: make -j8"
