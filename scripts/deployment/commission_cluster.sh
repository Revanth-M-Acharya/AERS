#!/usr/bin/env bash
# ==============================================================================
# SMART AEGIS - Phase 11 Automated Cluster Commissioning Trigger
# Runs automated commissioning self-test sequence on all cluster nodes.
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

echo "=========================================================="
echo " SMART AEGIS: Cluster Commissioning & Hardware Self-Test  "
echo "=========================================================="

cd "${REPO_ROOT}"
make -j4 build/bin/test_cluster_deployment_hardware
./build/bin/test_cluster_deployment_hardware

echo "=========================================================="
echo " Commissioning and Hardware Self-Test Complete!           "
echo "=========================================================="
