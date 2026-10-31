/**
 * @file IncidentManager.hpp
 * @brief Management of active emergency incident records, progression, and historical audit trail.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/control_center/EmergencyValidator.hpp"
#include "coordinator/control_center/EmergencyPriorityEngine.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <cstdint>

namespace aegis::coordinator::control_center {

enum class IncidentState {
    REPORTED,
    VALIDATED,
    DISPATCHED,
    EN_ROUTE,
    ON_SCENE,
    TRANSPORTING,
    AT_HOSPITAL,
    HANDOVER_COMPLETE,
    CANCELLED,
    CLOSED
};

inline std::string IncidentStateToString(IncidentState state) {
    switch (state) {
        case IncidentState::REPORTED:          return "REPORTED";
        case IncidentState::VALIDATED:         return "VALIDATED";
        case IncidentState::DISPATCHED:        return "DISPATCHED";
        case IncidentState::EN_ROUTE:          return "EN_ROUTE";
        case IncidentState::ON_SCENE:          return "ON_SCENE";
        case IncidentState::TRANSPORTING:      return "TRANSPORTING";
        case IncidentState::AT_HOSPITAL:       return "AT_HOSPITAL";
        case IncidentState::HANDOVER_COMPLETE: return "HANDOVER_COMPLETE";
        case IncidentState::CANCELLED:         return "CANCELLED";
        case IncidentState::CLOSED:            return "CLOSED";
        default:                               return "UNKNOWN";
    }
}

struct IncidentRecord {
    std::string incidentId;
    std::string emergencyId;
    EmergencyType type{EmergencyType::GeneralEmergency};
    PriorityLevel priority{PriorityLevel::Priority1};
    PatientSeverity severity{PatientSeverity::CRITICAL};
    AmbulanceClass ambulanceClass{AmbulanceClass::ALS};
    std::string assignedAmbulanceId;
    std::string backupAmbulanceId;
    std::string assignedHospitalId;
    std::string destinationHospitalId;
    std::string assignedDispatcherId;
    std::string originIntersectionId;
    std::vector<std::string> currentRoute;
    double currentEtaSeconds{0.0};
    IncidentState state{IncidentState::REPORTED};
    uint64_t createdAtMonoMs{0};
    uint64_t updatedAtMonoMs{0};
    uint64_t closedAtMonoMs{0};
    std::vector<std::string> missionHistory;
};

class IncidentManager {
public:
    IncidentManager() = default;
    ~IncidentManager() = default;

    /**
     * @brief Create a new incident entry in REPORTED state.
     */
    bool CreateIncident(const IncidentRecord& record);

    /**
     * @brief Update state of an existing incident.
     */
    bool UpdateState(const std::string& incidentId, IncidentState newState, const std::string& note = "", uint64_t monoMs = 0);

    /**
     * @brief Update assigned resources.
     */
    bool AssignResources(const std::string& incidentId,
                         const std::string& ambulanceId,
                         const std::string& hospitalId,
                         const std::string& dispatcherId = "");

    bool UpdateRouteAndETA(const std::string& incidentId,
                           const std::vector<std::string>& route,
                           double etaSeconds,
                           uint64_t monoMs = 0);

    bool CancelIncident(const std::string& incidentId, const std::string& reason, uint64_t monoMs = 0);
    bool CompleteIncident(const std::string& incidentId, uint64_t monoMs = 0);
    bool ArchiveIncident(const std::string& incidentId, uint64_t monoMs = 0);

    std::optional<IncidentRecord> GetIncident(const std::string& incidentId) const;
    std::optional<IncidentRecord> GetIncidentByEmergencyId(const std::string& emergencyId) const;
    std::vector<IncidentRecord> GetAllActiveIncidents() const;
    std::vector<IncidentRecord> GetAllIncidents() const;
    size_t GetActiveIncidentCount() const;

    void Reset();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, IncidentRecord> incidents_;
    std::unordered_map<std::string, std::string> emergencyToIncidentMap_;
};

} // namespace aegis::coordinator::control_center
