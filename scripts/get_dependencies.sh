#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
CUBE_DIR="${PROJECT_DIR}/external/STM32CubeF1"
REPO_URL="https://github.com/STMicroelectronics/STM32CubeF1.git"

mkdir -p "${PROJECT_DIR}/external"

if [[ ! -d "${CUBE_DIR}/.git" ]]; then
    if [[ -e "${CUBE_DIR}" ]]; then
        echo "Removing incomplete dependency directory: ${CUBE_DIR}"
        rm -rf "${CUBE_DIR}"
    fi
    echo "Cloning official STM32CubeF1 repository..."
    git clone --depth 1 "${REPO_URL}" "${CUBE_DIR}"
fi

echo "Fetching only the required CMSIS device and HAL submodules..."
git -C "${CUBE_DIR}" submodule update --init --depth 1 -- \
    Drivers/CMSIS/Device/ST/STM32F1xx \
    Drivers/STM32F1xx_HAL_Driver

test -f "${CUBE_DIR}/Drivers/CMSIS/Include/core_cm3.h"
test -f "${CUBE_DIR}/Drivers/CMSIS/Device/ST/STM32F1xx/Include/stm32f1xx.h"
test -f "${CUBE_DIR}/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal.h"
echo "Dependencies are ready. Run: make -j8"
