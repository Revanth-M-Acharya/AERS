/**
 * @file HospitalResourceEngine.h
 * @brief SMART AEGIS (AERS) Phase 11B - Hospital Resource Allocation & Bed Management Engine
 *
 * Provides a centralized municipal Hospital Resource Allocation Engine that continuously
 * maintains live hospital resources, real-time emergency department and ICU capacities,
 * and automatically selects the optimal destination hospital for emergency ambulances
 * utilizing deterministic multi-criteria scoring.
 *
 * Pure C++17, QNX SDP 8.0 & Raspberry Pi 4 Starter Kit compliant.
 * Zero third-party dependencies, thread-safe, deterministic.
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_HOSPITAL_RESOURCE_ENGINE_H
#define AEGIS_HOSPITAL_RESOURCE_ENGINE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <memory>
#include <optional>
#include <chrono>

namespace aegis {
namespace hospital_management {

// =============================================================================
// Enumerations
// =============================================================================

/**
 * @brief Operational status of a hospital facility.
 */
enum class HospitalStatus {
    ONLINE,          ///< Hospital is fully operational and receiving patients
    OFFLINE,         ///< Hospital telemetry lost or communications offline
    FULL,            ///< Hospital beds and emergency department at capacity
    EMERGENCY_ONLY,  ///< Reserved strictly for critical life-threatening trauma/resuscitation
    MAINTENANCE      ///< Facility undergoing maintenance or decontamination; diversion active
};

/**
 * @brief Specialized clinical and trauma capabilities supported by a hospital.
 */
enum class HospitalCapability {
    GENERAL,         ///< General medical and inpatient surgical care
    TRAUMA,          ///< Level 1 / Level 2 Trauma center with dedicated trauma team
    CARDIAC,         ///< Cardiac catheterization lab (STEMI receiving center)
    PEDIATRIC,       ///< Dedicated pediatric emergency department and pediatric inpatient wards
    NEURO,           ///< Comprehensive stroke and neurosurgical intervention center
    BURNS,           ///< Specialized burn care and burn resuscitation unit
    ICU,             ///< Intensive Care Unit with critical care ventilation support
    NEONATAL,        ///< Neonatal Intensive Care Unit (NICU Level 3/4)
    ORTHOPEDIC,      ///< Specialized orthopedic trauma and surgical management
    MULTISPECIALITY  ///< Comprehensive quaternary facility supporting all primary clinical disciplines
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

// =============================================================================
// Data Models
// =============================================================================

/**
 * @brief Live bed availability metrics for inpatient wards.
 */
struct BedAvailability {
    uint32_t totalBeds{0};
    uint32_t availableBeds{0};
    uint32_t reservedBeds{0};
    uint32_t occupiedBeds{0};
    double occupancyRate{0.0}; ///< Inpatient occupancy percentage (0.0% - 100.0%)

    BedAvailability() = default;
    BedAvailability(uint32_t total, uint32_t avail, uint32_t reserved = 0, uint32_t occupied = 0)
        : totalBeds(total), availableBeds(avail), reservedBeds(reserved), occupiedBeds(occupied) {
        if (totalBeds > 0) {
            occupancyRate = (static_cast<double>(totalBeds - availableBeds) / static_cast<double>(totalBeds)) * 100.0;
        }
    }
};

/**
 * @brief Intensive Care Unit (ICU) bed metrics.
 */
struct ICUBed {
    uint32_t totalIcuBeds{0};
    uint32_t availableIcuBeds{0};
    uint32_t reservedIcuBeds{0};
    uint32_t occupiedIcuBeds{0};
    double icuOccupancyRate{0.0}; ///< ICU occupancy percentage (0.0% - 100.0%)

    ICUBed() = default;
    ICUBed(uint32_t total, uint32_t avail, uint32_t reserved = 0, uint32_t occupied = 0)
        : totalIcuBeds(total), availableIcuBeds(avail), reservedIcuBeds(reserved), occupiedIcuBeds(occupied) {
        if (totalIcuBeds > 0) {
            icuOccupancyRate = (static_cast<double>(totalIcuBeds - availableIcuBeds) / static_cast<double>(totalIcuBeds)) * 100.0;
        }
    }
};

/**
 * @brief Emergency Department (ED) operational capacity and queue telemetry.
 */
struct EmergencyDepartment {
    uint32_t capacity{0};                  ///< Total ER bay capacity
    uint32_t currentLoad{0};              ///< Active patients currently in ER
    uint32_t triageQueueLength{0};        ///< Patients waiting in triage queue
    double averageWaitTimeMinutes{0.0};    ///< Estimated wait time in minutes
    double occupancyPercentage{0.0};      ///< Current ER occupancy percentage (0.0% - 100.0%)
    bool diversionActive{false};          ///< Flag indicating emergency diversion status

    EmergencyDepartment() = default;
    EmergencyDepartment(uint32_t cap, uint32_t load, uint32_t queue = 0, double wait = 0.0, bool diversion = false)
        : capacity(cap), currentLoad(load), triageQueueLength(queue), averageWaitTimeMinutes(wait), diversionActive(diversion) {
        if (capacity > 0) {
            occupancyPercentage = (static_cast<double>(currentLoad) / static_cast<double>(capacity)) * 100.0;
        }
    }
};

/**
 * @brief Mechanical ventilator operational and allocation status.
 */
struct VentilatorStatus {
    uint32_t totalVentilators{0};
    uint32_t availableVentilators{0};
    uint32_t inUseVentilators{0};
    uint32_t reservedVentilators{0};

    VentilatorStatus() = default;
    VentilatorStatus(uint32_t total, uint32_t avail, uint32_t inUse = 0, uint32_t reserved = 0)
        : totalVentilators(total), availableVentilators(avail), inUseVentilators(inUse), reservedVentilators(reserved) {}
};

/**
 * @brief Medical physician and surgical specialist staffing availability.
 */
struct DoctorAvailability {
    uint32_t totalDoctors{0};
    uint32_t availableDoctors{0};
    uint32_t onDutyDoctors{0};
    uint32_t traumaSurgeonsAvailable{0};
    uint32_t cardiologistsAvailable{0};
    uint32_t neurologistsAvailable{0};
    uint32_t pediatriciansAvailable{0};

    DoctorAvailability() = default;
    DoctorAvailability(uint32_t total, uint32_t avail, uint32_t onDuty = 0,
                       uint32_t trauma = 0, uint32_t cardiac = 0, uint32_t neuro = 0, uint32_t ped = 0)
        : totalDoctors(total), availableDoctors(avail), onDutyDoctors(onDuty),
          traumaSurgeonsAvailable(trauma), cardiologistsAvailable(cardiac),
          neurologistsAvailable(neuro), pediatriciansAvailable(ped) {}
};

/**
 * @brief Comprehensive operational profile of a hospital facility.
 */
struct HospitalRecord {
    std::string hospitalId{""};
    std::string hospitalName{""};
    double latitude{0.0};
    double longitude{0.0};
    HospitalStatus status{HospitalStatus::ONLINE};
    std::vector<HospitalCapability> capabilities;
    BedAvailability bedAvailability;
    ICUBed icuBeds;
    EmergencyDepartment emergencyDept;
    VentilatorStatus ventilatorStatus;
    DoctorAvailability doctorAvailability;
    uint64_t lastUpdatedTimestamp{0};
    std::string contactPhone{""};
    std::string address{""};

    HospitalRecord() = default;
    HospitalRecord(std::string id, std::string name, double lat, double lon,
                   HospitalStatus st = HospitalStatus::ONLINE,
                   std::vector<HospitalCapability> caps = {HospitalCapability::GENERAL})
        : hospitalId(std::move(id)), hospitalName(std::move(name)), latitude(lat), longitude(lon),
          status(st), capabilities(std::move(caps)) {}

    bool HasCapability(HospitalCapability cap) const {
        for (const auto& c : capabilities) {
            if (c == cap || c == HospitalCapability::MULTISPECIALITY) {
                return true;
            }
        }
        return false;
    }
};

/**
 * @brief System-wide aggregation statistics for hospital resources.
 */
struct HospitalStatistics {
    uint32_t totalHospitalsRegistered{0};
    uint32_t onlineHospitals{0};
    uint32_t offlineHospitals{0};
    uint32_t fullHospitals{0};
    uint32_t emergencyOnlyHospitals{0};
    uint32_t maintenanceHospitals{0};
    uint32_t totalBeds{0};
    uint32_t availableBeds{0};
    uint32_t reservedBeds{0};
    uint32_t totalIcuBeds{0};
    uint32_t availableIcuBeds{0};
    uint32_t reservedIcuBeds{0};
    uint32_t totalVentilators{0};
    uint32_t availableVentilators{0};
    uint32_t totalDoctorsAvailable{0};
    uint64_t totalSelectionsMade{0};
    uint64_t totalBedsReserved{0};
    uint64_t totalIcuBedsReserved{0};
    uint64_t totalBedsReleased{0};
    uint64_t totalIcuBedsReleased{0};
    double averageSelectionLatencyMs{0.0};
};

/**
 * @brief Request parameters for destination hospital selection.
 */
struct HospitalSelection {
    std::string incidentId{""};
    std::string ambulanceId{""};
    double incidentLatitude{0.0};
    double incidentLongitude{0.0};
    HospitalCapability requiredSpecialty{HospitalCapability::GENERAL};
    bool requiresICU{false};
    bool requiresVentilator{false};
    uint32_t patientAcuityLevel{1}; ///< 1 = Critical/Resuscitation, 2 = Emergent, 3 = Urgent
    double currentSpeedKmH{50.0};
    uint64_t requestTimestamp{0};

    HospitalSelection() = default;
    HospitalSelection(std::string incId, std::string ambId, double lat, double lon,
                      HospitalCapability spec = HospitalCapability::GENERAL,
                      bool reqIcu = false, bool reqVent = false,
                      uint32_t acuity = 1, double speed = 50.0)
        : incidentId(std::move(incId)), ambulanceId(std::move(ambId)),
          incidentLatitude(lat), incidentLongitude(lon),
          requiredSpecialty(spec), requiresICU(reqIcu), requiresVentilator(reqVent),
          patientAcuityLevel(acuity), currentSpeedKmH(speed) {}
};

/**
 * @brief Destination recommendation result from the multi-criteria selection algorithm.
 */
struct HospitalRecommendation {
    std::string incidentId{""};
    std::string ambulanceId{""};
    std::string selectedHospitalId{""};
    std::string hospitalName{""};
    HospitalCapability matchedCapability{HospitalCapability::GENERAL};
    double distanceKm{0.0};
    double estimatedEtaMinutes{0.0};
    uint32_t availableBeds{0};
    uint32_t availableIcuBeds{0};
    uint32_t availableVentilators{0};
    uint32_t availableDoctors{0};
    double score{0.0};
    std::string recommendationSummary{""};
    std::string reason{""};
    bool successful{false};
    uint64_t decisionTimestamp{0};
    double calculationTimeMs{0.0};
};

// =============================================================================
// HospitalResourceEngine Class
// =============================================================================

/**
 * @class HospitalResourceEngine
 * @brief Production-grade Centralized Hospital Resource Allocation & Bed Management Engine.
 *
 * Thread-safe, deterministic, and QNX SDP 8.0 compliant.
 * Manages hospital registries, bed/ICU/ventilator allocations, real-time load,
 * and executes deterministic 10-rule destination hospital optimization under 5 ms.
 */
class HospitalResourceEngine {
public:
    HospitalResourceEngine();
    ~HospitalResourceEngine() = default;

    // Disallow copy/move to enforce thread safety and single identity
    HospitalResourceEngine(const HospitalResourceEngine&) = delete;
    HospitalResourceEngine& operator=(const HospitalResourceEngine&) = delete;
    HospitalResourceEngine(HospitalResourceEngine&&) = delete;
    HospitalResourceEngine& operator=(HospitalResourceEngine&&) = delete;

    // -------------------------------------------------------------------------
    // Hospital Management & Registry APIs
    // -------------------------------------------------------------------------

    /**
     * @brief Register a hospital facility in the centralized registry.
     * @param hospital Complete hospital profile.
     * @return true if successfully registered, false if invalid or ID already exists.
     */
    bool RegisterHospital(const HospitalRecord& hospital);

    /**
     * @brief Remove a hospital facility from the centralized registry.
     * @param hospitalId Unique facility identifier.
     * @return true if found and removed, false otherwise.
     */
    bool RemoveHospital(const std::string& hospitalId);

    /**
     * @brief Update geographical coordinates of a hospital facility.
     * @param hospitalId Unique facility identifier.
     * @param latitude Updated latitude in degrees.
     * @param longitude Updated longitude in degrees.
     * @return true if found and updated, false otherwise.
     */
    bool UpdateHospitalLocation(const std::string& hospitalId, double latitude, double longitude);

    /**
     * @brief Update inpatient bed availability for a hospital.
     */
    bool UpdateBedAvailability(const std::string& hospitalId, const BedAvailability& beds);
    bool UpdateBedAvailability(const std::string& hospitalId, uint32_t totalBeds, uint32_t availableBeds);

    /**
     * @brief Update Intensive Care Unit (ICU) beds for a hospital.
     */
    bool UpdateICUBeds(const std::string& hospitalId, const ICUBed& icuBeds);
    bool UpdateICUBeds(const std::string& hospitalId, uint32_t totalIcuBeds, uint32_t availableIcuBeds);

    /**
     * @brief Update Emergency Department capacity and queue load.
     */
    bool UpdateEmergencyCapacity(const std::string& hospitalId, const EmergencyDepartment& emergencyDept);
    bool UpdateEmergencyCapacity(const std::string& hospitalId, uint32_t capacity, uint32_t currentLoad, uint32_t queueLength = 0);

    /**
     * @brief Update medical doctor and surgeon availability.
     */
    bool UpdateDoctorAvailability(const std::string& hospitalId, const DoctorAvailability& doctors);
    bool UpdateDoctorAvailability(const std::string& hospitalId, uint32_t totalDoctors, uint32_t availableDoctors);

    /**
     * @brief Update mechanical ventilator inventory and operational status.
     */
    bool UpdateVentilatorStatus(const std::string& hospitalId, const VentilatorStatus& ventStatus);
    bool UpdateVentilatorStatus(const std::string& hospitalId, uint32_t totalVentilators, uint32_t availableVentilators);

    /**
     * @brief Update clinical capabilities and medical specialties supported.
     */
    bool UpdateSpecialties(const std::string& hospitalId, const std::vector<HospitalCapability>& specialties);

    /**
     * @brief Update operational status of a hospital facility.
     */
    bool UpdateStatus(const std::string& hospitalId, HospitalStatus status);

    // -------------------------------------------------------------------------
    // Query & Optimization APIs
    // -------------------------------------------------------------------------

    /**
     * @brief Find the geographically nearest hospital facility.
     * @param latitude Query latitude in degrees.
     * @param longitude Query longitude in degrees.
     * @param outHospital Output parameter populated with nearest facility record.
     * @param requiredCapability Optional filter for clinical capability.
     * @return true if a matching facility was found, false otherwise.
     */
    bool FindNearestHospital(double latitude, double longitude,
                             HospitalRecord& outHospital,
                             HospitalCapability requiredCapability = HospitalCapability::GENERAL) const;

    std::optional<HospitalRecord> FindNearestHospital(double latitude, double longitude,
                                                      HospitalCapability requiredCapability = HospitalCapability::GENERAL) const;

    /**
     * @brief Automatically select the best destination hospital for an ambulance.
     * Executes deterministic 10-rule multi-factor scoring.
     * @param request Hospital selection criteria and patient condition.
     * @return HospitalRecommendation detailing optimal destination and structured reason.
     */
    HospitalRecommendation FindBestHospital(const HospitalSelection& request);

    // -------------------------------------------------------------------------
    // Reservation Management APIs
    // -------------------------------------------------------------------------

    /**
     * @brief Reserve a standard inpatient bed for an inbound patient.
     * @param hospitalId Unique facility identifier.
     * @param reservationId Optional tracking ID for reservation.
     * @return true if bed was successfully reserved, false if capacity exhausted or hospital not found.
     */
    bool ReserveBed(const std::string& hospitalId, const std::string& reservationId = "");

    /**
     * @brief Release a previously reserved standard bed.
     * @param hospitalId Unique facility identifier.
     * @param reservationId Optional tracking ID for reservation.
     * @return true if successfully released, false otherwise.
     */
    bool ReleaseBed(const std::string& hospitalId, const std::string& reservationId = "");

    /**
     * @brief Reserve an ICU bed for an inbound critical patient.
     * @param hospitalId Unique facility identifier.
     * @param reservationId Optional tracking ID for reservation.
     * @return true if ICU bed reserved, false if exhausted or hospital not found.
     */
    bool ReserveICUBed(const std::string& hospitalId, const std::string& reservationId = "");

    /**
     * @brief Release a previously reserved ICU bed.
     * @param hospitalId Unique facility identifier.
     * @param reservationId Optional tracking ID for reservation.
     * @return true if successfully released, false otherwise.
     */
    bool ReleaseICUBed(const std::string& hospitalId, const std::string& reservationId = "");

    // -------------------------------------------------------------------------
    // Reporting & Statistics APIs
    // -------------------------------------------------------------------------

    /**
     * @brief Generate structured hospital status and recommendation report.
     * Matches exact SMART AEGIS [HOSPITAL] specification format.
     */
    std::string GenerateHospitalReport(const std::string& hospitalId) const;
    std::string GenerateHospitalReport(const HospitalRecommendation& recommendation) const;

    /**
     * @brief Retrieve system-wide hospital statistics.
     */
    HospitalStatistics GetStatistics() const;

    /**
     * @brief Retrieve current status of a specific hospital.
     */
    bool GetHospitalStatus(const std::string& hospitalId, HospitalStatus& outStatus) const;
    HospitalStatus GetHospitalStatus(const std::string& hospitalId) const;

    /**
     * @brief Retrieve a specific hospital profile.
     */
    bool GetHospital(const std::string& hospitalId, HospitalRecord& outHospital) const;

    /**
     * @brief Retrieve snapshot of all registered hospitals.
     */
    std::vector<HospitalRecord> GetAllHospitals() const;

    /**
     * @brief Clear selection history, reports, and reset statistics.
     */
    void ClearHistory();

    /**
     * @brief Reset entire engine to clean initial state.
     */
    void Reset();

    // -------------------------------------------------------------------------
    // Utilities
    // -------------------------------------------------------------------------

    /**
     * @brief Calculate geodesic Haversine distance in kilometers between two GPS points.
     */
    static double CalculateDistanceKm(double lat1, double lon1, double lat2, double lon2);

private:
    struct ScoredCandidate {
        std::string hospitalId;
        std::string hospitalName;
        HospitalCapability matchedCapability{HospitalCapability::GENERAL};
        double distanceKm{0.0};
        double etaMinutes{0.0};
        double score{0.0};
        std::string recommendationSummary;
        std::string reason;
        uint32_t availableBeds{0};
        uint32_t availableIcuBeds{0};
        uint32_t availableVentilators{0};
        uint32_t availableDoctors{0};
    };

    ScoredCandidate EvaluateCandidate(const HospitalRecord& hospital, const HospitalSelection& request) const;
    uint64_t GetCurrentTimeMs() const;

    // Concurrency synchronization mutexes
    mutable std::mutex registryMutex_;      ///< Protects hospital registry (hospitals_)
    mutable std::mutex statisticsMutex_;    ///< Protects system statistics (statistics_)
    mutable std::mutex reportsMutex_;       ///< Protects generated audit reports (reports_)
    mutable std::mutex reservationsMutex_;  ///< Protects bed & ICU reservations (reservations_)
    mutable std::mutex historyMutex_;       ///< Protects decision history (history_)

    // Internal State Data Stores
    std::unordered_map<std::string, HospitalRecord> hospitals_;
    std::unordered_map<std::string, std::string> bedReservations_;
    std::unordered_map<std::string, std::string> icuReservations_;
    std::vector<HospitalRecommendation> history_;
    std::vector<std::string> reports_;
    HospitalStatistics statistics_;
    uint64_t reservationCounter_{0};
};

} // namespace hospital_management
} // namespace aegis

#endif // AEGIS_HOSPITAL_RESOURCE_ENGINE_H

