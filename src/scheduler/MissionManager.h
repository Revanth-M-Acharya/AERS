/**
 * @file MissionManager.h
 * @brief Mission lifecycle manager and tracking for SMART AEGIS.
 */

#ifndef AEGIS_SCHEDULER_MISSION_MANAGER_H
#define AEGIS_SCHEDULER_MISSION_MANAGER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <cstdint>

namespace aegis {
namespace scheduler {

/**
 * @brief Discrete lifecycle states for an active ambulance mission.
 */
enum class MissionState {
    ACTIVE,
    WAITING,
    COMPLETED,
    CANCELLED,
    PAUSED
};

inline std::string MissionStateToString(MissionState state) {
    switch (state) {
        case MissionState::ACTIVE:    return "ACTIVE";
        case MissionState::WAITING:   return "WAITING";
        case MissionState::COMPLETED: return "COMPLETED";
        case MissionState::CANCELLED: return "CANCELLED";
        case MissionState::PAUSED:    return "PAUSED";
        default:                      return "UNKNOWN";
    }
}

inline MissionState StringToMissionState(const std::string& str) {
    if (str == "ACTIVE" || str == "active") return MissionState::ACTIVE;
    if (str == "WAITING" || str == "waiting") return MissionState::WAITING;
    if (str == "COMPLETED" || str == "completed") return MissionState::COMPLETED;
    if (str == "CANCELLED" || str == "cancelled") return MissionState::CANCELLED;
    if (str == "PAUSED" || str == "paused") return MissionState::PAUSED;
    return MissionState::ACTIVE;
}

/**
 * @brief Comprehensive data model representing an emergency response mission.
 */
struct Mission {
    std::string missionId{""};
    std::string ambulanceId{""};
    std::string patientId{""};
    std::vector<std::string> route{};
    std::string destinationHospital{""};
    MissionState state{MissionState::ACTIVE};
    double priorityScore{0.0};
    std::string corridorId{""};
    std::string description{""};
    std::string reason{""};
    uint64_t startTimeEpochMs{0};
    uint64_t endTimeEpochMs{0};

    bool IsActive() const noexcept { return state == MissionState::ACTIVE; }
    std::string ToString() const;
    std::string ToJson() const;
};

/**
 * @brief Thread-safe manager governing mission creation, state transitions, and auditing.
 */
class MissionManager {
public:
    MissionManager() = default;
    ~MissionManager() = default;

    // Creation & Registration
    bool RegisterMission(const Mission& mission);
    bool CreateMission(std::string missionId,
                       std::string ambulanceId,
                       std::string patientId,
                       std::vector<std::string> route,
                       std::string destinationHospital = "",
                       double priorityScore = 0.0,
                       std::string corridorId = "",
                       std::string description = "");

    // Lifecycle State Transitions
    bool StartMission(const std::string& missionId);
    bool SetMissionWaiting(const std::string& missionId, const std::string& reason = "");
    bool PauseMission(const std::string& missionId, const std::string& reason = "");
    bool ResumeMission(const std::string& missionId);
    bool CompleteMission(const std::string& missionId);
    bool CancelMission(const std::string& missionId, const std::string& reason = "");

    // Mutators
    bool UpdateMissionRoute(const std::string& missionId, const std::vector<std::string>& route);
    bool UpdateMissionCorridor(const std::string& missionId, const std::string& corridorId);
    bool UpdatePriorityScore(const std::string& missionId, double score);

    // Queries
    std::optional<Mission> GetMission(const std::string& missionId) const;
    std::optional<Mission> GetMissionForAmbulance(const std::string& ambulanceId) const;
    std::vector<Mission> GetActiveMissions() const;
    std::vector<Mission> GetMissionsByState(MissionState state) const;
    std::vector<Mission> GetAllMissions() const;
    bool HasMission(const std::string& missionId) const;
    size_t GetMissionCount() const;

    // Reset
    void Clear();

private:
    static uint64_t CurrentTimeMillis();

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, Mission> m_missions{};
};

} // namespace scheduler
} // namespace aegis

#endif // AEGIS_SCHEDULER_MISSION_MANAGER_H
