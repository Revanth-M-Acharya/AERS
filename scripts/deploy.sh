#!/usr/bin/env bash
# ==============================================================================
# SMART AEGIS (AERS) - Phase 16D Production Deployment Script
# Targets: 4 Raspberry Pi 4B Nodes running QNX Neutrino RTOS / Linux
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build"
DEPLOY_PKG_DIR="${REPO_ROOT}/deploy_package"

echo "=========================================================="
echo " SMART AEGIS: Production Cluster Deployment Packager"
echo "=========================================================="

# 1. Compile all targets
echo "[1/4] Building SMART AEGIS binaries..."
cd "${REPO_ROOT}"
make -j4 build/lib/libaegis.a build/bin/aegis_coordinator build/bin/aegis_intersection_node

# 2. Assemble deployment package
echo "[2/4] Assembling deployment package in ${DEPLOY_PKG_DIR}..."
rm -rf "${DEPLOY_PKG_DIR}"
mkdir -p "${DEPLOY_PKG_DIR}/bin"
mkdir -p "${DEPLOY_PKG_DIR}/config"
mkdir -p "${DEPLOY_PKG_DIR}/scripts"
mkdir -p "${DEPLOY_PKG_DIR}/logs"

cp "${BUILD_DIR}/bin/aegis_coordinator" "${DEPLOY_PKG_DIR}/bin/"
cp "${BUILD_DIR}/bin/aegis_intersection_node" "${DEPLOY_PKG_DIR}/bin/"
cp -r "${REPO_ROOT}/config/"* "${DEPLOY_PKG_DIR}/config/"
cp "${REPO_ROOT}/scripts/run_coordinator.sh" "${DEPLOY_PKG_DIR}/scripts/"
cp "${REPO_ROOT}/scripts/run_intersection.sh" "${DEPLOY_PKG_DIR}/scripts/"
cp "${REPO_ROOT}/scripts/stop_cluster.sh" "${DEPLOY_PKG_DIR}/scripts/"
cp "${REPO_ROOT}/scripts/restart_cluster.sh" "${DEPLOY_PKG_DIR}/scripts/"
cp "${REPO_ROOT}/scripts/health_check.sh" "${DEPLOY_PKG_DIR}/scripts/"
cp "${REPO_ROOT}/scripts/collect_logs.sh" "${DEPLOY_PKG_DIR}/scripts/"
chmod +x "${DEPLOY_PKG_DIR}/scripts/"*.sh

# 3. Create node-specific configuration packages
echo "[3/4] Generating node-specific deployment profiles..."
mkdir -p "${DEPLOY_PKG_DIR}/rpi1_coordinator"
mkdir -p "${DEPLOY_PKG_DIR}/rpi2_int_a"
mkdir -p "${DEPLOY_PKG_DIR}/rpi3_int_b"
mkdir -p "${DEPLOY_PKG_DIR}/rpi4_int_c"

cp "${DEPLOY_PKG_DIR}/config/deployment/rpi1_coordinator.json" "${DEPLOY_PKG_DIR}/rpi1_coordinator/node.json"
cp "${DEPLOY_PKG_DIR}/config/deployment/rpi2_intersection_a.json" "${DEPLOY_PKG_DIR}/rpi2_int_a/node.json"
cp "${DEPLOY_PKG_DIR}/config/deployment/rpi3_intersection_b.json" "${DEPLOY_PKG_DIR}/rpi3_int_b/node.json"
cp "${DEPLOY_PKG_DIR}/config/deployment/rpi4_intersection_c.json" "${DEPLOY_PKG_DIR}/rpi4_int_c/node.json"

# 4. Final verification
echo "[4/4] Verifying package integrity..."
test -f "${DEPLOY_PKG_DIR}/bin/aegis_coordinator"
test -f "${DEPLOY_PKG_DIR}/bin/aegis_intersection_node"
test -f "${DEPLOY_PKG_DIR}/rpi1_coordinator/node.json"

echo "=========================================================="
echo " Deployment package successfully generated in:"
echo " ${DEPLOY_PKG_DIR}"
echo "=========================================================="

