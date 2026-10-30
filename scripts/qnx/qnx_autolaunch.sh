#!/bin/sh
# ==============================================================================
# SMART AEGIS (AERS) Phase 20.6: QNX Boot Auto-Launch Script
# Can be called from /etc/rc.d/rc.local or QNX system startup
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BASE_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

NODE_ID="${1:-}"

# Auto-detect Node ID if not provided
if [ -z "${NODE_ID}" ]; then
    if [ -f "/etc/aegis_node_id" ]; then
        NODE_ID="$(cat /etc/aegis_node_id | tr -d ' \r\n')"
    elif [ -f "${BASE_DIR}/config/node_id.txt" ]; then
        NODE_ID="$(cat "${BASE_DIR}/config/node_id.txt" | tr -d ' \r\n')"
    else
        NODE_ID="COORDINATOR"
    fi
fi

echo "=========================================================="
echo " SMART AEGIS: QNX Boot Auto-Launch"
echo " Detected Node ID: ${NODE_ID}"
echo " Base Directory  : ${BASE_DIR}"
echo "=========================================================="

# 1. Setup static Wi-Fi IP
if [ -x "${SCRIPT_DIR}/qnx_setup_wifi.sh" ]; then
    echo "[BOOT] Initializing static Wi-Fi interface..."
    "${SCRIPT_DIR}/qnx_setup_wifi.sh" "${NODE_ID}" wlan0 || true
fi

# 2. Launch Supervisor based on role
case "${NODE_ID}" in
    COORDINATOR|coordinator|rpi1)
        echo "[BOOT] Starting Central Coordinator Supervisor..."
        exec "${SCRIPT_DIR}/run_qnx_coordinator.sh"
        ;;
    INT_*|rpi[234])
        echo "[BOOT] Starting Intersection Node Supervisor for ${NODE_ID}..."
        exec "${SCRIPT_DIR}/run_qnx_intersection.sh" "${NODE_ID}"
        ;;
    *)
        echo "[ERROR] Unrecognized Node ID: ${NODE_ID}" >&2
        exit 1
        ;;
esac

