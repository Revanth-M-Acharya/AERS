#!/usr/bin/env bash
# ==============================================================================
# SMART AEGIS - Phase 11 Centralized Log Aggregator
# Collects runtime, commissioning, and diagnostic logs from all 4 Pis.
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
OUT_DIR="${REPO_ROOT}/collected_logs_cluster_${TIMESTAMP}"

RPI_COORD_IP="${RPI_COORD_IP:-192.168.1.100}"
RPI_INT_A_IP="${RPI_INT_A_IP:-192.168.1.101}"
RPI_INT_B_IP="${RPI_INT_B_IP:-192.168.1.102}"
RPI_INT_C_IP="${RPI_INT_C_IP:-192.168.1.103}"
SSH_USER="${SSH_USER:-qnxuser}"
SSH_OPTS="-o ConnectTimeout=3 -o BatchMode=yes -o StrictHostKeyChecking=no"

echo "=========================================================="
echo " SMART AEGIS: Cluster Log Collection to ${OUT_DIR}"
echo "=========================================================="
mkdir -p "${OUT_DIR}/rpi_coord" "${OUT_DIR}/int_a" "${OUT_DIR}/int_b" "${OUT_DIR}/int_c"

for target in "rpi_coord:${RPI_COORD_IP}" "int_a:${RPI_INT_A_IP}" "int_b:${RPI_INT_B_IP}" "int_c:${RPI_INT_C_IP}"; do
    node_name="${target%%:*}"
    ip="${target##*:}"
    echo "Fetching logs from ${node_name} (${ip})..."
    scp ${SSH_OPTS} "${SSH_USER}@${ip}:/opt/aegis/logs/*" "${OUT_DIR}/${node_name}/" 2>/dev/null || echo "Node ${ip} offline, copying local logs if available"
done

echo "Cluster logs collected into ${OUT_DIR}"
