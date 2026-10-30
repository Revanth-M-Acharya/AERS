#!/bin/bash
# ==============================================================================
# SMART AEGIS (AERS) Phase 20.6: QNX Raspberry Pi 4 Deployment Packager
# Generates dedicated and universal deployment archives for QNX Neutrino RTOS
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
DIST_BASE="${ROOT_DIR}/build/qnx_dist"
COORD_DIST="${DIST_BASE}/coordinator"
INT_DIST="${DIST_BASE}/intersection"
UNIV_DIST="${DIST_BASE}/universal"

PKG_COORD="${ROOT_DIR}/aegis_coordinator_qnx_rpi4.tar.gz"
PKG_INT="${ROOT_DIR}/aegis_intersection_qnx_rpi4.tar.gz"
PKG_UNIV="${ROOT_DIR}/aegis_qnx_rpi4.tar.gz"

echo "=========================================================="
echo " SMART AEGIS: Packaging QNX Raspberry Pi 4 Deployments"
echo "=========================================================="

# Select compiled binaries from build-qnx/bin if present, else fallback to build/bin
BIN_SRC="${ROOT_DIR}/build-qnx/bin"
if [ ! -d "${BIN_SRC}" ]; then
    BIN_SRC="${ROOT_DIR}/build/bin"
fi

rm -rf "${DIST_BASE}"
mkdir -p "${COORD_DIST}/bin" "${COORD_DIST}/config/deployment" "${COORD_DIST}/scripts" "${COORD_DIST}/logs"
mkdir -p "${INT_DIST}/bin" "${INT_DIST}/config/deployment" "${INT_DIST}/scripts" "${INT_DIST}/logs"
mkdir -p "${UNIV_DIST}/bin" "${UNIV_DIST}/config/deployment" "${UNIV_DIST}/scripts" "${UNIV_DIST}/logs"

# 1. README & DEPLOYMENT_INSTRUCTIONS generator
cat << 'EOF' > "${DIST_BASE}/README.md"
# SMART AEGIS (AERS) - QNX Raspberry Pi 4 Deployment Bundle
Target OS: QNX SDP 8.0 / QNX SDP 7.1
Target SBC: Raspberry Pi 4 Model B (Broadcom BCM2711, aarch64le)
Network: Wi-Fi TCP/IP with static IP addresses

Cluster Topology:
- Coordinator : 192.168.1.100:9000
- INT_A       : 192.168.1.101:9001
- INT_B       : 192.168.1.102:9002
- INT_C       : 192.168.1.103:9003
EOF

cat << 'EOF' > "${DIST_BASE}/DEPLOYMENT_INSTRUCTIONS.md"
# Deployment & Execution Instructions

1. Network Setup:
   Execute scripts/qnx_setup_wifi.sh on each node to assign static IP.
   Example:
     sh scripts/qnx_setup_wifi.sh 192.168.1.100

2. Starting Coordinator (RPi 1 - 192.168.1.100):
   sh scripts/run_qnx_coordinator.sh

3. Starting Roadside Intersection Nodes (RPi 2-4):
   sh scripts/run_qnx_intersection.sh INT_A   # On RPi 2 (192.168.1.101)
   sh scripts/run_qnx_intersection.sh INT_B   # On RPi 3 (192.168.1.102)
   sh scripts/run_qnx_intersection.sh INT_C   # On RPi 4 (192.168.1.103)

4. Logs:
   Log files are automatically written to the logs/ directory.
EOF

# 2. Populate Coordinator Distribution
echo "[PACK] Populating Coordinator package..."
[ -f "${BIN_SRC}/aegis_coordinator" ] && cp "${BIN_SRC}/aegis_coordinator" "${COORD_DIST}/bin/"
[ -f "${BIN_SRC}/aegis_ambulance" ] && cp "${BIN_SRC}/aegis_ambulance" "${COORD_DIST}/bin/"
chmod +x "${COORD_DIST}/bin/"* 2>/dev/null || true

cp "${ROOT_DIR}/config/hardware.json" "${COORD_DIST}/config/" 2>/dev/null || true
cp "${ROOT_DIR}/config/node.json" "${COORD_DIST}/config/" 2>/dev/null || true
cp "${ROOT_DIR}/config/deployment/rpi1_coordinator.json" "${COORD_DIST}/config/deployment/" 2>/dev/null || true

cp "${ROOT_DIR}/scripts/qnx/run_qnx_coordinator.sh" "${COORD_DIST}/scripts/"
cp "${ROOT_DIR}/scripts/qnx/qnx_setup_wifi.sh" "${COORD_DIST}/scripts/"
cp "${ROOT_DIR}/scripts/qnx/qnx_autolaunch.sh" "${COORD_DIST}/scripts/"
chmod +x "${COORD_DIST}/scripts/"*.sh

cp "${DIST_BASE}/README.md" "${COORD_DIST}/"
cp "${DIST_BASE}/DEPLOYMENT_INSTRUCTIONS.md" "${COORD_DIST}/"

# 3. Populate Intersection Distribution
echo "[PACK] Populating Intersection package..."
[ -f "${BIN_SRC}/aegis_intersection_node" ] && cp "${BIN_SRC}/aegis_intersection_node" "${INT_DIST}/bin/"
[ -f "${BIN_SRC}/aegis_traffic_controller" ] && cp "${BIN_SRC}/aegis_traffic_controller" "${INT_DIST}/bin/"
chmod +x "${INT_DIST}/bin/"* 2>/dev/null || true

cp "${ROOT_DIR}/config/hardware.json" "${INT_DIST}/config/" 2>/dev/null || true
cp "${ROOT_DIR}/config/deployment/rpi2_intersection_a.json" "${INT_DIST}/config/deployment/" 2>/dev/null || true
cp "${ROOT_DIR}/config/deployment/rpi3_intersection_b.json" "${INT_DIST}/config/deployment/" 2>/dev/null || true
cp "${ROOT_DIR}/config/deployment/rpi4_intersection_c.json" "${INT_DIST}/config/deployment/" 2>/dev/null || true

cp "${ROOT_DIR}/scripts/qnx/run_qnx_intersection.sh" "${INT_DIST}/scripts/"
cp "${ROOT_DIR}/scripts/qnx/qnx_setup_wifi.sh" "${INT_DIST}/scripts/"
cp "${ROOT_DIR}/scripts/qnx/qnx_autolaunch.sh" "${INT_DIST}/scripts/"
chmod +x "${INT_DIST}/scripts/"*.sh

cp "${DIST_BASE}/README.md" "${INT_DIST}/"
cp "${DIST_BASE}/DEPLOYMENT_INSTRUCTIONS.md" "${INT_DIST}/"

# 4. Populate Universal Distribution
echo "[PACK] Populating Universal package..."
for bin in aegis_coordinator aegis_intersection_node aegis_ambulance aegis_traffic_controller; do
    if [ -f "${BIN_SRC}/${bin}" ]; then
        cp "${BIN_SRC}/${bin}" "${UNIV_DIST}/bin/"
        chmod +x "${UNIV_DIST}/bin/${bin}"
    fi
done

cp "${ROOT_DIR}/config/hardware.json" "${UNIV_DIST}/config/" 2>/dev/null || true
cp "${ROOT_DIR}/config/node.json" "${UNIV_DIST}/config/" 2>/dev/null || true
cp -r "${ROOT_DIR}/config/deployment/"* "${UNIV_DIST}/config/deployment/" 2>/dev/null || true

cp "${ROOT_DIR}/scripts/qnx/run_qnx_coordinator.sh" "${UNIV_DIST}/scripts/"
cp "${ROOT_DIR}/scripts/qnx/run_qnx_intersection.sh" "${UNIV_DIST}/scripts/"
cp "${ROOT_DIR}/scripts/qnx/qnx_setup_wifi.sh" "${UNIV_DIST}/scripts/"
cp "${ROOT_DIR}/scripts/qnx/qnx_autolaunch.sh" "${UNIV_DIST}/scripts/"
chmod +x "${UNIV_DIST}/scripts/"*.sh

cp "${DIST_BASE}/README.md" "${UNIV_DIST}/"
cp "${DIST_BASE}/DEPLOYMENT_INSTRUCTIONS.md" "${UNIV_DIST}/"

# 5. Compress Archives
echo "[TAR] Creating ${PKG_COORD}..."
tar -czf "${PKG_COORD}" -C "${COORD_DIST}" .

echo "[TAR] Creating ${PKG_INT}..."
tar -czf "${PKG_INT}" -C "${INT_DIST}" .

echo "[TAR] Creating ${PKG_UNIV}..."
tar -czf "${PKG_UNIV}" -C "${UNIV_DIST}" .

echo "=========================================================="
echo " [SUCCESS] Packages created successfully:"
echo " 1. Coordinator  : ${PKG_COORD}"
echo " 2. Intersection : ${PKG_INT}"
echo " 3. Universal    : ${PKG_UNIV}"
echo "=========================================================="
