#!/usr/bin/env bash
set -euo pipefail

# ==============================================================================
# SMART AEGIS: Startup Script for Raspberry Pi 1 (Central Coordinator / QNX)
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BIN_PATH="${ROOT_DIR}/build/bin/aegis_coordinator"
CONFIG_PATH="${ROOT_DIR}/config/deployment/rpi1_coordinator.json"

if [ $# -ge 1 ]; then
    CONFIG_PATH="$1"
fi

if [ ! -f "${BIN_PATH}" ]; then
    echo "[ERROR] Coordinator binary not found at: ${BIN_PATH}"
    echo "Please build the project first: make all or ./scripts/build.sh"
    exit 1
fi

if [ ! -f "${CONFIG_PATH}" ]; then
    echo "[ERROR] Configuration file not found at: ${CONFIG_PATH}"
    exit 1
fi

echo "=========================================================="
echo " Starting SMART AEGIS Central Coordinator (Raspberry Pi 1)"
echo " Binary : ${BIN_PATH}"
echo " Config : ${CONFIG_PATH}"
echo "=========================================================="

exec "${BIN_PATH}" --config "${CONFIG_PATH}"
