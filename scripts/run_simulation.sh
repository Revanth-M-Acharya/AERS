#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

echo "=========================================================="
echo " Starting SMART AEGIS Multi-Node Simulation Testbed"
echo "=========================================================="

# Check if MQTT broker is alive
if ! nc -z 127.0.0.1 1883 2>/dev/null; then
    echo "[WARN] Local MQTT broker on port 1883 not detected."
    echo "       Please ensure Mosquitto is running: mosquitto -v"
fi

echo "Launching simulation components (mock GPS, traffic FSM, telemetry)..."
echo "Press Ctrl+C to terminate all simulated nodes."

# Simulators will be invoked here as they are implemented
trap 'kill $(jobs -p) 2>/dev/null || true' SIGINT SIGTERM EXIT
wait
