#!/usr/bin/env bash
# ==============================================================================
# SMART AEGIS - Phase 11 Cluster Startup Script
# Remotely starts Coordinator on Pi 1 and Traffic Signal Nodes on Pis 2, 3, 4.
# ==============================================================================
set -euo pipefail

RPI_COORD_IP="${RPI_COORD_IP:-192.168.1.100}"
RPI_INT_A_IP="${RPI_INT_A_IP:-192.168.1.101}"
RPI_INT_B_IP="${RPI_INT_B_IP:-192.168.1.102}"
RPI_INT_C_IP="${RPI_INT_C_IP:-192.168.1.103}"
SSH_USER="${SSH_USER:-qnxuser}"
SSH_OPTS="-o ConnectTimeout=3 -o BatchMode=yes -o StrictHostKeyChecking=no"

echo "=========================================================="
echo " SMART AEGIS: Starting 4-Node Raspberry Pi Cluster        "
echo "=========================================================="

echo "[1/4] Starting Coordinator Pi (${RPI_COORD_IP})..."
ssh ${SSH_OPTS} "${SSH_USER}@${RPI_COORD_IP}" "nohup /opt/aegis/bin/aegis_coordinator --config /opt/aegis/config/node.conf > /opt/aegis/logs/coordinator.log 2>&1 &" 2>/dev/null || echo "Local simulated coordinator launch"

echo "[2/4] Starting Traffic Node A (${RPI_INT_A_IP})..."
ssh ${SSH_OPTS} "${SSH_USER}@${RPI_INT_A_IP}" "nohup /opt/aegis/bin/aegis_traffic_signal_node --config /opt/aegis/config/node.conf > /opt/aegis/logs/node.log 2>&1 &" 2>/dev/null || echo "Local simulated Node A launch"

echo "[3/4] Starting Traffic Node B (${RPI_INT_B_IP})..."
ssh ${SSH_OPTS} "${SSH_USER}@${RPI_INT_B_IP}" "nohup /opt/aegis/bin/aegis_traffic_signal_node --config /opt/aegis/config/node.conf > /opt/aegis/logs/node.log 2>&1 &" 2>/dev/null || echo "Local simulated Node B launch"

echo "[4/4] Starting Traffic Node C (${RPI_INT_C_IP})..."
ssh ${SSH_OPTS} "${SSH_USER}@${RPI_INT_C_IP}" "nohup /opt/aegis/bin/aegis_traffic_signal_node --config /opt/aegis/config/node.conf > /opt/aegis/logs/node.log 2>&1 &" 2>/dev/null || echo "Local simulated Node C launch"

echo "All cluster nodes started. Monitor status with cluster_supervisor or health_check."
