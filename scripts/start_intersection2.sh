#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${ROOT_DIR}"

BIN=""
if [ -x "${ROOT_DIR}/build/bin/intersection" ]; then
    BIN="${ROOT_DIR}/build/bin/intersection"
elif [ -x "${ROOT_DIR}/build/bin/aegis_intersection_node" ]; then
    BIN="${ROOT_DIR}/build/bin/aegis_intersection_node"
else
    echo "[ERROR] Intersection binary not found. Build the project first."
    exit 1
fi

echo "[INFO] Starting SMART AEGIS Roadside Intersection 2 (Port 5002)..."
exec "${BIN}" --id 2 --port 5002 "$@"
