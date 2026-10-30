#!/usr/bin/env bash
# ==============================================================================
# SMART AEGIS - Run Central Coordinator Daemon (Raspberry Pi 1)
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
CONFIG_FILE="${1:-${REPO_ROOT}/config/deployment/rpi1_coordinator.json}"
LOG_DIR="${REPO_ROOT}/logs/coordinator"

mkdir -p "${LOG_DIR}"

if [[ ! -f "${CONFIG_FILE}" ]]; then
    if [[ -f "${REPO_ROOT}/config/node.json" ]]; then
        CONFIG_FILE="${REPO_ROOT}/config/node.json"
    else
        echo "[ERROR] Coordinator configuration file not found: ${CONFIG_FILE}"
        exit 1
    fi
fi

BIN="${REPO_ROOT}/build/bin/aegis_coordinator"
if [[ ! -x "${BIN}" ]]; then
    echo "[ERROR] Binary not found or not executable: ${BIN}"
    echo "Please build the project first with: make build/bin/aegis_coordinator"
    exit 1
fi

echo "[STARTING] Central Coordinator with config: ${CONFIG_FILE}"
exec "${BIN}" --config "${CONFIG_FILE}"

