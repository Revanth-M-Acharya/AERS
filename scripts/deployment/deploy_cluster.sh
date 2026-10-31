#!/usr/bin/env bash
# ==============================================================================
# SMART AEGIS - Phase 11 Remote Cluster Deployment Utility
# Target: 4x Raspberry Pi 4 Model B running QNX Neutrino RTOS 8.0 (aarch64le)
# Deploys builds, node configuration, and startup files from single dev machine.
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build"
CONFIG_DIR="${REPO_ROOT}/config/cluster"
STAGING_DIR="${REPO_ROOT}/build/deployment_staging"

# Cluster Nodes
RPI_COORD_IP="${RPI_COORD_IP:-192.168.1.100}"
RPI_INT_A_IP="${RPI_INT_A_IP:-192.168.1.101}"
RPI_INT_B_IP="${RPI_INT_B_IP:-192.168.1.102}"
RPI_INT_C_IP="${RPI_INT_C_IP:-192.168.1.103}"
SSH_USER="${SSH_USER:-qnxuser}"
SSH_OPTS="-o ConnectTimeout=3 -o BatchMode=yes -o StrictHostKeyChecking=no"

echo "=========================================================="
echo " SMART AEGIS: Remote 4-Node Cluster Deployment Manager    "
echo " Target: Raspberry Pi 4 Model B (QNX SDP 8.0 / aarch64le) "
echo "=========================================================="

# 1. Verify / Build Binaries
echo "[1/4] Ensuring binaries are built..."
cd "${REPO_ROOT}"
make -j4 build/lib/libaegis.a build/bin/aegis_coordinator build/bin/aegis_traffic_signal_node

# 2. Prepare Local Staging Packages
echo "[2/4] Assembling node-specific staging packages..."
rm -rf "${STAGING_DIR}"
mkdir -p "${STAGING_DIR}/rpi_coord/bin" "${STAGING_DIR}/rpi_coord/config" "${STAGING_DIR}/rpi_coord/logs"
mkdir -p "${STAGING_DIR}/int_a/bin" "${STAGING_DIR}/int_a/config" "${STAGING_DIR}/int_a/logs"
mkdir -p "${STAGING_DIR}/int_b/bin" "${STAGING_DIR}/int_b/config" "${STAGING_DIR}/int_b/logs"
mkdir -p "${STAGING_DIR}/int_c/bin" "${STAGING_DIR}/int_c/config" "${STAGING_DIR}/int_c/logs"

# Copy Coordinator Package
cp "${BUILD_DIR}/bin/aegis_coordinator" "${STAGING_DIR}/rpi_coord/bin/"
cp "${CONFIG_DIR}/rpi1_coordinator.conf" "${STAGING_DIR}/rpi_coord/config/node.conf"
cp "${CONFIG_DIR}/cluster_topology.json" "${STAGING_DIR}/rpi_coord/config/"

# Copy Traffic Node Packages
for node in "int_a:rpi2_node_a.conf" "int_b:rpi3_node_b.conf" "int_c:rpi4_node_c.conf"; do
    target_dir="${node%%:*}"
    conf_file="${node##*:}"
    cp "${BUILD_DIR}/bin/aegis_traffic_signal_node" "${STAGING_DIR}/${target_dir}/bin/"
    cp "${CONFIG_DIR}/${conf_file}" "${STAGING_DIR}/${target_dir}/config/node.conf"
    cp "${CONFIG_DIR}/cluster_topology.json" "${STAGING_DIR}/${target_dir}/config/"
done

echo "[3/4] Packaging complete in ${STAGING_DIR}"

# 3. Remote Transfer (SCP or Local Staging Verification)
echo "[4/4] Deploying to target Raspberry Pi cluster..."
DEPLOY_MODE="${1:-staging}"

if [ "${DEPLOY_MODE}" == "live" ]; then
    echo "Transferring over SSH/SCP to live QNX nodes..."
    
    # Deploy to Coordinator
    echo " -> Deploying to Coordinator Pi (${RPI_COORD_IP})..."
    scp -r ${SSH_OPTS} "${STAGING_DIR}/rpi_coord/"* "${SSH_USER}@${RPI_COORD_IP}:/opt/aegis/" || echo "Warning: RPI_COORD unreachable, staged locally"

    # Deploy to Node A
    echo " -> Deploying to Traffic Node A (${RPI_INT_A_IP})..."
    scp -r ${SSH_OPTS} "${STAGING_DIR}/int_a/"* "${SSH_USER}@${RPI_INT_A_IP}:/opt/aegis/" || echo "Warning: INT_A unreachable, staged locally"

    # Deploy to Node B
    echo " -> Deploying to Traffic Node B (${RPI_INT_B_IP})..."
    scp -r ${SSH_OPTS} "${STAGING_DIR}/int_b/"* "${SSH_USER}@${RPI_INT_B_IP}:/opt/aegis/" || echo "Warning: INT_B unreachable, staged locally"

    # Deploy to Node C
    echo " -> Deploying to Traffic Node C (${RPI_INT_C_IP})..."
    scp -r ${SSH_OPTS} "${STAGING_DIR}/int_c/"* "${SSH_USER}@${RPI_INT_C_IP}:/opt/aegis/" || echo "Warning: INT_C unreachable, staged locally"
else
    echo "Deployment staged successfully in ${STAGING_DIR} (Pass 'live' as argument to trigger SCP transfer)."
fi

echo "=========================================================="
echo " Cluster Deployment Pipeline Completed Successfully!     "
echo "=========================================================="
