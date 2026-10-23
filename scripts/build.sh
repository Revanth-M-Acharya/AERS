#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_TYPE="${1:-Debug}"

cd "${ROOT_DIR}"

echo "=========================================================="
echo " SMART AEGIS: Build Automation Runner [${BUILD_TYPE}]"
echo "=========================================================="

if command -v cmake >/dev/null 2>&1; then
    echo "[BUILD ENGINE] CMake detected. Building via CMake..."
    mkdir -p "${ROOT_DIR}/build"
    cd "${ROOT_DIR}/build"
    cmake -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" ..
    cmake --build . --parallel "$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"
    if [ "${2:-}" = "test" ]; then
        echo "[TEST ENGINE] Executing CTest..."
        ctest --output-on-failure
    fi
else
    echo "[BUILD ENGINE] CMake not found in PATH. Building via native Makefile..."
    make BUILD_TYPE="${BUILD_TYPE}" all
    if [ "${2:-}" = "test" ] || [ "${1:-}" = "test" ]; then
        make test
    fi
fi

echo "=========================================================="
echo " Build completed successfully."
echo "=========================================================="
