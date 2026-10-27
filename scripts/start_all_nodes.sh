#!/usr/bin/env bash
set -euo pipefail

# ==============================================================================
# SMART AEGIS: Multi-Node Local Orchestrator & Deployment Launcher
#
# Startup Sequence:
# 1. Raspberry Pi 1 (Central Coordinator) starts first.
# 2. Awaits Coordinator socket listener readiness.
# 3. Raspberry Pi 2 (Intersection Node A) starts and connects.
# 4. Raspberry Pi 3 (Intersection Node B) starts and connects.
# 5. Raspberry Pi 4 (Intersection Node C) starts and connects.
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BUILD_DIR="${ROOT_DIR}/build/bin"

COORD_BIN="${BUILD_DIR}/aegis_coordinator"
NODE_BIN="${BUILD_DIR}/aegis_intersection_node"

if [ ! -f "${COORD_BIN}" ] || [ ! -f "${NODE_BIN}" ]; then
    echo "[BUILD] Compiling binaries..."
    make -C "${ROOT_DIR}" all
fi

PIDS=()

cleanup() {
    echo ""
    echo "[SHUTDOWN] Stopping all AERS deployment nodes..."
    for pid in "${PIDS[@]}"; do
        if kill -0 "${pid}" 2>/dev/null; then
            kill -TERM "${pid}" 2>/dev/null || true
        fi
    done
    wait 2>/dev/null || true
    echo "[SHUTDOWN] All nodes stopped cleanly."
}

trap cleanup INT TERM EXIT

echo "=========================================================="
echo " SMART AEGIS: Multi-Node Deployment Launcher"
echo "=========================================================="

# 1. Start Coordinator first
echo "[1/4] Launching Raspberry Pi 1 (Central Coordinator)..."
"${COORD_BIN}" --config "${ROOT_DIR}/config/deployment/rpi1_coordinator.json" &
PIDS+=($!)
sleep 0.5

# 2. Start Intersection Node A
echo "[2/4] Launching Raspberry Pi 2 (Intersection Node A / INT_A)..."
"${NODE_BIN}" --config "${ROOT_DIR}/config/deployment/rpi2_intersection_a.json" &
PIDS+=($!)
sleep 0.2

# 3. Start Intersection Node B
echo "[3/4] Launching Raspberry Pi 3 (Intersection Node B / INT_B)..."
"${NODE_BIN}" --config "${ROOT_DIR}/config/deployment/rpi3_intersection_b.json" &
PIDS+=($!)
sleep 0.2

# 4. Start Intersection Node C
echo "[4/4] Launching Raspberry Pi 4 (Intersection Node C / INT_C)..."
"${NODE_BIN}" --config "${ROOT_DIR}/config/deployment/rpi4_intersection_c.json" &
PIDS+=($!)

echo "=========================================================="
echo " All 4 Raspberry Pi Nodes Active & Connected."
echo " Press Ctrl+C to terminate the deployment cluster."
echo "=========================================================="

wait
