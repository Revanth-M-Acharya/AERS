/**
 * @file IncidentCommandEngine.h
 * @brief SMART AEGIS (AERS) Phase 12B - Real-Time Incident Command & Decision Support Engine
 *
 * Implements the City Command Center and Emergency Operations Center (EOC) backend.
 * Supervises all municipal emergency incidents and missions simultaneously, providing
 * situational awareness, emergency escalation, automated multi-agency command decisions,
 * manual dispatcher overrides, disaster coordination, and tamper-evident audit logging.
 *
 * Pure C++17, QNX SDP 8.0, Raspberry Pi 4 & POSIX compliant.
 * Zero third-party dependencies, thread-safe, deterministic.
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_INCIDENT_COMMAND_ENGINE_H
#define AEGIS_INCIDENT_COMMAND_ENGINE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <memory>
#include <chrono>

namespace aegis {
namespace incident_command {

// =============================================================================
// Enumerations
// =============================================================================

/**
 * @brief Operational severity of an emergency incident.
 */
enum class IncidentSeverity {
    NORMAL,     ///< Standard single-vehicle emergency response
    HIGH,       ///< Elevated response requiring priority coordination
    CRITICAL,   ///< Severe multi-casualty or life-threatening incident
    DISASTER    ///< Catastrophic event requiring multi-agency disaster mobilization
};

/**
 * @brief Command decisions emitted by automated rules or manual operator action.
 */
enum class CommandDecision {
    MONITOR,                 ///< Situation under control; standard continuous telemetry monitoring
    ESCALATE,                ///< Escalate incident severity and alert supervisory commanders
    DEPLOY_EXTRA_AMBULANCE,  ///< Dispatch secondary or backup emergency medical vehicle
    REQUEST_POLICE,          ///< Request immediate law enforcement assistance / traffic control
    REQUEST_FIRE,            ///< Request fire and rescue dispatch
    REQUEST_DISASTER_TEAM,   ///< Mobilize municipal disaster response and FEMA teams
    REQUEST_HELICOPTER,      ///< Request aeromedical evacuation / air ambulance
    REROUTE_ALL,             ///< Force global route replanning across active mission corridors
    LOCK_CORRIDORS,          ///< Lock signal phases across designated green corridors
    EVACUATE_AREA,           ///< Issue municipal evacuation order for affected zone
    REASSIGN_HOSPITAL        ///< Reassign destination hospital due to diversion or capacity exhaustion
};

/**
 * @brief Operational lifecycle status of an emergency incident.
 */
enum class IncidentStatus {
    ACTIVE,      ///< Incident is actively being managed and dispatched
    RESOLVED,    ///< On-scene triage complete; clinical care delivered; corridor cleared
    MONITORING,  ///< Post-incident surveillance or standby observation
    ESCALATED,   ///< Incident severity escalated; elevated command protocol active
    CLOSED       ///< Incident officially closed and archived
};

/**
 * @brief Tactical actions executed via dispatcher manual intervention.
 */
enum class OperatorAction {
    NONE,                    ///< No manual intervention
    MANUAL_OVERRIDE,         ///< General manual override by command staff
    FORCE_REROUTE,           ///< Force dynamic route replanning for an assigned mission
    FORCE_HOSPITAL_CHANGE,   ///< Override assigned destination hospital
    FORCE_AMBULANCE_CHANGE,  ///< Override assigned emergency response vehicle
    CANCEL_CORRIDOR,         ///< Immediately cancel an active green corridor
    RESTART_CORRIDOR         ///< Reactivate or reschedule a cancelled green corridor
};

// =============================================================================
// String Conversion Helpers
// =============================================================================

std::string ToString(IncidentSeverity severity);
std::string ToString(CommandDecision decision);
std::string ToString(IncidentStatus status);
std::string ToString(OperatorAction action);

IncidentSeverity SeverityFromString(const std::string& str);
CommandDecision DecisionFromString(const std::string& str);
IncidentStatus StatusFromString(const std::string& str);
OperatorAction ActionFromString(const std::string& str);

// =============================================================================
// Data Models
// =============================================================================

/**
 * @brief Comprehensive operational record for a municipal emergency incident.
 */
struct IncidentRecord {
    std::string incidentId;
    std::string missionId;
    IncidentSeverity severity{IncidentSeverity::NORMAL};
    IncidentStatus status{IncidentStatus::ACTIVE};
    std::string priority{"NORMAL"};
    std::string location;
    double latitude{0.0};
    double longitude{0.0};
    uint64_t creationTimestamp{0};
    uint64_t lastUpdateTimestamp{0};
    uint64_t resolutionTimestamp{0};
    std::string operatorName{"DISPATCH"};
    std::string assignedCommander{"NONE"};
    uint32_t affectedPopulation{0};
    double estimatedResolutionMinutes{0.0};
    CommandDecision currentDecision{CommandDecision::MONITOR};
    std::string notes;

    // Deterministic contextual rule triggers
    bool isMassCasualty{false};
    bool hasFire{false};
    bool trafficCollapse{false};
    bool hospitalUnavailable{false};
    bool ambulanceFailed{false};
    bool multipleMissionsAffected{false};
    bool largeCongestion{false};
    bool requiresAirEvac{false};

    IncidentRecord() = default;

    IncidentRecord(const std::string& id,
                   IncidentSeverity sev = IncidentSeverity::NORMAL,
                   const std::string& loc = "",
                   double lat = 0.0,
                   double lon = 0.0)
        : incidentId(id),
          severity(sev),
          location(loc),
          latitude(lat),
          longitude(lon) {}

    IncidentRecord(const std::string& id,
                   const std::string& missId,
                   IncidentSeverity sev,
                   const std::string& prio = "NORMAL")
        : incidentId(id),
          missionId(missId),
          severity(sev),
          priority(prio) {}
};

/**
 * @brief Tamper-evident command decision entry in the incident tactical timeline.
 */
struct DecisionHistory {
    uint64_t timestamp{0};
    std::string formattedTime;
    std::string incidentId;
    CommandDecision decision{CommandDecision::MONITOR};
    OperatorAction action{OperatorAction::NONE};
    std::string operatorName;
    std::string reason;

    DecisionHistory() = default;

    DecisionHistory(uint64_t ts,
                    CommandDecision dec,
                    const std::string& op,
                    const std::string& rsn)
        : timestamp(ts),
          decision(dec),
          operatorName(op),
          reason(rsn) {}

    DecisionHistory(uint64_t ts,
                    const std::string& incId,
                    CommandDecision dec,
                    OperatorAction act,
                    const std::string& op,
                    const std::string& rsn)
        : timestamp(ts),
          incidentId(incId),
          decision(dec),
          action(act),
          operatorName(op),
          reason(rsn) {}
};

/**
 * @brief City-wide incident command statistics aggregated across all incidents.
 */
struct IncidentStatistics {
    uint32_t totalIncidents{0};
    uint32_t activeIncidents{0};
    uint32_t resolvedIncidents{0};
    uint32_t criticalIncidents{0};
    uint32_t disasters{0};
    uint32_t manualOverrides{0};
    uint32_t automaticDecisions{0};
    double averageResolutionTime{0.0}; ///< In minutes
};

// =============================================================================
// Incident Command Engine Class
// =============================================================================

/**
 * @brief Master controller for municipal incident command and decision support.
 */
class IncidentCommandEngine {
public:
    IncidentCommandEngine();
    ~IncidentCommandEngine() = default;

    // Prevent copy/move to enforce thread safety
    IncidentCommandEngine(const IncidentCommandEngine&) = delete;
    IncidentCommandEngine& operator=(const IncidentCommandEngine&) = delete;
    IncidentCommandEngine(IncidentCommandEngine&&) = delete;
    IncidentCommandEngine& operator=(IncidentCommandEngine&&) = delete;

    // -------------------------------------------------------------------------
    // Incident Lifecycle Management
    // -------------------------------------------------------------------------
    bool CreateIncident(const IncidentRecord& record);
    bool ResolveIncident(const std::string& incidentId);
    bool CloseIncident(const std::string& incidentId);
    bool EscalateIncident(const std::string& incidentId, const std::string& reason = "");
    bool DeEscalateIncident(const std::string& incidentId, const std::string& reason = "");
    bool AssignCommander(const std::string& incidentId, const std::string& commander);
    bool AssignMission(const std::string& incidentId, const std::string& missionId);
    bool UpdateSeverity(const std::string& incidentId, IncidentSeverity severity);
    bool UpdateStatus(const std::string& incidentId, IncidentStatus status);

    // -------------------------------------------------------------------------
    // Command Decisions & Operator Intervention
    // -------------------------------------------------------------------------
    CommandDecision GenerateAutomaticDecision(const std::string& incidentId);
    bool ApplyDecision(const std::string& incidentId,
                       CommandDecision decision,
                       const std::string& reason,
                       const std::string& operatorName);
    bool ManualOverride(const std::string& incidentId,
                        OperatorAction action,
                        CommandDecision decision,
                        const std::string& reason,
                        const std::string& operatorName);

    // -------------------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------------------
    bool GetIncident(const std::string& incidentId, IncidentRecord& outRecord) const;
    std::vector<IncidentRecord> GetAllIncidents() const;
    std::vector<IncidentRecord> GetActiveIncidents() const;
    std::vector<IncidentRecord> GetCriticalIncidents() const;
    std::vector<DecisionHistory> GetDecisionHistory(const std::string& incidentId) const;
    std::vector<DecisionHistory> GetAllDecisionHistory() const;

    // -------------------------------------------------------------------------
    // Reports & Situational Dashboards
    // -------------------------------------------------------------------------
    std::string GenerateIncidentReport(const std::string& incidentId) const;
    std::string GenerateCitySituationReport() const;
    std::string GenerateDecisionTimeline(const std::string& incidentId) const;

    // -------------------------------------------------------------------------
    // Statistics & Lifecycle Utilities
    // -------------------------------------------------------------------------
    IncidentStatistics GetStatistics() const;
    bool ArchiveIncident(const std::string& incidentId);
    bool RestoreIncident(const std::string& incidentId);
    bool DeleteIncident(const std::string& incidentId);
    void Reset();
    void ClearHistory();

    // -------------------------------------------------------------------------
    // Contextual Trigger Setters (Rule Triggers)
    // -------------------------------------------------------------------------
    bool SetFireDetected(const std::string& incidentId, bool detected);
    bool SetMassCasualty(const std::string& incidentId, bool massCasualty, uint32_t affectedPop = 0);
    bool SetTrafficCollapse(const std::string& incidentId, bool collapse);
    bool SetHospitalUnavailable(const std::string& incidentId, bool unavailable);
    bool SetAmbulanceFailed(const std::string& incidentId, bool failed);
    bool SetMultipleMissionsAffected(const std::string& incidentId, bool affected);
    bool SetLargeCongestion(const std::string& incidentId, bool congestion);

private:
    CommandDecision EvaluateAutomaticDecisionInternal(const IncidentRecord& record, std::string& outReason) const;
    static uint64_t GetCurrentTimeMs();
    static std::string FormatTimestamp(uint64_t tsMs);

    // Dedicated Thread Safety Mutexes per specification:
    // registryMutex, historyMutex, statisticsMutex, reportMutex, decisionMutex, operatorMutex
    mutable std::mutex registryMutex_;
    mutable std::mutex historyMutex_;
    mutable std::mutex statisticsMutex_;
    mutable std::mutex reportMutex_;
    mutable std::mutex decisionMutex_;
    mutable std::mutex operatorMutex_;

    std::unordered_map<std::string, IncidentRecord> incidents_;
    std::unordered_map<std::string, IncidentRecord> archivedIncidents_;
    std::vector<DecisionHistory> decisionHistory_;
    std::unordered_map<std::string, std::vector<DecisionHistory>> incidentDecisionMap_;

    IncidentStatistics statistics_;
    double totalResolutionTimeMinutes_{0.0};
};

} // namespace incident_command
} // namespace aegis

#endif // AEGIS_INCIDENT_COMMAND_ENGINE_H

