/**
 * @file MissionReplayEngine.hpp
 * @brief Time-travel replay engine supporting pause, step, speed scaling, and mission archive review.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "digital_twin/CityModel.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

namespace aegis::digital_twin {

enum class ReplayState {
    STOPPED,
    PLAYING,
    PAUSED
};

std::string ReplayStateToString(ReplayState state);

struct DigitalTwinFrame {
    uint64_t frameIndex{0};
    uint64_t timestampMonoMs{0};
    double simTimeSeconds{0.0};
    std::string activeEmergencyId;
    std::vector<IntersectionNode> intersections;
    std::vector<RoadSegment> roadSegments;
    std::vector<AmbulanceEntity> ambulances;
    std::vector<HospitalFacility> hospitals;
    std::vector<RoadConditionEntity> roadConditions;
    std::string frameDescription;
};

class MissionReplayEngine {
public:
    MissionReplayEngine();
    ~MissionReplayEngine() = default;

    // Recording
    void RecordFrame(uint64_t timestampMonoMs, double simTimeSec, const CityModel& city, const std::string& emergencyId = "", const std::string& desc = "");
    void ClearRecordings();
    size_t GetTotalFrames() const;

    // Playback Controls
    void Play();
    void Pause();
    void Resume();
    void Stop();
    void Rewind();

    bool StepForward();
    bool StepBackward();
    bool SeekToFrame(size_t frameIndex);
    bool SeekToTimestamp(uint64_t timestampMonoMs);

    void SetPlaybackSpeed(double speedMultiplier);
    double GetPlaybackSpeed() const;

    ReplayState GetState() const;
    size_t GetCurrentFrameIndex() const;
    DigitalTwinFrame GetCurrentFrame() const;

    // Filtering & Export
    std::vector<DigitalTwinFrame> FilterByMission(const std::string& emergencyId) const;
    std::string ExportArchiveJson() const;

private:
    mutable std::mutex mutex_;
    std::vector<DigitalTwinFrame> frames_;
    size_t currentFrameIndex_{0};
    ReplayState state_{ReplayState::STOPPED};
    double speedMultiplier_{1.0};
};

} // namespace aegis::digital_twin

