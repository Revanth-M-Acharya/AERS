/**
 * @file CameraFeedManager.hpp
 * @brief Manages dynamic camera feed subscriptions and activation for active emergency corridors.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/perception/CameraRegistry.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>

namespace aegis::coordinator::perception {

class CameraFeedManager {
public:
    explicit CameraFeedManager(std::shared_ptr<CameraRegistry> registry);
    ~CameraFeedManager() = default;

    /**
     * @brief Activate streaming feeds for all cameras covering the given route segments.
     * @param corridorId Unique identifier of the emergency corridor session.
     * @param routeSegments List of road segments along the planned ambulance path.
     * @return List of camera IDs successfully activated.
     */
    std::vector<std::string> ActivateFeedsForCorridor(const std::string& corridorId,
                                                      const std::vector<std::string>& routeSegments);

    /**
     * @brief Deactivate camera feeds associated with a completed or cancelled corridor.
     */
    bool DeactivateFeedsForCorridor(const std::string& corridorId);

    /**
     * @brief Check whether a specific camera feed is currently active and streaming.
     */
    bool IsCameraActive(const std::string& cameraId) const;

    /**
     * @brief Get all camera IDs currently in active streaming mode across all corridors.
     */
    std::vector<std::string> GetActiveCameraIds() const;

    /**
     * @brief Get camera IDs mapped to a specific corridor ID.
     */
    std::vector<std::string> GetCorridorCameras(const std::string& corridorId) const;

    size_t GetActiveFeedCount() const;
    void Reset();

private:
    std::shared_ptr<CameraRegistry> registry_;
    mutable std::mutex mutex_;

    // corridorId -> list of camera IDs
    std::unordered_map<std::string, std::vector<std::string>> corridorCameraMap_;

    // cameraId -> reference count (how many corridors currently require this feed)
    std::unordered_map<std::string, uint32_t> cameraRefCounts_;
};

} // namespace aegis::coordinator::perception

