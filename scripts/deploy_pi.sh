#!/usr/bin/env bash
set -euo pipefail

TARGET_IP="${1:-192.168.10.101}"
TARGET_USER="${2:-pi}"
TARGET_PATH="/home/${TARGET_USER}/aegis"

echo "=========================================================="
echo " Deploying SMART AEGIS to Raspberry Pi: ${TARGET_USER}@${TARGET_IP}"
echo "=========================================================="

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${ROOT_DIR}/build"

if [ ! -d "${BUILD_DIR}" ]; then
    echo "Error: Build directory not found. Run ./scripts/build.sh first."
    exit 1
fi

ssh "${TARGET_USER}@${TARGET_IP}" "mkdir -p ${TARGET_PATH}/bin ${TARGET_PATH}/config"
scp -r "${ROOT_DIR}/config" "${TARGET_USER}@${TARGET_IP}:${TARGET_PATH}/"
echo "Deployment successful."
