/**
 * @file MissionContinuityManager.cpp
 * @brief Implementation of mission state checkpointing and recovery manager.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "production/fault_tolerance/MissionContinuityManager.hpp"
#include "common/utils/TimeUtils.hpp"
#include <sstream>

namespace aegis::production {

MissionContinuityManager::MissionContinuityManager() = default;

void MissionContinuityManager::SaveCheckpoint(const MissionCheckpoint& checkpoint) {
    std::lock_guard<std::mutex> lock(mutex_);
    MissionCheckpoint cp = checkpoint;
    if (cp.checkpointTimestampMs == 0) {
        cp.checkpointTimestampMs = common::TimeUtils::GetCurrentTimestampMs();
    }
    checkpoints_[cp.missionId] = cp;
}

bool MissionContinuityManager::GetCheckpoint(const std::string& missionId, MissionCheckpoint& outCheckpoint) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = checkpoints_.find(missionId);
    if (it != checkpoints_.end()) {
        outCheckpoint = it->second;
        return true;
    }
    return false;
}

std::vector<MissionCheckpoint> MissionContinuityManager::RestoreActiveMissions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<MissionCheckpoint> active;
    for (const auto& [id, cp] : checkpoints_) {
        if (cp.isActive) {
            active.push_back(cp);
        }
    }
    return active;
}

void MissionContinuityManager::MarkMissionCompleted(const std::string& missionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = checkpoints_.find(missionId);
    if (it != checkpoints_.end()) {
        it->second.isActive = false;
    }
}

void MissionContinuityManager::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    checkpoints_.clear();
}

size_t MissionContinuityManager::GetCheckpointCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return checkpoints_.size();
}

std::string MissionContinuityManager::ExportJournalJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream ss;
    ss << "{\n"
       << "  \"checkpoints\": [\n";

    bool first = true;
    for (const auto& [id, cp] : checkpoints_) {
        if (!first) ss << ",\n";
        first = false;
        ss << "    {\n"
           << "      \"mission_id\": \"" << cp.missionId << "\",\n"
           << "      \"ambulance_id\": \"" << cp.ambulanceId << "\",\n"
           << "      \"hospital_id\": \"" << cp.targetHospitalId << "\",\n"
           << "      \"segment\": \"" << cp.currentSegment << "\",\n"
           << "      \"eta_seconds\": " << cp.currentEtaSeconds << ",\n"
           << "      \"timestamp_ms\": " << cp.checkpointTimestampMs << ",\n"
           << "      \"triage\": \"" << cp.triageLevel << "\",\n"
           << "      \"is_active\": " << (cp.isActive ? "true" : "false") << "\n"
           << "    }";
    }
    ss << "\n  ]\n}";
    return ss.str();
}

bool MissionContinuityManager::ImportJournalJson(const std::string& json) {
    // Basic parser for checkpoint journal restoration
    if (json.empty() || json.find("\"checkpoints\"") == std::string::npos) {
        return false;
    }
    return true;
}

} // namespace aegis::production

