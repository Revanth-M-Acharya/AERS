/**
 * @file CameraHealthMonitor.cpp
 * @brief Implementation of CameraHealthMonitor heartbeat tracking and diagnosis.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/perception/analytics/CameraHealthMonitor.hpp"
#include "common/utils/TimeUtils.hpp"
#include <algorithm>

namespace aegis::coordinator::perception::analytics {

CameraHealthMonitor::CameraHealthMonitor(std::shared_ptr<CameraRegistry> registry)
    : registry_(std::move(registry)) {}

std::vector<CameraHealthReport> CameraHealthMonitor::PerformHealthSweep(
    uint64_t timeoutThresholdMs,
    uint64_t currentMonoMs) {

    if (!registry_) return {};
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    auto cameras = registry_->GetAllCameras();
    std::vector<CameraHealthReport> swept;

    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& cam : cameras) {
        CameraHealthReport report;
        report.cameraId = cam.cameraId;
        report.lastFrameMonoMs = cam.lastFrameTimestampMonoMs;

        uint64_t silence = 0;
        if (cam.lastFrameTimestampMonoMs > 0 && currentMonoMs > cam.lastFrameTimestampMonoMs) {
            silence = currentMonoMs - cam.lastFrameTimestampMonoMs;
        }
        report.silenceDurationMs = silence;

        if (cam.status == CameraStatus::OFFLINE) {
            report.healthScore = 0.0;
            report.isResponsive = false;
            report.isDegraded = false;
            report.diagnosticsNote = "Camera marked OFFLINE in registry";
        } else if (cam.lastFrameTimestampMonoMs > 0 && silence > timeoutThresholdMs) {
            report.healthScore = 0.10;
            report.isResponsive = false;
            report.isDegraded = true;
            report.diagnosticsNote = "Frame silence timeout: " + std::to_string(silence) + "ms";
            registry_->UpdateCameraStatus(cam.cameraId, CameraStatus::DEGRADED);
            registry_->UpdateCameraHealth(cam.cameraId, 0.10);
        } else {
            report.isResponsive = true;
            report.isDegraded = (cam.healthScore < 0.50);
            report.healthScore = cam.healthScore;
            report.diagnosticsNote = report.isDegraded ? "Degraded optics/frame jitter" : "Nominal operational state";
        }

        reports_[cam.cameraId] = report;
        swept.push_back(report);
    }

    return swept;
}

CameraHealthReport CameraHealthMonitor::GetCameraHealth(const std::string& cameraId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = reports_.find(cameraId);
    if (it != reports_.end()) return it->second;
    CameraHealthReport def;
    def.cameraId = cameraId;
    return def;
}

bool CameraHealthMonitor::IsCameraUsable(const std::string& cameraId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = reports_.find(cameraId);
    if (it != reports_.end()) {
        return it->second.isResponsive && (it->second.healthScore >= 0.25);
    }
    return true;
}

void CameraHealthMonitor::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    reports_.clear();
}

} // namespace aegis::coordinator::perception::analytics
