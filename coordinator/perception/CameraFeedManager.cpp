/**
 * @file CameraFeedManager.cpp
 * @brief Implementation of CameraFeedManager dynamic corridor camera activation.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/perception/CameraFeedManager.hpp"
#include <algorithm>

namespace aegis::coordinator::perception {

CameraFeedManager::CameraFeedManager(std::shared_ptr<CameraRegistry> registry)
    : registry_(std::move(registry)) {}

std::vector<std::string> CameraFeedManager::ActivateFeedsForCorridor(
    const std::string& corridorId,
    const std::vector<std::string>& routeSegments) {

    if (corridorId.empty() || routeSegments.empty() || !registry_) {
        return {};
    }

    auto cameras = registry_->GetCamerasForRoute(routeSegments);
    std::vector<std::string> activated;

    std::lock_guard<std::mutex> lock(mutex_);

    // If corridor was already registered, clear its previous cameras first
    auto it = corridorCameraMap_.find(corridorId);
    if (it != corridorCameraMap_.end()) {
        for (const auto& camId : it->second) {
            auto refIt = cameraRefCounts_.find(camId);
            if (refIt != cameraRefCounts_.end()) {
                if (--(refIt->second) == 0) {
                    cameraRefCounts_.erase(refIt);
                    registry_->UpdateCameraStatus(camId, CameraStatus::ACTIVE);
                }
            }
        }
        corridorCameraMap_.erase(it);
    }

    std::vector<std::string> corridorCams;
    for (const auto& cam : cameras) {
        if (cam.status != CameraStatus::OFFLINE) {
            corridorCams.push_back(cam.cameraId);
            activated.push_back(cam.cameraId);

            uint32_t count = ++cameraRefCounts_[cam.cameraId];
            if (count == 1) {
                registry_->UpdateCameraStatus(cam.cameraId, CameraStatus::STREAMING);
            }
        }
    }

    corridorCameraMap_[corridorId] = corridorCams;
    return activated;
}

bool CameraFeedManager::DeactivateFeedsForCorridor(const std::string& corridorId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = corridorCameraMap_.find(corridorId);
    if (it == corridorCameraMap_.end()) return false;

    for (const auto& camId : it->second) {
        auto refIt = cameraRefCounts_.find(camId);
        if (refIt != cameraRefCounts_.end()) {
            if (--(refIt->second) == 0) {
                cameraRefCounts_.erase(refIt);
                if (registry_) {
                    registry_->UpdateCameraStatus(camId, CameraStatus::ACTIVE);
                }
            }
        }
    }

    corridorCameraMap_.erase(it);
    return true;
}

bool CameraFeedManager::IsCameraActive(const std::string& cameraId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cameraRefCounts_.find(cameraId);
    return (it != cameraRefCounts_.end() && it->second > 0);
}

std::vector<std::string> CameraFeedManager::GetActiveCameraIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> active;
    active.reserve(cameraRefCounts_.size());
    for (const auto& [camId, count] : cameraRefCounts_) {
        if (count > 0) active.push_back(camId);
    }
    return active;
}

std::vector<std::string> CameraFeedManager::GetCorridorCameras(const std::string& corridorId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = corridorCameraMap_.find(corridorId);
    if (it != corridorCameraMap_.end()) return it->second;
    return {};
}

size_t CameraFeedManager::GetActiveFeedCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cameraRefCounts_.size();
}

void CameraFeedManager::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (registry_) {
        for (const auto& [camId, _] : cameraRefCounts_) {
            registry_->UpdateCameraStatus(camId, CameraStatus::ACTIVE);
        }
    }
    corridorCameraMap_.clear();
    cameraRefCounts_.clear();
}

} // namespace aegis::coordinator::perception

