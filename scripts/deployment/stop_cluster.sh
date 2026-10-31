#!/usr/bin/env bash
# ==============================================================================
# SMART AEGIS - Phase 11 Cluster Shutdown Script
# Remotely stops Coordinator and Traffic Signal Nodes across all 4 Pis.
# ==============================================================================
set -euo pipefail

RPI_COORD_IP="${RPI_COORD_IP:-192.168.1.100}"
RPI_INT_A_IP="${RPI_INT_A_IP:-192.168.1.101}"
RPI_INT_B_IP="${RPI_INT_B_IP:-192.168.1.102}"
RPI_INT_C_IP="${RPI_INT_C_IP:-192.168.1.103}"
SSH_USER="${SSH_USER:-qnxuser}"
SSH_OPTS="-o ConnectTimeout=3 -o BatchMode=yes -o StrictHostKeyChecking=no"

echo "=========================================================="
echo " SMART AEGIS: Graceful Cluster Shutdown                   "
echo "=========================================================="

for ip in "${RPI_INT_A_IP}" "${RPI_INT_B_IP}" "${RPI_INT_C_IP}" "${RPI_COORD_IP}"; do
    echo "Stopping AEGIS processes on ${ip}..."
    ssh ${SSH_OPTS} "${SSH_USER}@${ip}" "killall aegis_traffic_signal_node aegis_coordinator 2>/dev/null || true" 2>/dev/null || echo "Node ${ip} stopped locally."
done

echo "Cluster shutdown complete."
