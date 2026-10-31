/**
 * @file CameraHealthMonitor.hpp
 * @brief Continuous supervisory monitor for perception camera hardware integrity.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/perception/CameraRegistry.hpp"
#include <string>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace aegis::coordinator::perception::analytics {

struct CameraHealthReport {
    std::string cameraId;
    double healthScore{1.0};           // 0.0 to 1.0
    bool isResponsive{true};
    bool isDegraded{false};
    uint64_t lastFrameMonoMs{0};
    uint64_t silenceDurationMs{0};
    std::string diagnosticsNote;
};

class CameraHealthMonitor {
public:
    explicit CameraHealthMonitor(std::shared_ptr<CameraRegistry> registry);
    ~CameraHealthMonitor() = default;

    /**
     * @brief Run health sweep across all registered cameras.
     * @param timeoutThresholdMs Heartbeat/frame silence limit before marking offline (e.g. 5000 ms).
     */
    std::vector<CameraHealthReport> PerformHealthSweep(uint64_t timeoutThresholdMs = 5000,
                                                      uint64_t currentMonoMs = 0);

    CameraHealthReport GetCameraHealth(const std::string& cameraId) const;
    bool IsCameraUsable(const std::string& cameraId) const;

    void Reset();

private:
    std::shared_ptr<CameraRegistry> registry_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, CameraHealthReport> reports_;
};

} // namespace aegis::coordinator::perception::analytics

