/**
 * @file VideoStreamManager.cpp
 * @brief Implementation of VideoStreamManager session tracking and synthetic frame generator.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/perception/VideoStreamManager.hpp"
#include "common/utils/TimeUtils.hpp"
#include <algorithm>

namespace aegis::coordinator::perception {

bool VideoStreamManager::OpenStream(const std::string& cameraId, const std::string& streamUrl, double targetFps) {
    if (cameraId.empty()) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    StreamSessionStats stats;
    stats.cameraId = cameraId;
    stats.streamUrl = streamUrl;
    stats.targetFps = (targetFps > 0.0) ? targetFps : 30.0;
    stats.currentFps = stats.targetFps;
    stats.isActive = true;
    stats.lastFrameMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    streams_[cameraId] = stats;
    lastFrameNumbers_[cameraId] = 0;
    return true;
}

bool VideoStreamManager::CloseStream(const std::string& cameraId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = streams_.find(cameraId);
    if (it == streams_.end()) return false;
    it->second.isActive = false;
    return true;
}

bool VideoStreamManager::IsStreamActive(const std::string& cameraId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = streams_.find(cameraId);
    return (it != streams_.end() && it->second.isActive);
}

bool VideoStreamManager::IngestFrame(const VideoFrameMetadata& frame) {
    if (frame.cameraId.empty()) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = streams_.find(frame.cameraId);
    if (it == streams_.end()) {
        // Auto-register session
        StreamSessionStats stats;
        stats.cameraId = frame.cameraId;
        stats.isActive = true;
        stats.targetFps = 30.0;
        stats.currentFps = 30.0;
        streams_[frame.cameraId] = stats;
        it = streams_.find(frame.cameraId);
    }

    auto& stats = it->second;
    uint64_t lastIdx = lastFrameNumbers_[frame.cameraId];

    if (frame.frameIndex > lastIdx + 1 && lastIdx > 0) {
        stats.droppedFrames += (frame.frameIndex - lastIdx - 1);
    }
    lastFrameNumbers_[frame.cameraId] = frame.frameIndex;

    uint64_t now = frame.timestampMonoMs;
    if (now == 0) now = common::TimeUtils::GetMonotonicTimeMs();

    if (stats.lastFrameMonoMs > 0 && now > stats.lastFrameMonoMs) {
        double deltaSec = static_cast<double>(now - stats.lastFrameMonoMs) / 1000.0;
        if (deltaSec > 0.0) {
            double instantFps = 1.0 / deltaSec;
            stats.currentFps = 0.8 * stats.currentFps + 0.2 * instantFps;
        }
    }

    stats.totalFramesReceived++;
    stats.lastFrameMonoMs = now;
    return true;
}

VideoFrameMetadata VideoStreamManager::GenerateSimulatedFrame(const std::string& cameraId,
                                                             uint32_t vehicleCount,
                                                             double avgSpeed) {
    uint64_t now = common::TimeUtils::GetMonotonicTimeMs();
    uint64_t frameIdx = 1;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = lastFrameNumbers_.find(cameraId);
        if (it != lastFrameNumbers_.end()) {
            frameIdx = it->second + 1;
        }
    }

    VideoFrameMetadata frame;
    frame.cameraId = cameraId;
    frame.frameIndex = frameIdx;
    frame.timestampMonoMs = now;
    frame.width = 1920;
    frame.height = 1080;
    frame.detectedVehicleCount = vehicleCount;
    frame.averageVehicleSpeedKmh = avgSpeed;
    frame.motionDetected = (vehicleCount > 0);

    IngestFrame(frame);
    return frame;
}

std::optional<StreamSessionStats> VideoStreamManager::GetStreamStats(const std::string& cameraId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = streams_.find(cameraId);
    if (it != streams_.end()) return it->second;
    return std::nullopt;
}

std::vector<StreamSessionStats> VideoStreamManager::GetAllStreamStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<StreamSessionStats> list;
    list.reserve(streams_.size());
    for (const auto& [_, s] : streams_) {
        list.push_back(s);
    }
    return list;
}

double VideoStreamManager::GetOverallHealthScore(const std::string& cameraId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = streams_.find(cameraId);
    if (it == streams_.end() || !it->second.isActive) return 0.0;

    const auto& s = it->second;
    if (s.totalFramesReceived == 0) return 1.0;

    double dropRatio = static_cast<double>(s.droppedFrames) /
                       static_cast<double>(s.totalFramesReceived + s.droppedFrames);
    double fpsRatio = (s.targetFps > 0.0) ? (s.currentFps / s.targetFps) : 1.0;
    fpsRatio = std::clamp(fpsRatio, 0.0, 1.0);

    double health = 0.6 * (1.0 - dropRatio) + 0.4 * fpsRatio;
    return std::clamp(health, 0.0, 1.0);
}

void VideoStreamManager::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    streams_.clear();
    lastFrameNumbers_.clear();
}

} // namespace aegis::coordinator::perception

