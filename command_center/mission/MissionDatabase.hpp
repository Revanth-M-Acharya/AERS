/**
 * @file MissionDatabase.hpp
 * @brief In-memory historical repository of completed and active emergency missions.
 * Captures routing, dynamic replans, police advisories, preemption, and outcomes for review and replay.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <cstdint>

namespace aegis::command_center {

struct HistoricalMissionRecord {
    std::string missionId;
    std::string emergencyType{"CARDIAC_ARREST"};
    std::string priority{"CRITICAL (Code Red)"};
    std::string assignedAmbulance{"AMB-01"};
    std::string assignedHospital{"City Trauma Hospital"};
    uint64_t dispatchTimeMonoMs{0};
    uint64_t arrivalTimeMonoMs{0};
    uint64_t completionTimeMonoMs{0};
    double travelTimeSeconds{138.0}; // ~2m 18s (matching Command Center example!)
    std::vector<std::string> routeUsed;
    bool wasReplanned{false};
    std::string replanReason{"None"};
    std::vector<std::string> signalsPreempted;
    bool manualPoliceAdvisory{false};
    std::string outcome{"SUCCESSFUL_PATIENT_HANDOVER"};
    std::string missionStatus{"COMPLETED"}; // ACTIVE, COMPLETED, CANCELLED
};

class MissionDatabase {
public:
    MissionDatabase();
    ~MissionDatabase() = default;

    // Prevent copying
    MissionDatabase(const MissionDatabase&) = delete;
    MissionDatabase& operator=(const MissionDatabase&) = delete;

    /**
     * @brief Populate default historical completed missions (e.g. 15 today's missions, ~2m 18s avg response).
     */
    void InitializeDefaultHistory();

    /**
     * @brief Record new mission intake.
     */
    bool CreateMission(const HistoricalMissionRecord& mission);

    /**
     * @brief Update dynamic route replanning event.
     */
    bool RecordReplanning(const std::string& missionId,
                          const std::vector<std::string>& newRoute,
                          const std::string& reason);

    /**
     * @brief Record police verified road incident advisory.
     */
    bool RecordPoliceAdvisory(const std::string& missionId, const std::string& reason);

    /**
     * @brief Mark mission completed upon patient delivery.
     */
    bool CompleteMission(const std::string& missionId,
                         uint64_t completionTimeMs,
                         const std::string& outcome = "SUCCESSFUL_PATIENT_HANDOVER");

    /**
     * @brief Fetch single mission by ID.
     */
    std::optional<HistoricalMissionRecord> GetMission(const std::string& missionId) const;

    /**
     * @brief Fetch all missions in database.
     */
    std::vector<HistoricalMissionRecord> GetAllMissions() const;

    /**
     * @brief Fetch active ongoing missions.
     */
    std::vector<HistoricalMissionRecord> GetActiveMissions() const;

    /**
     * @brief Fetch completed missions.
     */
    std::vector<HistoricalMissionRecord> GetCompletedMissions() const;

    /**
     * @brief Total number of missions stored.
     */
    size_t GetTotalMissionsCount() const;

    /**
     * @brief Compute average travel/response time in seconds across completed missions.
     */
    double GetAverageResponseTimeSec() const;

    /**
     * @brief Clear all mission records.
     */
    void Clear();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, HistoricalMissionRecord> missions_;
    std::vector<std::string> chronologicalOrder_;
};

} // namespace aegis::command_center
