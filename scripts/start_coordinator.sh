#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${ROOT_DIR}"

CONFIG_FILE="${ROOT_DIR}/config/intersections.json"
if [ ! -f "${CONFIG_FILE}" ]; then
    echo "[WARN] ${CONFIG_FILE} not found; using default cluster topology."
fi

BIN=""
if [ -x "${ROOT_DIR}/build/bin/coordinator" ]; then
    BIN="${ROOT_DIR}/build/bin/coordinator"
elif [ -x "${ROOT_DIR}/build/bin/aegis_coordinator" ]; then
    BIN="${ROOT_DIR}/build/bin/aegis_coordinator"
else
    echo "[ERROR] Coordinator binary not found. Build the project first."
    exit 1
fi

echo "[INFO] Starting SMART AEGIS Coordinator..."
exec "${BIN}" "$@"
