/**
 * @file CameraRegistry.hpp
 * @brief Registry and directory for roadside CCTV and perception camera nodes.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <cstdint>

namespace aegis::coordinator::perception {

enum class CameraStatus {
    OFFLINE,
    ACTIVE,
    STREAMING,
    DEGRADED,
    CALIBRATING
};

std::string CameraStatusToString(CameraStatus status);
CameraStatus StringToCameraStatus(const std::string& str);

struct CameraInfo {
    std::string cameraId;
    std::string intersectionId;
    std::vector<std::string> coveredSegments;
    double latitude{0.0};
    double longitude{0.0};
    double viewingAngleDeg{0.0};
    uint32_t resolutionWidth{1920};
    uint32_t resolutionHeight{1080};
    uint32_t targetFps{30};
    CameraStatus status{CameraStatus::ACTIVE};
    std::string streamUrl;
    bool ptzCapable{false};
    uint64_t lastFrameTimestampMonoMs{0};
    double healthScore{1.0}; // 0.0 to 1.0
};

class CameraRegistry {
public:
    CameraRegistry() = default;
    ~CameraRegistry() = default;

    bool RegisterCamera(const CameraInfo& camera);
    bool UnregisterCamera(const std::string& cameraId);
    bool UpdateCameraStatus(const std::string& cameraId, CameraStatus status);
    bool UpdateCameraHealth(const std::string& cameraId, double healthScore);
    bool RecordFrameReceived(const std::string& cameraId, uint64_t timestampMonoMs = 0);

    std::optional<CameraInfo> GetCamera(const std::string& cameraId) const;
    std::vector<CameraInfo> GetAllCameras() const;
    std::vector<CameraInfo> GetCamerasForSegment(const std::string& roadSegment) const;
    std::vector<CameraInfo> GetCamerasForIntersection(const std::string& intersectionId) const;
    std::vector<CameraInfo> GetCamerasForRoute(const std::vector<std::string>& routeSegments) const;

    size_t GetCameraCount() const;
    size_t GetActiveCameraCount() const;

    bool LoadFromConfigFile(const std::string& configFilePath);
    void Clear();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, CameraInfo> cameras_;
    std::unordered_map<std::string, std::vector<std::string>> segmentToCamerasMap_;
    std::unordered_map<std::string, std::vector<std::string>> intersectionToCamerasMap_;
};

} // namespace aegis::coordinator::perception

