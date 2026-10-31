/**
 * @file CameraRegistry.cpp
 * @brief Implementation of CameraRegistry directory management and config parser.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/perception/CameraRegistry.hpp"
#include "common/utils/TimeUtils.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace aegis::coordinator::perception {

std::string CameraStatusToString(CameraStatus status) {
    switch (status) {
        case CameraStatus::OFFLINE:     return "OFFLINE";
        case CameraStatus::ACTIVE:      return "ACTIVE";
        case CameraStatus::STREAMING:   return "STREAMING";
        case CameraStatus::DEGRADED:    return "DEGRADED";
        case CameraStatus::CALIBRATING: return "CALIBRATING";
        default:                        return "UNKNOWN";
    }
}

CameraStatus StringToCameraStatus(const std::string& str) {
    if (str == "OFFLINE")     return CameraStatus::OFFLINE;
    if (str == "ACTIVE")      return CameraStatus::ACTIVE;
    if (str == "STREAMING")   return CameraStatus::STREAMING;
    if (str == "DEGRADED")    return CameraStatus::DEGRADED;
    if (str == "CALIBRATING") return CameraStatus::CALIBRATING;
    return CameraStatus::OFFLINE;
}

bool CameraRegistry::RegisterCamera(const CameraInfo& camera) {
    if (camera.cameraId.empty()) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    if (cameras_.find(camera.cameraId) != cameras_.end()) {
        return false; // Already registered
    }

    cameras_[camera.cameraId] = camera;

    // Index by segments
    for (const auto& seg : camera.coveredSegments) {
        segmentToCamerasMap_[seg].push_back(camera.cameraId);
    }

    // Index by intersection
    if (!camera.intersectionId.empty()) {
        intersectionToCamerasMap_[camera.intersectionId].push_back(camera.cameraId);
    }

    return true;
}

bool CameraRegistry::UnregisterCamera(const std::string& cameraId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cameras_.find(cameraId);
    if (it == cameras_.end()) return false;

    // Clean segment index
    for (const auto& seg : it->second.coveredSegments) {
        auto segIt = segmentToCamerasMap_.find(seg);
        if (segIt != segmentToCamerasMap_.end()) {
            auto& vec = segIt->second;
            vec.erase(std::remove(vec.begin(), vec.end(), cameraId), vec.end());
        }
    }

    // Clean intersection index
    if (!it->second.intersectionId.empty()) {
        auto intIt = intersectionToCamerasMap_.find(it->second.intersectionId);
        if (intIt != intersectionToCamerasMap_.end()) {
            auto& vec = intIt->second;
            vec.erase(std::remove(vec.begin(), vec.end(), cameraId), vec.end());
        }
    }

    cameras_.erase(it);
    return true;
}

bool CameraRegistry::UpdateCameraStatus(const std::string& cameraId, CameraStatus status) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cameras_.find(cameraId);
    if (it == cameras_.end()) return false;
    it->second.status = status;
    return true;
}

bool CameraRegistry::UpdateCameraHealth(const std::string& cameraId, double healthScore) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cameras_.find(cameraId);
    if (it == cameras_.end()) return false;
    it->second.healthScore = std::clamp(healthScore, 0.0, 1.0);
    if (healthScore < 0.20 && it->second.status != CameraStatus::OFFLINE) {
        it->second.status = CameraStatus::DEGRADED;
    }
    return true;
}

bool CameraRegistry::RecordFrameReceived(const std::string& cameraId, uint64_t timestampMonoMs) {
    if (timestampMonoMs == 0) timestampMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cameras_.find(cameraId);
    if (it == cameras_.end()) return false;
    it->second.lastFrameTimestampMonoMs = timestampMonoMs;
    return true;
}

std::optional<CameraInfo> CameraRegistry::GetCamera(const std::string& cameraId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cameras_.find(cameraId);
    if (it != cameras_.end()) return it->second;
    return std::nullopt;
}

std::vector<CameraInfo> CameraRegistry::GetAllCameras() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<CameraInfo> result;
    result.reserve(cameras_.size());
    for (const auto& [_, cam] : cameras_) {
        result.push_back(cam);
    }
    return result;
}

std::vector<CameraInfo> CameraRegistry::GetCamerasForSegment(const std::string& roadSegment) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<CameraInfo> result;
    auto it = segmentToCamerasMap_.find(roadSegment);
    if (it != segmentToCamerasMap_.end()) {
        for (const auto& camId : it->second) {
            auto camIt = cameras_.find(camId);
            if (camIt != cameras_.end()) {
                result.push_back(camIt->second);
            }
        }
    }
    return result;
}

std::vector<CameraInfo> CameraRegistry::GetCamerasForIntersection(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<CameraInfo> result;
    auto it = intersectionToCamerasMap_.find(intersectionId);
    if (it != intersectionToCamerasMap_.end()) {
        for (const auto& camId : it->second) {
            auto camIt = cameras_.find(camId);
            if (camIt != cameras_.end()) {
                result.push_back(camIt->second);
            }
        }
    }
    return result;
}

std::vector<CameraInfo> CameraRegistry::GetCamerasForRoute(const std::vector<std::string>& routeSegments) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<CameraInfo> result;
    std::unordered_map<std::string, bool> added;

    for (const auto& seg : routeSegments) {
        auto it = segmentToCamerasMap_.find(seg);
        if (it != segmentToCamerasMap_.end()) {
            for (const auto& camId : it->second) {
                if (!added[camId]) {
                    auto camIt = cameras_.find(camId);
                    if (camIt != cameras_.end()) {
                        result.push_back(camIt->second);
                        added[camId] = true;
                    }
                }
            }
        }
    }
    return result;
}

size_t CameraRegistry::GetCameraCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cameras_.size();
}

size_t CameraRegistry::GetActiveCameraCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [_, cam] : cameras_) {
        if (cam.status == CameraStatus::ACTIVE || cam.status == CameraStatus::STREAMING) {
            count++;
        }
    }
    return count;
}

static std::string Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

bool CameraRegistry::LoadFromConfigFile(const std::string& configFilePath) {
    std::ifstream file(configFilePath);
    if (!file.is_open()) return false;

    std::string line;
    CameraInfo currentCamera;
    bool inSection = false;

    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        if (line.front() == '[' && line.back() == ']') {
            if (inSection && !currentCamera.cameraId.empty()) {
                RegisterCamera(currentCamera);
            }
            currentCamera = CameraInfo();
            currentCamera.cameraId = line.substr(1, line.length() - 2);
            inSection = true;
            continue;
        }

        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = Trim(line.substr(0, eqPos));
        std::string val = Trim(line.substr(eqPos + 1));

        if (key == "intersection_id") {
            currentCamera.intersectionId = val;
        } else if (key == "covered_segments") {
            std::stringstream ss(val);
            std::string item;
            while (std::getline(ss, item, ',')) {
                item = Trim(item);
                if (!item.empty()) currentCamera.coveredSegments.push_back(item);
            }
        } else if (key == "latitude") {
            currentCamera.latitude = std::stod(val);
        } else if (key == "longitude") {
            currentCamera.longitude = std::stod(val);
        } else if (key == "viewing_angle_deg") {
            currentCamera.viewingAngleDeg = std::stod(val);
        } else if (key == "target_fps") {
            currentCamera.targetFps = static_cast<uint32_t>(std::stoul(val));
        } else if (key == "status") {
            currentCamera.status = StringToCameraStatus(val);
        } else if (key == "stream_url") {
            currentCamera.streamUrl = val;
        } else if (key == "ptz_capable") {
            currentCamera.ptzCapable = (val == "true" || val == "1");
        }
    }

    if (inSection && !currentCamera.cameraId.empty()) {
        RegisterCamera(currentCamera);
    }

    return true;
}

void CameraRegistry::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    cameras_.clear();
    segmentToCamerasMap_.clear();
    intersectionToCamerasMap_.clear();
}

} // namespace aegis::coordinator::perception

