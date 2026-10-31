/**
 * @file StatisticsEngine.hpp
 * @brief Analytical statistics engine calculating mission success rates, ETA savings, and clearance metrics.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace aegis::digital_twin {

struct MissionStatisticsReport {
    uint32_t totalMissionsDispatched{0};
    uint32_t totalMissionsCompleted{0};
    double missionSuccessRatePercent{100.0};
    double averageEtaReductionSeconds{0.0};
    double totalTimeSavedMinutes{0.0};

    uint32_t totalRouteReplans{0};
    std::unordered_map<std::string, uint32_t> replanReasons;

    uint32_t totalSignalSyncs{0};
    double averageCorridorClearanceSeconds{0.0};
    double averageHospitalHandoverSeconds{0.0};

    uint32_t totalIncidentsDetected{0};
    double averageIncidentDetectionToClearanceSeconds{0.0};

    std::string ToJson() const;
    std::string ToSummaryString() const;
};

class StatisticsEngine {
public:
    StatisticsEngine();
    ~StatisticsEngine() = default;

    void RecordMissionStarted(const std::string& missionId);
    void RecordMissionCompleted(const std::string& missionId, double etaReductionSec, double handoverSec = 45.0);
    void RecordRouteReplan(const std::string& missionId, const std::string& reason);
    void RecordSignalSync(const std::string& intersectionId, double clearanceSec = 12.0);
    void RecordIncidentClearance(const std::string& incidentId, double clearanceDurationSec);

    MissionStatisticsReport GenerateReport() const;
    void Reset();

private:
    mutable std::mutex mutex_;
    uint32_t missionsDispatched_{0};
    uint32_t missionsCompleted_{0};
    double totalEtaReductionSec_{0.0};
    double totalHandoverSec_{0.0};

    uint32_t totalReplans_{0};
    std::unordered_map<std::string, uint32_t> replanReasons_;

    uint32_t totalSignalSyncs_{0};
    double totalClearanceSec_{0.0};

    uint32_t totalIncidents_{0};
    double totalIncidentClearanceSec_{0.0};
};

} // namespace aegis::digital_twin

