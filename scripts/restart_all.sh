#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "[INFO] Restarting SMART AEGIS Cluster..."
"${SCRIPT_DIR}/stop_all.sh"
sleep 2

echo "[INFO] Starting Intersections in background..."
"${SCRIPT_DIR}/start_intersection1.sh" >/dev/null 2>&1 &
"${SCRIPT_DIR}/start_intersection2.sh" >/dev/null 2>&1 &
"${SCRIPT_DIR}/start_intersection3.sh" >/dev/null 2>&1 &
sleep 1

echo "[INFO] Starting Coordinator..."
"${SCRIPT_DIR}/start_coordinator.sh"
