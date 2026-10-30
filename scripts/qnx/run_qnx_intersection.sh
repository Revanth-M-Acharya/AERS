#!/bin/sh
# ==============================================================================
# SMART AEGIS (AERS) Phase 20.6: QNX Intersection Node Production Supervisor
# Target: Raspberry Pi 4 Model B (Broadcom BCM2711, aarch64le)
# Static IPs:
#   INT_A -> 192.168.1.101:9001
#   INT_B -> 192.168.1.102:9002
#   INT_C -> 192.168.1.103:9003
# Features: Supervisor loop, auto-restart on failure, signal trapping, logging
# ==============================================================================

NODE_ID="${1:-INT_A}"
BASE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
export AEGIS_HOME="${BASE_DIR}"
export AEGIS_NODE_ID="${NODE_ID}"
export AEGIS_LOG_DIR="${AEGIS_HOME}/logs"
export LD_LIBRARY_PATH="${AEGIS_HOME}/lib:${LD_LIBRARY_PATH:-/usr/lib}"

case "${NODE_ID}" in
    INT_A|rpi2)
        NODE_ID="INT_A"
        CONFIG_FILE="${AEGIS_HOME}/config/deployment/rpi2_intersection_a.json"
        NODE_IP="192.168.1.101"
        ;;
    INT_B|rpi3)
        NODE_ID="INT_B"
        CONFIG_FILE="${AEGIS_HOME}/config/deployment/rpi3_intersection_b.json"
        NODE_IP="192.168.1.102"
        ;;
    INT_C|rpi4)
        NODE_ID="INT_C"
        CONFIG_FILE="${AEGIS_HOME}/config/deployment/rpi4_intersection_c.json"
        NODE_IP="192.168.1.103"
        ;;
    *)
        echo "[ERROR] Unknown Node ID: ${NODE_ID}. Expected: INT_A, INT_B, or INT_C" >&2
        exit 1
        ;;
esac

mkdir -p "${AEGIS_LOG_DIR}"

STDOUT_LOG="${AEGIS_LOG_DIR}/${NODE_ID}_stdout.log"
STDERR_LOG="${AEGIS_LOG_DIR}/${NODE_ID}_stderr.log"
PID_FILE="${AEGIS_LOG_DIR}/${NODE_ID}.pid"

echo "=========================================================="
echo " SMART AEGIS: QNX Roadside Intersection Supervisor"
echo " Node ID        : ${NODE_ID}"
echo " Static IP      : ${NODE_IP}"
echo " Coordinator IP : 192.168.1.100:9000"
echo " Configuration  : ${CONFIG_FILE}"
echo " Logs           : ${AEGIS_LOG_DIR}"
echo "=========================================================="

BIN="${AEGIS_HOME}/bin/aegis_intersection_node"
if [ ! -x "${BIN}" ]; then
    echo "[ERROR] Executable not found at: ${BIN}" >&2
    exit 1
fi

CHILD_PID=0
SHUTDOWN_REQUESTED=0

cleanup() {
    SHUTDOWN_REQUESTED=1
    echo "[SUPERVISOR] Graceful shutdown signal received for ${NODE_ID}..."
    if [ "${CHILD_PID}" -gt 0 ] && kill -0 "${CHILD_PID}" 2>/dev/null; then
        echo "[SUPERVISOR] Terminating Intersection Node ${NODE_ID} (PID: ${CHILD_PID})..."
        kill -TERM "${CHILD_PID}" 2>/dev/null
        wait "${CHILD_PID}" 2>/dev/null
    fi
    rm -f "${PID_FILE}"
    echo "[SUPERVISOR] Node ${NODE_ID} shutdown complete."
    exit 0
}

trap cleanup INT TERM

MAX_RESTARTS=10
RESTART_COUNT=0
BACKOFF_SEC=2

while [ "${SHUTDOWN_REQUESTED}" -eq 0 ]; do
    echo "[SUPERVISOR] Launching Intersection Node ${NODE_ID} at QNX Real-Time Priority 21..."
    
    if command -v on >/dev/null 2>&1; then
        on -p 21 "${BIN}" --config "${CONFIG_FILE}" >> "${STDOUT_LOG}" 2>> "${STDERR_LOG}" &
    else
        "${BIN}" --config "${CONFIG_FILE}" >> "${STDOUT_LOG}" 2>> "${STDERR_LOG}" &
    fi
    CHILD_PID=$!
    echo "${CHILD_PID}" > "${PID_FILE}"
    echo "[SUPERVISOR] Node ${NODE_ID} running (PID: ${CHILD_PID})"

    wait "${CHILD_PID}"
    EXIT_CODE=$?

    if [ "${SHUTDOWN_REQUESTED}" -eq 1 ]; then
        break
    fi

    echo "[SUPERVISOR] Node ${NODE_ID} exited with code ${EXIT_CODE}"
    if [ "${EXIT_CODE}" -eq 0 ]; then
        echo "[SUPERVISOR] Clean exit. Supervisor stopping."
        break
    fi

    RESTART_COUNT=$((RESTART_COUNT + 1))
    if [ "${RESTART_COUNT}" -gt "${MAX_RESTARTS}" ]; then
        echo "[SUPERVISOR] Maximum restart limit (${MAX_RESTARTS}) reached for ${NODE_ID}. Halting." >&2
        rm -f "${PID_FILE}"
        exit 1
    fi

    echo "[SUPERVISOR] Restarting ${NODE_ID} in ${BACKOFF_SEC} seconds (Attempt ${RESTART_COUNT}/${MAX_RESTARTS})..."
    sleep "${BACKOFF_SEC}"
done

rm -f "${PID_FILE}"
exit 0
