#!/usr/bin/env bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "=========================================================="
echo "          SMART AEGIS SYSTEM HEALTH CHECK"
echo "=========================================================="

VALIDATOR=""
if [ -x "${ROOT_DIR}/build/bin/validator" ]; then
    VALIDATOR="${ROOT_DIR}/build/bin/validator"
elif [ -x "${ROOT_DIR}/build/bin/test_smart_aegis_system" ]; then
    VALIDATOR="${ROOT_DIR}/build/bin/test_smart_aegis_system"
fi

if [ -n "${VALIDATOR}" ]; then
    "${VALIDATOR}"
else
    echo "[WARN] Validator binary not built. Checking process status..."
    pgrep -l -f "coordinator" || echo "Coordinator: OFFLINE"
    pgrep -l -f "intersection" || echo "Intersections: OFFLINE"
fi
