#!/usr/bin/env bash
# ==============================================================================
# SMART AEGIS - Run Roadside Intersection Node Daemon (Raspberry Pis 2, 3, 4)
# Usage: ./run_intersection.sh [INT_A | INT_B | INT_C | path/to/config.json]
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
TARGET="${1:-INT_A}"

CONFIG_FILE=""
if [[ -f "${TARGET}" ]]; then
    CONFIG_FILE="${TARGET}"
elif [[ "${TARGET}" == "INT_A" || "${TARGET}" == "int_a" || "${TARGET}" == "rpi2" ]]; then
    CONFIG_FILE="${REPO_ROOT}/config/deployment/rpi2_intersection_a.json"
elif [[ "${TARGET}" == "INT_B" || "${TARGET}" == "int_b" || "${TARGET}" == "rpi3" ]]; then
    CONFIG_FILE="${REPO_ROOT}/config/deployment/rpi3_intersection_b.json"
elif [[ "${TARGET}" == "INT_C" || "${TARGET}" == "int_c" || "${TARGET}" == "rpi4" ]]; then
    CONFIG_FILE="${REPO_ROOT}/config/deployment/rpi4_intersection_c.json"
else
    CONFIG_FILE="${REPO_ROOT}/config/deployment/rpi2_intersection_a.json"
fi

if [[ ! -f "${CONFIG_FILE}" ]]; then
    echo "[ERROR] Intersection configuration file not found: ${CONFIG_FILE}"
    exit 1
fi

BIN="${REPO_ROOT}/build/bin/aegis_intersection_node"
if [[ ! -x "${BIN}" ]]; then
    echo "[ERROR] Binary not found or not executable: ${BIN}"
    echo "Please build the project first with: make build/bin/aegis_intersection_node"
    exit 1
fi

echo "[STARTING] Roadside Intersection Node with config: ${CONFIG_FILE}"
exec "${BIN}" --config "${CONFIG_FILE}"

