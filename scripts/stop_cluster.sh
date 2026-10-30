#!/usr/bin/env bash
# ==============================================================================
# SMART AEGIS - Graceful Cluster Termination Script
# ==============================================================================
set -euo pipefail

echo "=========================================================="
echo " SMART AEGIS: Gracefully Halting Cluster Daemons"
echo "=========================================================="

# Terminate intersection nodes first
echo "[1/2] Terminating Roadside Intersection Daemons..."
if pgrep -f "aegis_intersection_node" >/dev/null 2>&1; then
    pkill -TERM -f "aegis_intersection_node" || true
    sleep 1
    if pgrep -f "aegis_intersection_node" >/dev/null 2>&1; then
        pkill -KILL -f "aegis_intersection_node" || true
    fi
    echo "  ✓ Intersection daemons stopped."
else
    echo "  ✓ No intersection daemons running."
fi

# Terminate coordinator
echo "[2/2] Terminating Central Coordinator Daemon..."
if pgrep -f "aegis_coordinator" >/dev/null 2>&1; then
    pkill -TERM -f "aegis_coordinator" || true
    sleep 1
    if pgrep -f "aegis_coordinator" >/dev/null 2>&1; then
        pkill -KILL -f "aegis_coordinator" || true
    fi
    echo "  ✓ Central coordinator stopped."
else
    echo "  ✓ No coordinator daemon running."
fi

echo "=========================================================="
echo " SMART AEGIS Cluster Stopped Gracefully."
echo "=========================================================="

