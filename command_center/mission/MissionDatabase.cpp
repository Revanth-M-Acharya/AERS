/**
 * @file MissionDatabase.cpp
 * @brief Implementation of in-memory Mission Database.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "command_center/mission/MissionDatabase.hpp"
#include <numeric>

namespace aegis::command_center {

MissionDatabase::MissionDatabase() {
    InitializeDefaultHistory();
}

void MissionDatabase::InitializeDefaultHistory() {
    std::lock_guard<std::mutex> lock(mutex_);
    missions_.clear();
    chronologicalOrder_.clear();

    // 15 Missions (Average response time: 138.0s = 2m 18s)
    const double times[15] = {
        135.0, 142.0, 130.0, 145.0, 138.0,
        140.0, 134.0, 136.0, 144.0, 137.0,
        139.0, 133.0, 141.0, 136.0, 140.0
    };

    for (int i = 1; i <= 15; ++i) {
        std::string id = "MSN-2026-0" + (i < 10 ? std::string("0") : "") + std::to_string(i);
        HistoricalMissionRecord m;
        m.missionId = id;
        m.emergencyType = (i % 3 == 0) ? "CARDIAC_ARREST" : ((i % 3 == 1) ? "TRAUMA_ACCIDENT" : "ACUTE_STROKE");
        m.priority = (i % 4 == 0) ? "HIGH (Code Yellow)" : "CRITICAL (Code Red)";
        m.assignedAmbulance = "AMB-0" + std::to_string((i % 8) + 1);
        m.assignedHospital = (i % 2 == 0) ? "City Trauma Hospital" : "Metro Heart Institute";
        m.dispatchTimeMonoMs = static_cast<uint64_t>(1000000 + i * 300000);
        m.travelTimeSeconds = times[i - 1];
        m.arrivalTimeMonoMs = m.dispatchTimeMonoMs + static_cast<uint64_t>(m.travelTimeSeconds * 1000);
        m.completionTimeMonoMs = m.arrivalTimeMonoMs + 180000;
        m.routeUsed = {"HOME", "INT_1", "INT_2", "INT_3", "HOSPITAL"};
        m.signalsPreempted = {"INT_1", "INT_2", "INT_3"};
        m.wasReplanned = (i == 7 || i == 12);
        m.replanReason = m.wasReplanned ? "TEMPORARY_POLICE_BARRICADE" : "None";
        m.manualPoliceAdvisory = m.wasReplanned;
        m.outcome = "SUCCESSFUL_PATIENT_HANDOVER";
        m.missionStatus = (i == 15) ? "ACTIVE" : "COMPLETED";

        missions_[id] = m;
        chronologicalOrder_.push_back(id);
    }
}

bool MissionDatabase::CreateMission(const HistoricalMissionRecord& mission) {
    std::lock_guard<std::mutex> lock(mutex_);
    missions_[mission.missionId] = mission;
    chronologicalOrder_.push_back(mission.missionId);
    return true;
}

bool MissionDatabase::RecordReplanning(const std::string& missionId,
                                      const std::vector<std::string>& newRoute,
                                      const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end()) {
        return false;
    }
    it->second.wasReplanned = true;
    it->second.replanReason = reason;
    it->second.routeUsed = newRoute;
    return true;
}

bool MissionDatabase::RecordPoliceAdvisory(const std::string& missionId, const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end()) {
        return false;
    }
    it->second.manualPoliceAdvisory = true;
    it->second.replanReason = reason;
    return true;
}

bool MissionDatabase::CompleteMission(const std::string& missionId,
                                     uint64_t completionTimeMs,
                                     const std::string& outcome) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end()) {
        return false;
    }
    it->second.missionStatus = "COMPLETED";
    it->second.completionTimeMonoMs = completionTimeMs;
    it->second.outcome = outcome;
    if (it->second.dispatchTimeMonoMs > 0 && completionTimeMs >= it->second.dispatchTimeMonoMs) {
        it->second.travelTimeSeconds = static_cast<double>(completionTimeMs - it->second.dispatchTimeMonoMs) / 1000.0;
    }
    return true;
}

std::optional<HistoricalMissionRecord> MissionDatabase::GetMission(const std::string& missionId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<HistoricalMissionRecord> MissionDatabase::GetAllMissions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<HistoricalMissionRecord> results;
    results.reserve(chronologicalOrder_.size());
    for (const auto& id : chronologicalOrder_) {
        auto it = missions_.find(id);
        if (it != missions_.end()) {
            results.push_back(it->second);
        }
    }
    return results;
}

std::vector<HistoricalMissionRecord> MissionDatabase::GetActiveMissions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<HistoricalMissionRecord> results;
    for (const auto& id : chronologicalOrder_) {
        auto it = missions_.find(id);
        if (it != missions_.end() && it->second.missionStatus == "ACTIVE") {
            results.push_back(it->second);
        }
    }
    return results;
}

std::vector<HistoricalMissionRecord> MissionDatabase::GetCompletedMissions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<HistoricalMissionRecord> results;
    for (const auto& id : chronologicalOrder_) {
        auto it = missions_.find(id);
        if (it != missions_.end() && it->second.missionStatus == "COMPLETED") {
            results.push_back(it->second);
        }
    }
    return results;
}

size_t MissionDatabase::GetTotalMissionsCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return missions_.size();
}

double MissionDatabase::GetAverageResponseTimeSec() const {
    std::lock_guard<std::mutex> lock(mutex_);
    double sum = 0.0;
    size_t count = 0;
    for (const auto& [_, m] : missions_) {
        if (m.travelTimeSeconds > 0.0) {
            sum += m.travelTimeSeconds;
            count++;
        }
    }
    return count > 0 ? (sum / static_cast<double>(count)) : 0.0;
}

void MissionDatabase::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    missions_.clear();
    chronologicalOrder_.clear();
}

} // namespace aegis::command_center
