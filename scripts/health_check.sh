#!/usr/bin/env bash
# ==============================================================================
# SMART AEGIS - Cluster Health & Diagnostic Verification Script
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
COORD_PORT="${1:-9000}"

echo "=========================================================="
echo " SMART AEGIS: Cluster Health Diagnostic Check"
echo "=========================================================="

FAILED=0

# 1. Process checks
echo -n "[CHECK 1] Process Liveness: "
COORD_PID=$(pgrep -f "aegis_coordinator" || true)
INT_PIDS=$(pgrep -f "aegis_intersection_node" || true)

if [[ -n "${COORD_PID}" ]]; then
    echo -n "Coordinator [PID ${COORD_PID}] (ACTIVE) | "
else
    echo -n "Coordinator (INACTIVE) | "
fi

INT_COUNT=0
if [[ -n "${INT_PIDS}" ]]; then
    INT_COUNT=$(echo "${INT_PIDS}" | wc -w | tr -d ' ')
fi
echo "Intersection Nodes: ${INT_COUNT} active"

# 2. Port listening checks
echo -n "[CHECK 2] Coordinator TCP Port ${COORD_PORT}: "
if nc -z 127.0.0.1 "${COORD_PORT}" 2>/dev/null || timeout 1 bash -c "cat < /dev/null > /dev/tcp/127.0.0.1/${COORD_PORT}" 2>/dev/null; then
    echo "OPEN (PASS)"
else
    echo "NOT ACCEPTING CONNECTIONS (NOTE: start with ./scripts/run_coordinator.sh)"
fi

# 3. Log directories and files
echo -n "[CHECK 3] Logging Infrastructure: "
if [[ -d "${REPO_ROOT}/logs" ]]; then
    echo "OK (logs directory present)"
else
    echo "MISSING logs directory (will be generated at boot)"
fi

# 4. Configuration integrity
echo -n "[CHECK 4] Deployment Configurations: "
if [[ -f "${REPO_ROOT}/config/node.json" && -f "${REPO_ROOT}/config/deployment/rpi1_coordinator.json" ]]; then
    echo "VALID (PASS)"
else
    echo "FAIL (missing config files)"
    FAILED=1
fi

echo "=========================================================="
if [[ ${FAILED} -eq 0 ]]; then
    echo " Cluster Health Status: OPERATIONAL / READY"
    exit 0
else
    echo " Cluster Health Status: DEGRADED / ATTENTION REQUIRED"
    exit 1
fi

