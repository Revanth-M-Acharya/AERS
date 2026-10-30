#!/bin/bash
# ==============================================================================
# SMART AEGIS (AERS) Phase 20.5: QNX Remote Deployment Script
# Target: Raspberry Pi 4 Model B (Broadcom BCM2711, aarch64le)
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"

TARGET_IP="${1:-192.168.1.100}"
NODE_ROLE="${2:-COORDINATOR}"
REMOTE_USER="${3:-root}"
REMOTE_DEST="/opt/aegis"

case "${NODE_ROLE}" in
    COORDINATOR|coordinator|RPi1|rpi1)
        PACKAGE_TAR="${ROOT_DIR}/aegis_coordinator_qnx_rpi4.tar.gz"
        ;;
    INTERSECTION|intersection|INT_*|rpi[234])
        PACKAGE_TAR="${ROOT_DIR}/aegis_intersection_qnx_rpi4.tar.gz"
        ;;
    *)
        PACKAGE_TAR="${ROOT_DIR}/aegis_qnx_rpi4.tar.gz"
        ;;
esac

echo "=========================================================="
echo " SMART AEGIS: QNX Remote Deployment"
echo " Target IP   : ${TARGET_IP}"
echo " Node Role   : ${NODE_ROLE}"
echo " Package     : ${PACKAGE_TAR}"
echo " Remote Dest : ${REMOTE_DEST}"
echo "=========================================================="

# Ensure deployment package is built
if [ ! -f "${PACKAGE_TAR}" ]; then
    echo "[INFO] Deployment package not found. Generating now..."
    "${SCRIPT_DIR}/package_qnx.sh"
fi

PKG_FILENAME="$(basename "${PACKAGE_TAR}")"

if ping -c 1 -W 2 "${TARGET_IP}" >/dev/null 2>&1; then
    echo "[NET] Target ${TARGET_IP} is reachable. Initiating SCP transfer..."
    ssh "${REMOTE_USER}@${TARGET_IP}" "mkdir -p ${REMOTE_DEST}"
    scp "${PACKAGE_TAR}" "${REMOTE_USER}@${TARGET_IP}:${REMOTE_DEST}/"
    ssh "${REMOTE_USER}@${TARGET_IP}" "tar -xzf ${REMOTE_DEST}/${PKG_FILENAME} -C ${REMOTE_DEST}/ && rm ${REMOTE_DEST}/${PKG_FILENAME}"
    echo "[SUCCESS] SMART AEGIS deployed to ${TARGET_IP}:${REMOTE_DEST}."
else
    echo "[WARN] Target ${TARGET_IP} is currently unreachable over network."
    echo "[INFO] Standalone deployment package is prepared at: ${PACKAGE_TAR}"
    echo "[INFO] To manually deploy onto SD card / target:"
    echo "       scp ${PACKAGE_TAR} ${REMOTE_USER}@${TARGET_IP}:${REMOTE_DEST}/"
fi

