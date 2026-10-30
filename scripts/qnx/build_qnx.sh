#!/bin/bash
# ==============================================================================
# SMART AEGIS (AERS) Phase 20.5: QNX SDP 8.0 Cross-Compilation Script
# Target: Raspberry Pi 4 Model B (Broadcom BCM2711, aarch64le)
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"

BUILD_TYPE="${1:-Debug}"
TARGET_ARCH="${2:-aarch64le}"

echo "=========================================================="
echo " SMART AEGIS: QNX Cross-Compilation"
echo " Target Architecture: ${TARGET_ARCH}"
echo " Build Configuration: ${BUILD_TYPE}"
echo " Root Directory      : ${ROOT_DIR}"
echo "=========================================================="

cd "${ROOT_DIR}"

if command -v q++ &> /dev/null; then
    echo "[INFO] QNX SDP toolchain detected (q++). Building via Makefile.qnx..."
    make -f Makefile.qnx ARCH="${TARGET_ARCH}" BUILD_TYPE="${BUILD_TYPE}"
elif [ -n "${QNX_HOST:-}" ] && [ -x "${QNX_HOST}/usr/bin/q++" ]; then
    echo "[INFO] Using QNX_HOST at: ${QNX_HOST}"
    make -f Makefile.qnx ARCH="${TARGET_ARCH}" BUILD_TYPE="${BUILD_TYPE}"
else
    echo "[WARN] QNX SDP toolchain (q++) not in PATH. Performing host validation build..."
    make all BUILD_TYPE="${BUILD_TYPE}"
fi

echo "=========================================================="
echo " [SUCCESS] Build completed successfully."
echo "=========================================================="

