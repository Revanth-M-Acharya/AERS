#!/usr/bin/env bash
# ==============================================================================
# SMART AEGIS - Multi-Node Diagnostic Log Collector Script
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
LOGS_DIR="${REPO_ROOT}/logs"
ARCHIVE_DIR="${REPO_ROOT}/collected_logs_$(date +%Y%m%d_%H%M%S)"

echo "=========================================================="
echo " SMART AEGIS: Collecting Forensic Node Logs"
echo "=========================================================="

mkdir -p "${ARCHIVE_DIR}"

if [[ -d "${LOGS_DIR}" ]]; then
    echo "Copying logs from ${LOGS_DIR} to ${ARCHIVE_DIR}..."
    cp -r "${LOGS_DIR}"/* "${ARCHIVE_DIR}/" 2>/dev/null || true
fi

# Search and copy any loose log files
for log_name in runtime.log network.log gpio.log diagnostics.log crash.log; do
    find "${REPO_ROOT}" -name "${log_name}" -exec cp --parents {} "${ARCHIVE_DIR}/" 2>/dev/null \; || true
done

TAR_FILE="${REPO_ROOT}/aegis_logs_$(date +%Y%m%d_%H%M%S).tar.gz"
tar -czf "${TAR_FILE}" -C "${ARCHIVE_DIR}" .

echo "  ✓ Consolidated logs archive created: ${TAR_FILE}"
echo "=========================================================="
echo " Log Collection Complete."
echo "=========================================================="

