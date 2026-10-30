/**
 * @file MissionExecutionEngine.h
 * @brief SMART AEGIS (AERS) Phase 12A - City-Wide Emergency Mission Execution Engine
 *
 * Provides the highest-level execution layer and master controller for the entire
 * Autonomous Emergency Response System. Coordinates medical priority, ambulance fleet
 * assignment, hospital resource allocation, corridor routing, green wave scheduling,
 * dynamic replanning, distributed traffic signal synchronization, and edge execution
 * into a single unified, deterministic mission lifecycle.
 *
 * Pure C++17, QNX SDP 8.0, Raspberry Pi 4 & POSIX compliant.
 * Zero third-party dependencies, thread-safe, deterministic.
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_MISSION_EXECUTION_ENGINE_H
#define AEGIS_MISSION_EXECUTION_ENGINE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <memory>
#include <optional>
#include <chrono>

namespace aegis {
namespace mission {

// =============================================================================
// Enumerations
// =============================================================================

/**
 * @brief Operational status of an emergency mission.
 */
enum class MissionStatus {
    NEW,          ///< Mission created, awaiting initial assignments and validation
    READY,        ///< Ambulance, hospital, route, and corridor assigned; ready for start
    ACTIVE,       ///< Mission actively in progress
    PAUSED,       ///< Mission temporarily suspended (e.g., awaiting triage confirmation)
    COMPLETED,    ///< Mission successfully completed; patient handed over at hospital
    FAILED,       ///< Mission failed due to unrecoverable incident or medical abort
    ABORTED,      ///< Mission aborted by operator or tactical coordinator
    CANCELLED     ///< Mission cancelled prior to patient handover (false alarm, redirected)
};

/**
 * @brief Discrete operational stage of an active emergency mission.
 */
enum class MissionStage {
    CALL_RECEIVED,         ///< 911 / emergency dispatch call ingested
    AMBULANCE_SELECTION,   ///< Matching vehicle from fleet management engine
    HOSPITAL_SELECTION,    ///< Matching facility from hospital resource engine
    ROUTE_PLANNING,        ///< Generating optimal route to incident & destination
    GREEN_CORRIDOR_SETUP,  ///< Synchronizing green wave traffic signals along route
    AMBULANCE_EN_ROUTE,    ///< Emergency vehicle dispatched and en route to patient
    PATIENT_PICKUP,        ///< Ambulance on scene; paramedic assessment and patient pickup
    TRANSPORTING,          ///< Transporting patient to receiving hospital along green corridor
    ARRIVING_HOSPITAL,     ///< Arriving at hospital emergency triage bay
    HANDOVER,              ///< Transfer of clinical care to hospital trauma/ED team
    MISSION_COMPLETE       ///< Handover finished, resources released, mission closed
};

/**
 * @brief Triage priority tier of an emergency mission.
 */
enum class MissionPriority {
    LOW,            ///< Non-urgent (e.g. minor contusion)
    MEDIUM,         ///< Moderate urgency (e.g. fracture)
    HIGH,           ///< Emergent condition (e.g. respiratory distress)
    CRITICAL,       ///< Immediate life threat (e.g. cardiac arrest, major trauma)
    MASS_CASUALTY   ///< Disaster / multi-casualty incident
};

/**
 * @brief Types of tactical milestones recorded in the mission timeline.
 */
enum class MissionEventType {
    MISSION_CREATED,
    PRIORITY_ASSIGNED,
    AMBULANCE_ASSIGNED,
    HOSPITAL_ASSIGNED,
    ROUTE_GENERATED,
    GREEN_CORRIDOR_STARTED,
    TRAFFIC_PLAN_GENERATED,
    MISSION_STARTED,
    PATIENT_PICKED_UP,
    ROUTE_REPLANNED,
    HOSPITAL_UPDATED,
    TRAFFIC_DELAY,
    ETA_UPDATED,
    AMBULANCE_ARRIVED,
    PATIENT_HANDED_OVER,
    MISSION_COMPLETED,
    MISSION_CANCELLED,
    MISSION_FAILED
};

/**
 * @brief String conversion utilities for enums.
 */
const char* MissionStatusToString(MissionStatus status);
bool StringToMissionStatus(const std::string& str, MissionStatus& outStatus);

const char* MissionStageToString(MissionStage stage);
bool StringToMissionStage(const std::string& str, MissionStage& outStage);

const char* MissionPriorityToString(MissionPriority priority);
bool StringToMissionPriority(const std::string& str, MissionPriority& outPriority);

const char* MissionEventTypeToString(MissionEventType eventType);

// =============================================================================
// Data Models
// =============================================================================

/**
 * @brief Complete master record describing an emergency mission lifecycle.
 */
struct MissionRecord {
    std::string missionId{""};
    std::string emergencyId{""};
    std::string incidentId{""};
    std::string patientId{""};
    std::string ambulanceId{""};
    std::string hospitalId{""};
    MissionPriority priority{MissionPriority::HIGH};
    MissionStage stage{MissionStage::CALL_RECEIVED};
    MissionStatus status{MissionStatus::NEW};

    uint64_t missionCreationTimestamp{0};    ///< Epoch ms when mission was created
    uint64_t dispatchTimestamp{0};           ///< Epoch ms when ambulance was dispatched
    uint64_t pickupTimestamp{0};             ///< Epoch ms when patient was picked up
    uint64_t hospitalArrivalTimestamp{0};    ///< Epoch ms when ambulance reached hospital
    uint64_t missionCompletionTimestamp{0};  ///< Epoch ms when handover completed

    double incidentLatitude{0.0};
    double incidentLongitude{0.0};
    double hospitalLatitude{0.0};
    double hospitalLongitude{0.0};

    double distance{0.0};                    ///< Remaining distance to active waypoint (km)
    double estimatedEta{0.0};                ///< Baseline estimated ETA (minutes)
    double actualEta{0.0};                   ///< Dynamic or completed actual travel time (minutes)

    std::string currentRouteId{""};
    std::string greenCorridorId{""};
    uint32_t assignedCorridorVersion{1};
    uint32_t trafficPlanVersion{1};

    uint64_t missionTimeout{3600000};        ///< Max mission lifespan in ms (default 1 hr)
    std::string missionNotes{""};
    double currentSpeed{0.0};                ///< Live vehicle speed in km/h
    double averageSpeed{50.0};               ///< Historical average speed in km/h
    double elapsedTime{0.0};                 ///< Cumulative elapsed mission duration (minutes)
    double remainingEta{0.0};                ///< Dynamic remaining ETA to destination (minutes)
    double totalRouteDistance{0.0};          ///< Total corridor route length (km)
    double currentMissionScore{100.0};       ///< Composite execution performance score (0-100)

    uint32_t patientAcuity{1};               ///< Clinical acuity tier (1=Immediate, 5=Non-urgent)
    std::string assignedDoctor{""};
    std::string crewId{""};
    std::string vehicleCapability{""};       ///< Vehicle tier (BLS, ALS, ICU, CARDIAC)
    std::string hospitalCapability{""};      ///< Hospital specialty (TRAUMA, CARDIAC, ICU)
    std::string cancellationReason{""};
    std::string failureReason{""};

    MissionRecord() = default;

    MissionRecord(std::string mId, std::string eId, std::string incId,
                  double incLat = 0.0, double incLon = 0.0,
                  MissionPriority prio = MissionPriority::HIGH)
        : missionId(std::move(mId)),
          emergencyId(std::move(eId)),
          incidentId(std::move(incId)),
          priority(prio),
          incidentLatitude(incLat),
          incidentLongitude(incLon) {}

    bool IsTerminal() const {
        return status == MissionStatus::COMPLETED ||
               status == MissionStatus::FAILED ||
               status == MissionStatus::ABORTED ||
               status == MissionStatus::CANCELLED;
    }

    bool IsActive() const {
        return status == MissionStatus::ACTIVE ||
               status == MissionStatus::PAUSED ||
               status == MissionStatus::READY;
    }
};

/**
 * @brief Entry in the chronological tactical mission timeline.
 */
struct MissionTimelineEntry {
    uint64_t timestampMs{0};
    std::string formattedTime{""};
    MissionEventType eventType{MissionEventType::MISSION_CREATED};
    std::string description{""};
    MissionStage stage{MissionStage::CALL_RECEIVED};
    MissionStatus status{MissionStatus::NEW};

    MissionTimelineEntry() = default;
    MissionTimelineEntry(uint64_t ts, std::string timeStr, MissionEventType evt,
                         std::string desc, MissionStage stg, MissionStatus st)
        : timestampMs(ts),
          formattedTime(std::move(timeStr)),
          eventType(evt),
          description(std::move(desc)),
          stage(stg),
          status(st) {}
};

/**
 * @brief System-wide aggregation statistics for mission execution.
 */
struct MissionStatistics {
    uint32_t totalMissions{0};
    uint32_t activeMissions{0};
    uint32_t completedMissions{0};
    uint32_t cancelledMissions{0};
    uint32_t failedMissions{0};
    double averageEta{0.0};                     ///< Average estimated ETA (minutes)
    double averageResponseTime{0.0};            ///< Dispatch to scene response time (minutes)
    double averagePickupTime{0.0};              ///< Patient triage and loading duration (minutes)
    double averageMissionTime{0.0};             ///< Total mission lifecycle duration (minutes)
    double averageHospitalArrivalTime{0.0};     ///< Transit time to hospital destination (minutes)
    double longestMission{0.0};                 ///< Longest completed mission duration (minutes)
    double shortestMission{0.0};                ///< Shortest completed mission duration (minutes)
    double totalDistance{0.0};                  ///< Cumulative total distance travelled (km)
    double averageDistance{0.0};                ///< Average distance per mission (km)
    uint32_t totalGreenCorridors{0};            ///< Total green corridor wave runs activated
    uint32_t totalRouteReplans{0};              ///< Total dynamic route replanning interventions
    double missionSuccessRate{0.0};             ///< Success rate percentage (0 - 100%)
    double missionFailureRate{0.0};             ///< Failure rate percentage (0 - 100%)
};

// =============================================================================
// MissionExecutionEngine Class
// =============================================================================

/**
 * @class MissionExecutionEngine
 * @brief Master controller executing and coordinating city-wide emergency missions.
 *
 * Thread-safe, deterministic, and QNX SDP 8.0 compliant.
 * Manages mission registries, timelines, state machines, assignments, reports,
 * and deterministic validation rules under strict sub-millisecond execution constraints.
 */
class MissionExecutionEngine {
public:
    MissionExecutionEngine();
    ~MissionExecutionEngine() = default;

    // Disallow copy/move to enforce single master identity and thread safety
    MissionExecutionEngine(const MissionExecutionEngine&) = delete;
    MissionExecutionEngine& operator=(const MissionExecutionEngine&) = delete;
    MissionExecutionEngine(MissionExecutionEngine&&) = delete;
    MissionExecutionEngine& operator=(MissionExecutionEngine&&) = delete;

    // -------------------------------------------------------------------------
    // Mission Creation & Lifecycle APIs
    // -------------------------------------------------------------------------

    /**
     * @brief Create a new emergency mission in the centralized registry.
     * Validates Rule 1 (Unique ID) and Rule 2 (Valid Emergency ID).
     * @param mission Complete or initialized mission record.
     * @return true if successfully created, false on duplicate ID or invalid fields.
     */
    bool CreateMission(const MissionRecord& mission);

    /**
     * @brief Transition a READY mission to ACTIVE execution.
     * Validates Rules 3, 4, 5, 6, 7, 8.
     * @param missionId Unique mission identifier.
     * @return true if started, false if mission not found or prerequisites not met.
     */
    bool StartMission(const std::string& missionId);

    /**
     * @brief Temporarily suspend an active mission.
     * @param missionId Unique mission identifier.
     * @param reason Diagnostic explanation for pause.
     * @return true if paused, false otherwise.
     */
    bool PauseMission(const std::string& missionId, const std::string& reason = "");

    /**
     * @brief Resume a paused mission back to ACTIVE state.
     * @param missionId Unique mission identifier.
     * @return true if resumed, false otherwise.
     */
    bool ResumeMission(const std::string& missionId);

    /**
     * @brief Terminate an active mission prematurely due to tactical abort.
     * @param missionId Unique mission identifier.
     * @param reason Explanation for abort.
     * @return true if aborted, false otherwise.
     */
    bool AbortMission(const std::string& missionId, const std::string& reason);

    /**
     * @brief Cancel an active mission prior to handover (redirect or false alarm).
     * @param missionId Unique mission identifier.
     * @param reason Explanation for cancellation.
     * @return true if cancelled, false otherwise.
     */
    bool CancelMission(const std::string& missionId, const std::string& reason);

    /**
     * @brief Complete a mission following clinical handover at destination hospital.
     * Validates Rule 10 (Archival and statistics recording).
     * @param missionId Unique mission identifier.
     * @return true if completed, false if not in valid pre-completion stage.
     */
    bool CompleteMission(const std::string& missionId);

    // -------------------------------------------------------------------------
    // Mission Assignment APIs
    // -------------------------------------------------------------------------

    /**
     * @brief Assign a designated emergency ambulance to the mission.
     */
    bool AssignAmbulance(const std::string& missionId, const std::string& ambulanceId,
                         const std::string& crewId = "", const std::string& vehicleCapability = "");

    /**
     * @brief Assign a destination medical facility to the mission.
     */
    bool AssignHospital(const std::string& missionId, const std::string& hospitalId,
                        double hospitalLat = 0.0, double hospitalLon = 0.0,
                        const std::string& hospitalCapability = "");

    /**
     * @brief Assign an active green corridor identifier and version to the mission.
     */
    bool AssignGreenCorridor(const std::string& missionId, const std::string& corridorId,
                             uint32_t corridorVersion = 1);

    /**
     * @brief Assign an active navigation route to the mission.
     */
    bool AssignRoute(const std::string& missionId, const std::string& routeId,
                     double distanceKm = 0.0, double etaMinutes = 0.0);

    /**
     * @brief Assign a distributed traffic signal synchronization plan version.
     */
    bool AssignTrafficPlan(const std::string& missionId, uint32_t planVersion);

    // -------------------------------------------------------------------------
    // Mission Updates APIs
    // -------------------------------------------------------------------------

    /**
     * @brief Advance or update the discrete operational stage of the mission.
     * Enforces the valid chronological stage progression.
     */
    bool UpdateMissionStage(const std::string& missionId, MissionStage newStage);

    /**
     * @brief Update the status of the mission, enforcing state machine rules.
     */
    bool UpdateMissionStatus(const std::string& missionId, MissionStatus newStatus,
                             const std::string& note = "");

    /**
     * @brief Update the estimated or remaining ETA to the next destination waypoint.
     */
    bool UpdateETA(const std::string& missionId, double remainingEtaMinutes);

    /**
     * @brief Update the remaining route distance.
     */
    bool UpdateDistance(const std::string& missionId, double remainingDistanceKm);

    /**
     * @brief Update the clinical acuity tier or condition notes for the patient.
     */
    bool UpdatePatientCondition(const std::string& missionId, uint32_t patientAcuity,
                                const std::string& conditionNotes = "");

    /**
     * @brief Dynamically divert the patient to a newly selected hospital.
     */
    bool UpdateHospital(const std::string& missionId, const std::string& newHospitalId,
                        const std::string& reason = "");

    /**
     * @brief Dynamically reassign the emergency response to a different ambulance.
     */
    bool UpdateAmbulance(const std::string& missionId, const std::string& newAmbulanceId,
                         const std::string& reason = "");

    // -------------------------------------------------------------------------
    // Mission Queries APIs
    // -------------------------------------------------------------------------

    /**
     * @brief Retrieve mission record by mission ID.
     */
    bool GetMission(const std::string& missionId, MissionRecord& outMission) const;
    std::optional<MissionRecord> GetMission(const std::string& missionId) const;

    /**
     * @brief Retrieve mission record by incident ID.
     */
    bool GetMissionByIncident(const std::string& incidentId, MissionRecord& outMission) const;

    /**
     * @brief Retrieve mission record if currently active.
     */
    bool GetActiveMission(const std::string& missionId, MissionRecord& outMission) const;

    /**
     * @brief Retrieve snapshot of all currently active missions.
     */
    std::vector<MissionRecord> GetAllActiveMissions() const;

    /**
     * @brief Retrieve snapshot of all completed missions.
     */
    std::vector<MissionRecord> GetCompletedMissions() const;

    /**
     * @brief Retrieve snapshot of all failed, aborted, or cancelled missions.
     */
    std::vector<MissionRecord> GetFailedMissions() const;

    /**
     * @brief Retrieve snapshot of all registered missions across all statuses.
     */
    std::vector<MissionRecord> GetAllMissions() const;

    // -------------------------------------------------------------------------
    // Mission Reports & Timelines APIs
    // -------------------------------------------------------------------------

    /**
     * @brief Generate formatted ASCII mission report matching [MISSION] specification.
     */
    std::string GenerateMissionReport(const std::string& missionId) const;

    /**
     * @brief Generate chronological ASCII timeline of all tactical events for a mission.
     */
    std::string GenerateMissionTimeline(const std::string& missionId) const;

    /**
     * @brief Generate one-line tactical summary for an active or completed mission.
     */
    std::string GenerateMissionSummary(const std::string& missionId) const;

    /**
     * @brief Retrieve raw chronological timeline entries for programmatic inspection.
     */
    std::vector<MissionTimelineEntry> GetTimeline(const std::string& missionId) const;

    /**
     * @brief Generate structured forensic audit log line conforming to specification.
     */
    std::string FormatStructuredLog(const std::string& missionId) const;

    // -------------------------------------------------------------------------
    // Mission Statistics APIs
    // -------------------------------------------------------------------------

    /**
     * @brief Retrieve aggregated system-wide mission execution statistics.
     */
    MissionStatistics GetStatistics() const;

    double GetAverageResponseTime() const;
    double GetAverageMissionDuration() const;
    double GetAveragePickupTime() const;
    double GetAverageHospitalTime() const;

    // -------------------------------------------------------------------------
    // Mission Management APIs
    // -------------------------------------------------------------------------

    /**
     * @brief Move a completed or terminated mission to the historical archive.
     */
    bool ArchiveMission(const std::string& missionId);

    /**
     * @brief Restore an archived mission back into the active registry.
     */
    bool RestoreMission(const std::string& missionId);

    /**
     * @brief Permanently purge a mission record and its timeline entries.
     */
    bool DeleteMission(const std::string& missionId);

    /**
     * @brief Clear all archived missions, generated reports, and historical timelines.
     */
    void ClearHistory();

    /**
     * @brief Reset entire engine to clean factory state.
     */
    void Reset();

    // -------------------------------------------------------------------------
    // Event Logging & Timeout Monitoring
    // -------------------------------------------------------------------------

    /**
     * @brief Record a tactical event in the mission timeline.
     */
    void LogEvent(const std::string& missionId, MissionEventType eventType,
                  const std::string& description);

    /**
     * @brief Evaluate active missions against Rule 9 (Mission Timeout Monitoring).
     * @param currentTimestampMs Current epoch timestamp (or 0 for real monotonic clock).
     * @return true if any mission timed out and was transitioned to FAILED.
     */
    bool CheckTimeouts(uint64_t currentTimestampMs = 0);

    /**
     * @brief Calculate geodesic Haversine distance in kilometers.
     */
    static double CalculateDistanceKm(double lat1, double lon1, double lat2, double lon2);

private:
    uint64_t GetCurrentTimeMs() const;
    std::string FormatTimestamp(uint64_t timestampMs) const;
    bool ValidateTransition(MissionStatus from, MissionStatus to) const;
    bool ValidateStageProgression(MissionStage from, MissionStage to) const;
    void RecalculateStatisticsLocked();

    // Synchronization primitives (separate mutexes per architectural domain)
    mutable std::mutex registryMutex_;    ///< Protects missions_ and incidentMap_
    mutable std::mutex statisticsMutex_;  ///< Protects statistics_
    mutable std::mutex timelineMutex_;    ///< Protects timelines_
    mutable std::mutex reportsMutex_;     ///< Protects generatedReports_
    mutable std::mutex historyMutex_;     ///< Protects archivedMissions_
    mutable std::mutex analyticsMutex_;   ///< Protects analytics metrics & cache

    // Data Stores
    std::unordered_map<std::string, MissionRecord> missions_;
    std::unordered_map<std::string, std::string> incidentMap_; ///< incidentId -> missionId
    std::unordered_map<std::string, std::vector<MissionTimelineEntry>> timelines_;
    std::unordered_map<std::string, MissionRecord> archivedMissions_;
    std::vector<std::string> generatedReports_;
    MissionStatistics statistics_;
};

} // namespace mission
} // namespace aegis

#endif // AEGIS_MISSION_EXECUTION_ENGINE_H

