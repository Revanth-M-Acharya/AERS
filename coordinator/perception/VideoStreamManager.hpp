/**
 * @file VideoStreamManager.hpp
 * @brief Video streaming session manager and simulated frame metadata ingestion.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <cstdint>
#include <optional>

namespace aegis::coordinator::perception {

struct VideoFrameMetadata {
    std::string cameraId;
    uint64_t frameIndex{0};
    uint64_t timestampMonoMs{0};
    uint32_t width{1920};
    uint32_t height{1080};
    uint32_t detectedVehicleCount{0};
    double averageVehicleSpeedKmh{0.0};
    bool motionDetected{false};
    std::vector<std::string> rawDetections; // JSON or serialized token strings
};

struct StreamSessionStats {
    std::string cameraId;
    std::string streamUrl;
    uint64_t totalFramesReceived{0};
    uint64_t droppedFrames{0};
    double currentFps{0.0};
    double targetFps{30.0};
    double bitrateKbps{2500.0};
    uint64_t lastFrameMonoMs{0};
    bool isActive{false};
};

class VideoStreamManager {
public:
    VideoStreamManager() = default;
    ~VideoStreamManager() = default;

    bool OpenStream(const std::string& cameraId, const std::string& streamUrl, double targetFps = 30.0);
    bool CloseStream(const std::string& cameraId);
    bool IsStreamActive(const std::string& cameraId) const;

    /**
     * @brief Ingest a video frame metadata packet from an external AI camera pipeline.
     */
    bool IngestFrame(const VideoFrameMetadata& frame);

    /**
     * @brief Generate a simulated synthetic video frame for a given camera (Digital Twin support).
     */
    VideoFrameMetadata GenerateSimulatedFrame(const std::string& cameraId,
                                             uint32_t vehicleCount = 10,
                                             double avgSpeed = 45.0);

    std::optional<StreamSessionStats> GetStreamStats(const std::string& cameraId) const;
    std::vector<StreamSessionStats> GetAllStreamStats() const;
    double GetOverallHealthScore(const std::string& cameraId) const;

    void Reset();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, StreamSessionStats> streams_;
    std::unordered_map<std::string, uint64_t> lastFrameNumbers_;
};

} // namespace aegis::coordinator::perception

