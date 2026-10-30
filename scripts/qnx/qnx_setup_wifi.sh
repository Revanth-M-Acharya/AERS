#!/bin/sh
# ==============================================================================
# SMART AEGIS (AERS) Phase 20.5: QNX Wi-Fi Static IP Configuration Script
# Target: Raspberry Pi 4 Model B (Broadcom BCM2711, aarch64le)
# ==============================================================================

set -e

NODE_ROLE="${1:-COORDINATOR}"
WIFI_IFACE="${2:-wlan0}"

case "${NODE_ROLE}" in
    COORDINATOR|rpi1)
        STATIC_IP="192.168.1.100"
        ;;
    INT_A|rpi2)
        STATIC_IP="192.168.1.101"
        ;;
    INT_B|rpi3)
        STATIC_IP="192.168.1.102"
        ;;
    INT_C|rpi4)
        STATIC_IP="192.168.1.103"
        ;;
    *)
        STATIC_IP="${NODE_ROLE}"
        ;;
esac

NETMASK="255.255.255.0"
GATEWAY="192.168.1.1"

echo "=========================================================="
echo " SMART AEGIS: QNX Wi-Fi Static Network Setup"
echo " Role / Target : ${NODE_ROLE}"
echo " Interface     : ${WIFI_IFACE}"
echo " Static IP     : ${STATIC_IP}"
echo " Netmask       : ${NETMASK}"
echo " Gateway       : ${GATEWAY}"
echo "=========================================================="

if command -v ifconfig >/dev/null 2>&1; then
    echo "[NET] Configuring static IP on ${WIFI_IFACE}..."
    ifconfig "${WIFI_IFACE}" "${STATIC_IP}" netmask "${NETMASK}" up
    
    if command -v route >/dev/null 2>&1; then
        echo "[NET] Adding default gateway ${GATEWAY}..."
        route add default "${GATEWAY}" 2>/dev/null || true
    fi
    echo "[SUCCESS] Network interface ${WIFI_IFACE} active at ${STATIC_IP}."
else
    echo "[SIMULATION] ifconfig not found on host; simulated configuration for ${STATIC_IP}."
fi

