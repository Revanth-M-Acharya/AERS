#!/bin/sh
# ==============================================================================
# SMART AEGIS (AERS) Phase 20.6: QNX Coordinator Production Supervisor Script
# Target: Raspberry Pi 4 Model B (Broadcom BCM2711, aarch64le)
# Static IP: 192.168.1.100:9000
# Features: Supervisor loop, auto-restart on failure, signal trapping, logging
# ==============================================================================

BASE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
export AEGIS_HOME="${BASE_DIR}"
export AEGIS_NODE_ID="COORDINATOR"
export AEGIS_LOG_DIR="${AEGIS_HOME}/logs"
export LD_LIBRARY_PATH="${AEGIS_HOME}/lib:${LD_LIBRARY_PATH:-/usr/lib}"
export QT_QPA_PLATFORM="offscreen"

mkdir -p "${AEGIS_LOG_DIR}"

STDOUT_LOG="${AEGIS_LOG_DIR}/coordinator_stdout.log"
STDERR_LOG="${AEGIS_LOG_DIR}/coordinator_stderr.log"
PID_FILE="${AEGIS_LOG_DIR}/coordinator.pid"

echo "=========================================================="
echo " SMART AEGIS: QNX Coordinator Supervisor"
echo " Node ID       : ${AEGIS_NODE_ID}"
echo " Static IP     : 192.168.1.100:9000"
echo " Base Dir      : ${AEGIS_HOME}"
echo " Logs          : ${AEGIS_LOG_DIR}"
echo "=========================================================="

CONFIG_FILE="${AEGIS_HOME}/config/deployment/rpi1_coordinator.json"
if [ ! -f "${CONFIG_FILE}" ]; then
    CONFIG_FILE="${AEGIS_HOME}/config/hardware.json"
fi

BIN="${AEGIS_HOME}/bin/aegis_coordinator"
if [ ! -x "${BIN}" ]; then
    echo "[ERROR] Executable not found at: ${BIN}" >&2
    exit 1
fi

CHILD_PID=0
SHUTDOWN_REQUESTED=0

cleanup() {
    SHUTDOWN_REQUESTED=1
    echo "[SUPERVISOR] Graceful shutdown signal received..."
    if [ "${CHILD_PID}" -gt 0 ] && kill -0 "${CHILD_PID}" 2>/dev/null; then
        echo "[SUPERVISOR] Terminating Coordinator process (PID: ${CHILD_PID})..."
        kill -TERM "${CHILD_PID}" 2>/dev/null
        wait "${CHILD_PID}" 2>/dev/null
    fi
    rm -f "${PID_FILE}"
    echo "[SUPERVISOR] Shutdown complete."
    exit 0
}

trap cleanup INT TERM

MAX_RESTARTS=10
RESTART_COUNT=0
BACKOFF_SEC=2

while [ "${SHUTDOWN_REQUESTED}" -eq 0 ]; do
    echo "[SUPERVISOR] Launching Central Coordinator at QNX Real-Time Priority 21..."
    
    if command -v on >/dev/null 2>&1; then
        on -p 21 "${BIN}" --config "${CONFIG_FILE}" >> "${STDOUT_LOG}" 2>> "${STDERR_LOG}" &
    else
        "${BIN}" --config "${CONFIG_FILE}" >> "${STDOUT_LOG}" 2>> "${STDERR_LOG}" &
    fi
    CHILD_PID=$!
    echo "${CHILD_PID}" > "${PID_FILE}"
    echo "[SUPERVISOR] Coordinator running (PID: ${CHILD_PID})"

    wait "${CHILD_PID}"
    EXIT_CODE=$?

    if [ "${SHUTDOWN_REQUESTED}" -eq 1 ]; then
        break
    fi

    echo "[SUPERVISOR] Coordinator exited with code ${EXIT_CODE}"
    if [ "${EXIT_CODE}" -eq 0 ]; then
        echo "[SUPERVISOR] Clean exit. Supervisor stopping."
        break
    fi

    RESTART_COUNT=$((RESTART_COUNT + 1))
    if [ "${RESTART_COUNT}" -gt "${MAX_RESTARTS}" ]; then
        echo "[SUPERVISOR] Maximum restart limit (${MAX_RESTARTS}) reached. Halting." >&2
        rm -f "${PID_FILE}"
        exit 1
    fi

    echo "[SUPERVISOR] Restarting in ${BACKOFF_SEC} seconds (Attempt ${RESTART_COUNT}/${MAX_RESTARTS})..."
    sleep "${BACKOFF_SEC}"
done

rm -f "${PID_FILE}"
exit 0
