#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
FREERTOS_DIR="${PROJECT_DIR}/external/FreeRTOS-Kernel"
REPO_URL="https://github.com/FreeRTOS/FreeRTOS-Kernel.git"

mkdir -p "${PROJECT_DIR}/external"

if [[ ! -d "${FREERTOS_DIR}/.git" ]]; then
    if [[ -e "${FREERTOS_DIR}" ]]; then
        echo "Removing incomplete FreeRTOS directory: ${FREERTOS_DIR}"
        rm -rf "${FREERTOS_DIR}"
    fi
    echo "Cloning official FreeRTOS-Kernel repository..."
    git clone --depth 1 "${REPO_URL}" "${FREERTOS_DIR}"
fi

test -f "${FREERTOS_DIR}/tasks.c"
test -f "${FREERTOS_DIR}/include/FreeRTOS.h"
test -f "${FREERTOS_DIR}/portable/GCC/ARM_CM3/port.c"
echo "FreeRTOS kernel is ready."
