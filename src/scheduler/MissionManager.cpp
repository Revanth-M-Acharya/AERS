/**
 * @file MissionManager.cpp
 * @brief Implementation of MissionManager lifecycle transitions, queries, and serialization.
 */

#include "scheduler/MissionManager.h"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace aegis {
namespace scheduler {

uint64_t MissionManager::CurrentTimeMillis() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

std::string Mission::ToString() const {
    std::ostringstream oss;
    oss << "Mission [" << missionId << "]\n"
        << "  Ambulance   : " << ambulanceId << "\n"
        << "  Patient     : " << patientId << "\n"
        << "  State       : " << MissionStateToString(state) << "\n"
        << "  Priority    : " << priorityScore << "\n"
        << "  Corridor    : " << (corridorId.empty() ? "None" : corridorId) << "\n"
        << "  Destination : " << (destinationHospital.empty() ? "None" : destinationHospital) << "\n"
        << "  Route Stops : " << route.size() << "\n"
        << "  Reason      : " << (reason.empty() ? "None" : reason);
    return oss.str();
}

std::string Mission::ToJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"mission_id\":\"" << missionId << "\","
        << "\"ambulance_id\":\"" << ambulanceId << "\","
        << "\"patient_id\":\"" << patientId << "\","
        << "\"state\":\"" << MissionStateToString(state) << "\","
        << "\"priority_score\":" << std::fixed << std::setprecision(2) << priorityScore << ","
        << "\"corridor_id\":\"" << corridorId << "\","
        << "\"destination_hospital\":\"" << destinationHospital << "\","
        << "\"description\":\"" << description << "\","
        << "\"reason\":\"" << reason << "\","
        << "\"start_time_epoch_ms\":" << startTimeEpochMs << ","
        << "\"end_time_epoch_ms\":" << endTimeEpochMs << ","
        << "\"route\":[";

    for (size_t i = 0; i < route.size(); ++i) {
        oss << "\"" << route[i] << "\"";
        if (i + 1 < route.size()) {
            oss << ",";
        }
    }
    oss << "]}";
    return oss.str();
}

bool MissionManager::RegisterMission(const Mission& mission) {
    if (mission.missionId.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_missions[mission.missionId] = mission;
    if (m_missions[mission.missionId].startTimeEpochMs == 0 &&
        mission.state == MissionState::ACTIVE) {
        m_missions[mission.missionId].startTimeEpochMs = CurrentTimeMillis();
    }
    return true;
}

bool MissionManager::CreateMission(std::string missionId,
                                   std::string ambulanceId,
                                   std::string patientId,
                                   std::vector<std::string> route,
                                   std::string destinationHospital,
                                   double priorityScore,
                                   std::string corridorId,
                                   std::string description) {
    if (missionId.empty()) {
        return false;
    }
    Mission mission;
    mission.missionId = std::move(missionId);
    mission.ambulanceId = std::move(ambulanceId);
    mission.patientId = std::move(patientId);
    mission.route = std::move(route);
    mission.destinationHospital = std::move(destinationHospital);
    mission.priorityScore = priorityScore;
    mission.corridorId = std::move(corridorId);
    mission.description = std::move(description);
    mission.state = MissionState::ACTIVE;
    mission.startTimeEpochMs = CurrentTimeMillis();
    mission.endTimeEpochMs = 0;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_missions[mission.missionId] = std::move(mission);
    return true;
}

bool MissionManager::StartMission(const std::string& missionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_missions.find(missionId);
    if (it == m_missions.end()) {
        return false;
    }
    it->second.state = MissionState::ACTIVE;
    if (it->second.startTimeEpochMs == 0) {
        it->second.startTimeEpochMs = CurrentTimeMillis();
    }
    it->second.reason = "";
    return true;
}

bool MissionManager::SetMissionWaiting(const std::string& missionId, const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_missions.find(missionId);
    if (it == m_missions.end()) {
        return false;
    }
    it->second.state = MissionState::WAITING;
    it->second.reason = reason;
    return true;
}

bool MissionManager::PauseMission(const std::string& missionId, const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_missions.find(missionId);
    if (it == m_missions.end()) {
        return false;
    }
    it->second.state = MissionState::PAUSED;
    it->second.reason = reason;
    return true;
}

bool MissionManager::ResumeMission(const std::string& missionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_missions.find(missionId);
    if (it == m_missions.end()) {
        return false;
    }
    it->second.state = MissionState::ACTIVE;
    it->second.reason = "";
    return true;
}

bool MissionManager::CompleteMission(const std::string& missionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_missions.find(missionId);
    if (it == m_missions.end()) {
        return false;
    }
    it->second.state = MissionState::COMPLETED;
    it->second.endTimeEpochMs = CurrentTimeMillis();
    return true;
}

bool MissionManager::CancelMission(const std::string& missionId, const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_missions.find(missionId);
    if (it == m_missions.end()) {
        return false;
    }
    it->second.state = MissionState::CANCELLED;
    it->second.reason = reason;
    it->second.endTimeEpochMs = CurrentTimeMillis();
    return true;
}

bool MissionManager::UpdateMissionRoute(const std::string& missionId, const std::vector<std::string>& route) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_missions.find(missionId);
    if (it == m_missions.end()) {
        return false;
    }
    it->second.route = route;
    return true;
}

bool MissionManager::UpdateMissionCorridor(const std::string& missionId, const std::string& corridorId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_missions.find(missionId);
    if (it == m_missions.end()) {
        return false;
    }
    it->second.corridorId = corridorId;
    return true;
}

bool MissionManager::UpdatePriorityScore(const std::string& missionId, double score) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_missions.find(missionId);
    if (it == m_missions.end()) {
        return false;
    }
    it->second.priorityScore = score;
    return true;
}

std::optional<Mission> MissionManager::GetMission(const std::string& missionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_missions.find(missionId);
    if (it != m_missions.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<Mission> MissionManager::GetMissionForAmbulance(const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& [id, mission] : m_missions) {
        if (mission.ambulanceId == ambulanceId) {
            return mission;
        }
    }
    return std::nullopt;
}

std::vector<Mission> MissionManager::GetActiveMissions() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Mission> result;
    for (const auto& [id, mission] : m_missions) {
        if (mission.state == MissionState::ACTIVE) {
            result.push_back(mission);
        }
    }
    return result;
}

std::vector<Mission> MissionManager::GetMissionsByState(MissionState state) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Mission> result;
    for (const auto& [id, mission] : m_missions) {
        if (mission.state == state) {
            result.push_back(mission);
        }
    }
    return result;
}

std::vector<Mission> MissionManager::GetAllMissions() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Mission> result;
    result.reserve(m_missions.size());
    for (const auto& [id, mission] : m_missions) {
        result.push_back(mission);
    }
    return result;
}

bool MissionManager::HasMission(const std::string& missionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_missions.find(missionId) != m_missions.end();
}

size_t MissionManager::GetMissionCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_missions.size();
}

void MissionManager::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_missions.clear();
}

} // namespace scheduler
} // namespace aegis
