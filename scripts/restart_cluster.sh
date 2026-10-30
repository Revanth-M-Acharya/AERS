#!/usr/bin/env bash
# ==============================================================================
# SMART AEGIS - Cluster Restart Script
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "=========================================================="
echo " Restarting SMART AEGIS Cluster..."
echo "=========================================================="

"${SCRIPT_DIR}/stop_cluster.sh"
echo "Waiting for TCP sockets to clear..."
sleep 2

echo "Launching Central Coordinator..."
"${SCRIPT_DIR}/run_coordinator.sh" &
sleep 1

echo "Launching Intersection Node A..."
"${SCRIPT_DIR}/run_intersection.sh" INT_A &
sleep 0.5

echo "Launching Intersection Node B..."
"${SCRIPT_DIR}/run_intersection.sh" INT_B &
sleep 0.5

echo "Launching Intersection Node C..."
"${SCRIPT_DIR}/run_intersection.sh" INT_C &
sleep 1

echo "=========================================================="
echo " SMART AEGIS Cluster Restarted Successfully."
echo " Use ./scripts/health_check.sh to verify cluster state."
echo "=========================================================="

