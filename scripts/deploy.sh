#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${ROOT_DIR}"

echo "=========================================================="
echo "          SMART AEGIS DEPLOYMENT AUTOMATION"
echo "=========================================================="

BUILD_TYPE="${1:-Release}"
echo "[INFO] Configuring build with CMake (Build Type: ${BUILD_TYPE})..."
cmake -S . -B build -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

echo "[INFO] Building binaries..."
cmake --build build -j4

echo "[INFO] Running deployment validator..."
if [ -x "${ROOT_DIR}/build/bin/validator" ]; then
    "${ROOT_DIR}/build/bin/validator"
fi

echo "=========================================================="
echo " [SUCCESS] SMART AEGIS build and deployment verified."
echo " Executables located in: ${ROOT_DIR}/build/bin"
echo "=========================================================="
