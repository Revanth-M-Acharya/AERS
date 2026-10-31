/**
 * @file MissionReplayEngine.cpp
 * @brief Implementation of MissionReplayEngine VCR-style historical player.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "digital_twin/MissionReplayEngine.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace aegis::digital_twin {

std::string ReplayStateToString(ReplayState state) {
    switch (state) {
        case ReplayState::STOPPED: return "STOPPED";
        case ReplayState::PLAYING: return "PLAYING";
        case ReplayState::PAUSED:  return "PAUSED";
    }
    return "UNKNOWN";
}

MissionReplayEngine::MissionReplayEngine() {
    ClearRecordings();
}

void MissionReplayEngine::RecordFrame(
    uint64_t timestampMonoMs,
    double simTimeSec,
    const CityModel& city,
    const std::string& emergencyId,
    const std::string& desc) {

    std::lock_guard<std::mutex> lock(mutex_);
    DigitalTwinFrame f;
    f.frameIndex = frames_.size();
    f.timestampMonoMs = timestampMonoMs;
    f.simTimeSeconds = simTimeSec;
    f.activeEmergencyId = emergencyId;
    f.intersections = city.GetAllIntersections();
    f.roadSegments = city.GetAllRoadSegments();
    f.ambulances = city.GetAllAmbulances();
    f.hospitals = city.GetAllHospitals();
    f.roadConditions = city.GetAllRoadConditions();
    f.frameDescription = desc;

    frames_.push_back(f);
    // Keep up to 1000 frames in circular history
    if (frames_.size() > 1000) {
        frames_.erase(frames_.begin());
        for (size_t i = 0; i < frames_.size(); ++i) {
            frames_[i].frameIndex = i;
        }
    }
}

void MissionReplayEngine::ClearRecordings() {
    std::lock_guard<std::mutex> lock(mutex_);
    frames_.clear();
    currentFrameIndex_ = 0;
    state_ = ReplayState::STOPPED;
    speedMultiplier_ = 1.0;
}

size_t MissionReplayEngine::GetTotalFrames() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return frames_.size();
}

void MissionReplayEngine::Play() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!frames_.empty()) {
        state_ = ReplayState::PLAYING;
    }
}

void MissionReplayEngine::Pause() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == ReplayState::PLAYING) {
        state_ = ReplayState::PAUSED;
    }
}

void MissionReplayEngine::Resume() {
    Play();
}

void MissionReplayEngine::Stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    state_ = ReplayState::STOPPED;
    currentFrameIndex_ = 0;
}

void MissionReplayEngine::Rewind() {
    std::lock_guard<std::mutex> lock(mutex_);
    currentFrameIndex_ = 0;
}

bool MissionReplayEngine::StepForward() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (frames_.empty()) return false;
    if (currentFrameIndex_ + 1 < frames_.size()) {
        currentFrameIndex_++;
        return true;
    }
    return false;
}

bool MissionReplayEngine::StepBackward() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (frames_.empty() || currentFrameIndex_ == 0) return false;
    currentFrameIndex_--;
    return true;
}

bool MissionReplayEngine::SeekToFrame(size_t frameIndex) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (frameIndex < frames_.size()) {
        currentFrameIndex_ = frameIndex;
        return true;
    }
    return false;
}

bool MissionReplayEngine::SeekToTimestamp(uint64_t timestampMonoMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (frames_.empty()) return false;

    auto it = std::lower_bound(frames_.begin(), frames_.end(), timestampMonoMs,
        [](const DigitalTwinFrame& f, uint64_t ts) {
            return f.timestampMonoMs < ts;
        });

    if (it != frames_.end()) {
        currentFrameIndex_ = static_cast<size_t>(std::distance(frames_.begin(), it));
        return true;
    }
    currentFrameIndex_ = frames_.size() - 1;
    return true;
}

void MissionReplayEngine::SetPlaybackSpeed(double speedMultiplier) {
    std::lock_guard<std::mutex> lock(mutex_);
    speedMultiplier_ = std::clamp(speedMultiplier, 0.1, 20.0);
}

double MissionReplayEngine::GetPlaybackSpeed() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return speedMultiplier_;
}

ReplayState MissionReplayEngine::GetState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

size_t MissionReplayEngine::GetCurrentFrameIndex() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return currentFrameIndex_;
}

DigitalTwinFrame MissionReplayEngine::GetCurrentFrame() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (currentFrameIndex_ < frames_.size()) {
        return frames_[currentFrameIndex_];
    }
    return DigitalTwinFrame{};
}

std::vector<DigitalTwinFrame> MissionReplayEngine::FilterByMission(const std::string& emergencyId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<DigitalTwinFrame> list;
    for (const auto& f : frames_) {
        if (f.activeEmergencyId == emergencyId) list.push_back(f);
    }
    return list;
}

std::string MissionReplayEngine::ExportArchiveJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream ss;
    ss << "{\"totalFrames\":" << frames_.size()
       << ",\"speed\":" << std::fixed << std::setprecision(1) << speedMultiplier_
       << ",\"frames\":[";
    for (size_t i = 0; i < frames_.size(); ++i) {
        const auto& f = frames_[i];
        ss << "{\"frame\":" << f.frameIndex
           << ",\"ts\":" << f.timestampMonoMs
           << ",\"simSec\":" << std::fixed << std::setprecision(2) << f.simTimeSeconds
           << ",\"emg\":\"" << f.activeEmergencyId << "\""
           << ",\"desc\":\"" << f.frameDescription << "\"}"
           << (i + 1 < frames_.size() ? "," : "");
    }
    ss << "]}";
    return ss.str();
}

} // namespace aegis::digital_twin

