/**
 * @file HospitalSelectionEngine.h
 * @brief SMART AEGIS (AERS) Phase 11B - Hospital Capacity Intelligence & Smart Hospital Selection Engine
 *
 * Provides centralized municipal hospital capacity monitoring, real-time emergency department
 * load tracking, and deterministic multi-criteria hospital selection for incoming ambulance missions.
 *
 * Pure C++17, QNX SDP 8.0 & Raspberry Pi 4 Starter Kit compliant.
 * Zero third-party dependencies, thread-safe, deterministic.
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_HOSPITAL_SELECTION_ENGINE_H
#define AEGIS_HOSPITAL_SELECTION_ENGINE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <memory>

namespace aegis {
namespace hospital {

/**
 * @brief Operational status of a medical hospital facility.
 */
enum class HospitalStatus {
    ONLINE,          ///< Hospital is fully operational and receiving patients
    OFFLINE,         ///< Hospital telemetry lost or communications offline
    FULL,            ///< Hospital emergency department and beds are at maximum capacity
    EMERGENCY_ONLY,  ///< Reserved strictly for critical life-threatening trauma/resuscitation
    MAINTENANCE      ///< Facility undergoing maintenance or decontamination; diversion active
};

/**
 * @brief Specialized clinical and trauma capabilities supported by a hospital.
 */
enum class HospitalCapability {
    GENERAL,     ///< General medical and surgical inpatient care
    TRAUMA,      ///< Level 1 / Level 2 Trauma center with dedicated trauma resuscitation team
    CARDIAC,     ///< Cardiac catheterization lab (STEMI receiving center)
    STROKE,      ///< Comprehensive stroke center (thrombolysis / thrombectomy)
    ICU,         ///< Intensive Care Unit with mechanical ventilation and critical care
    PEDIATRIC,   ///< Dedicated pediatric emergency department and pediatric inpatient wards
    NEONATAL,    ///< Neonatal Intensive Care Unit (NICU Level 3/4)
    BURN,        ///< Specialized burn care and burn resuscitation unit
    ORTHOPEDIC   ///< Specialized orthopedic trauma and fracture management
};

/**
 * @brief Convert HospitalStatus to string representation.
 */
const char* HospitalStatusToString(HospitalStatus status);

/**
 * @brief Convert string representation to HospitalStatus.
 */
bool StringToHospitalStatus(const std::string& str, HospitalStatus& outStatus);

/**
 * @brief Convert HospitalCapability to string representation.
 */
const char* HospitalCapabilityToString(HospitalCapability capability);

/**
 * @brief Convert string representation to HospitalCapability.
 */
bool StringToHospitalCapability(const std::string& str, HospitalCapability& outCap);

/**
 * @brief Real-time inpatient and emergency capacity snapshot.
 */
struct HospitalCapacitySnapshot {
    uint32_t totalBeds{100};
    uint32_t availableBeds{20};
    uint32_t totalIcuBeds{20};
    uint32_t availableIcuBeds{5};
    uint32_t totalOperatingTheatres{8};
    uint32_t availableOperatingTheatres{2};
    uint32_t emergencyRoomCapacity{30};
    uint32_t currentErPatients{15};
    double erOccupancyPercentage{50.0};
    uint32_t availableDoctors{6};
    uint32_t availableNurses{15};
    uint64_t lastUpdatedTimestamp{0};
};

/**
 * @brief Queue and ambulance unloading status at the emergency department.
 */
struct HospitalQueueInformation {
    uint32_t ambulanceQueueLength{0};        ///< Number of ambulances queued at triage bay
    double averageUnloadingDelayMinutes{5.0}; ///< Average minutes to offload a patient
    uint32_t triageQueueLength{0};           ///< Ambulatory walk-in patients waiting
    double estimatedWaitTimeMinutes{15.0};    ///< Estimated wait for non-critical triage
    uint64_t lastUpdatedTimestamp{0};
};

/**
 * @brief Comprehensive hospital facility record.
 */
struct HospitalRecord {
    std::string hospitalId;
    std::string hospitalName;
    double latitude{0.0};
    double longitude{0.0};
    HospitalStatus status{HospitalStatus::ONLINE};
    std::vector<HospitalCapability> capabilities;
    HospitalCapacitySnapshot capacity;
    HospitalQueueInformation queue;
    uint32_t assignedAmbulancesCount{0};     ///< Number of ambulances currently en route
    uint32_t totalPatientsHandledToday{0};   ///< Cumulative patient throughput today
    double averageTurnaroundMinutes{35.0};   ///< Door-to-handover average time
    uint64_t lastHeartbeatTimestamp{0};      ///< Telemetry keepalive timestamp in ms
    std::string contactPhone;
    std::string address;

    /**
     * @brief Checks if the hospital supports a specific capability.
     */
    bool HasCapability(HospitalCapability cap) const;
};

/**
 * @brief Request parameters for intelligent hospital selection.
 */
struct HospitalSelectionRequest {
    std::string incidentId;
    double incidentLatitude{0.0};
    double incidentLongitude{0.0};
    std::string patientSeverity{"HIGH"};     ///< "CRITICAL", "HIGH", "MEDIUM", "LOW"
    std::vector<HospitalCapability> requiredCapabilities;
    bool requiresIcu{false};
    bool requiresOperatingTheatre{false};
    double maxAcceptableEtaMinutes{45.0};
    std::string preferredHospitalId;
    uint64_t requestTimestamp{0};
};

/**
 * @brief Individual scored candidate hospital evaluation.
 */
struct HospitalCandidateScore {
    std::string hospitalId;
    std::string hospitalName;
    double distanceKm{0.0};
    double predictedEtaMinutes{0.0};
    double capabilityMatchScore{0.0};
    double icuAvailabilityScore{0.0};
    double bedAvailabilityScore{0.0};
    double erLoadScore{0.0};
    double queueDelayScore{0.0};
    double doctorAvailabilityScore{0.0};
    double operatingTheatreScore{0.0};
    double workloadBalanceScore{0.0};
    double totalScore{0.0};                  ///< Lower score indicates higher preference
    bool isEligible{true};
    std::string disqualificationReason;
};

/**
 * @brief Formal result output of smart hospital selection.
 */
struct HospitalSelectionResult {
    bool success{false};
    std::string incidentId;
    std::string selectedHospitalId;
    std::string selectedHospitalName;
    double distanceKm{0.0};
    double predictedEtaMinutes{0.0};
    uint32_t availableIcuBeds{0};
    uint32_t availableGeneralBeds{0};
    double erOccupancyPercentage{0.0};
    std::string selectionReason;
    uint64_t selectionTimestamp{0};
    std::vector<HospitalCandidateScore> rankedCandidates;
};

/**
 * @brief City-wide aggregate hospital network statistics.
 */
struct HospitalStatistics {
    size_t totalHospitals{0};
    size_t onlineHospitals{0};
    size_t offlineHospitals{0};
    size_t fullHospitals{0};
    size_t emergencyOnlyHospitals{0};
    size_t maintenanceHospitals{0};
    uint32_t totalBedsCapacity{0};
    uint32_t totalAvailableBeds{0};
    uint32_t totalIcuCapacity{0};
    uint32_t totalAvailableIcuBeds{0};
    uint32_t totalOperatingTheatres{0};
    uint32_t totalAvailableOperatingTheatres{0};
    double averageErOccupancyPercentage{0.0};
    uint32_t totalAmbulanceQueueLength{0};
    double averageAmbulanceUnloadDelayMinutes{0.0};
    uint32_t totalPatientsHandledToday{0};
};

/**
 * @class HospitalSelectionEngine
 * @brief Centralized engine for hospital capacity intelligence and deterministic destination routing.
 *
 * Evaluates candidate hospitals across 10 deterministic selection rules:
 * Rule 1: Shortest predicted ETA
 * Rule 2: Medical capability match
 * Rule 3: ICU availability
 * Rule 4: Bed availability
 * Rule 5: Emergency room load
 * Rule 6: Queue waiting time
 * Rule 7: Doctor availability
 * Rule 8: Operating theatre availability
 * Rule 9: Hospital online status
 * Rule 10: Hospital workload balancing
 */
class HospitalSelectionEngine {
public:
    HospitalSelectionEngine();
    ~HospitalSelectionEngine() = default;

    // Prevent copy/move
    HospitalSelectionEngine(const HospitalSelectionEngine&) = delete;
    HospitalSelectionEngine& operator=(const HospitalSelectionEngine&) = delete;
    HospitalSelectionEngine(HospitalSelectionEngine&&) = delete;
    HospitalSelectionEngine& operator=(HospitalSelectionEngine&&) = delete;

    /**
     * @brief Registers a new hospital or updates an existing record.
     */
    bool RegisterHospital(const HospitalRecord& hospital);

    /**
     * @brief Removes a hospital from active management.
     */
    bool RemoveHospital(const std::string& hospitalId);

    /**
     * @brief Updates the capacity snapshot of a hospital.
     */
    bool UpdateHospitalCapacity(const std::string& hospitalId, const HospitalCapacitySnapshot& capacity);

    /**
     * @brief Updates the ambulance bay queue length and estimated offload delay.
     */
    bool UpdateQueueLength(const std::string& hospitalId, uint32_t queueLength, double avgDelayMinutes = 5.0);

    /**
     * @brief Updates available emergency physician count.
     */
    bool UpdateDoctorAvailability(const std::string& hospitalId, uint32_t availableDoctors);

    /**
     * @brief Updates the active emergency room patient load.
     */
    bool UpdateEmergencyRoomLoad(const std::string& hospitalId, uint32_t erPatients);

    /**
     * @brief Updates operating theatre availability.
     */
    bool UpdateOperatingTheatreAvailability(const std::string& hospitalId, uint32_t availableTheatres);

    /**
     * @brief Updates operational status (ONLINE, OFFLINE, FULL, EMERGENCY_ONLY, MAINTENANCE).
     */
    bool UpdateHospitalStatus(const std::string& hospitalId, HospitalStatus status);

    /**
     * @brief Finds the geographically closest hospital supporting a minimum capability.
     */
    bool FindNearestHospital(double lat, double lon, HospitalRecord& outHospital,
                             HospitalCapability requiredCap = HospitalCapability::GENERAL);

    /**
     * @brief Executes multi-criteria deterministic ranking to find the optimal destination hospital.
     */
    HospitalSelectionResult FindBestHospital(const HospitalSelectionRequest& request);

    /**
     * @brief Failsafe recomputation when a previously selected hospital transitions to FULL or OFFLINE.
     */
    HospitalSelectionResult RecomputeDestination(const std::string& previousHospitalId,
                                                const HospitalSelectionRequest& request);

    /**
     * @brief Generates a formatted ASCII operational hospital capacity report.
     */
    std::string GenerateHospitalReport() const;

    /**
     * @brief Returns city-wide hospital network aggregate statistics.
     */
    HospitalStatistics GetHospitalStatistics() const;

    /**
     * @brief Returns a snapshot copy of all registered hospital records.
     */
    std::vector<HospitalRecord> GetHospitalRecords() const;

    /**
     * @brief Queries an individual hospital record by ID.
     */
    bool GetHospitalRecord(const std::string& hospitalId, HospitalRecord& outHospital) const;

    /**
     * @brief Returns history of structured forensic audit logs.
     */
    std::vector<std::string> GetStructuredLogs() const;

    /**
     * @brief Clears historical selection results and structured logs.
     */
    void ClearHistory();

    /**
     * @brief Updates heartbeat keepalive timestamp for a hospital.
     */
    bool UpdateHeartbeat(const std::string& hospitalId, uint64_t timestampMs = 0);

    /**
     * @brief Sets the heartbeat timeout duration in milliseconds.
     */
    void SetHeartbeatTimeoutMs(uint64_t timeoutMs);

    /**
     * @brief Gets the heartbeat timeout duration in milliseconds.
     */
    uint64_t GetHeartbeatTimeoutMs() const;

    /**
     * @brief Sets simulated monotonic time for deterministic testing without sleeping.
     */
    void SetSimulatedTimeMs(uint64_t simulatedTimeMs);

    /**
     * @brief Gets current simulated monotonic time in milliseconds.
     */
    uint64_t GetSimulatedTimeMs() const;

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, HospitalRecord> m_hospitals;
    std::vector<std::string> m_structuredLogs;
    std::vector<HospitalSelectionResult> m_selectionHistory;
    uint64_t m_heartbeatTimeoutMs{30000};
    uint64_t m_simulatedTimeMs{0};

    // Internal calculation utilities
    static double CalculateDistanceKmInternal(double lat1, double lon1, double lat2, double lon2);
    uint64_t GetCurrentTimeMsInternal() const;
    void CheckHeartbeatTimeoutsInternal(uint64_t nowMs);
    HospitalCandidateScore EvaluateCandidateInternal(const HospitalRecord& hospital,
                                                    const HospitalSelectionRequest& request,
                                                    double distanceKm,
                                                    double etaMinutes) const;
    std::string FormatAuditLogInternal(const HospitalSelectionResult& result) const;
};

} // namespace hospital
} // namespace aegis

#endif // AEGIS_HOSPITAL_SELECTION_ENGINE_H
