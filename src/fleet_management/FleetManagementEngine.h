/**
 * @file FleetManagementEngine.h
 * @brief Intelligent Ambulance Fleet Management & Dispatch Engine for SMART AEGIS (Phase 11A).
 *
 * Provides centralized management of municipal ambulance fleets, acting as the single
 * source of truth for ambulance status, capability, location, heartbeat freshness,
 * and deterministic multi-criteria dispatch selection.
 */

#ifndef AEGIS_FLEET_MANAGEMENT_ENGINE_H
#define AEGIS_FLEET_MANAGEMENT_ENGINE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <chrono>
#include <optional>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace aegis {
namespace fleet {

// =============================================================================
// Data Models & Enumerations
// =============================================================================

/**
 * @brief Operational status of an emergency response vehicle.
 */
enum class AmbulanceStatus {
    AVAILABLE,      ///< In service, ready for dispatch
    DISPATCHED,     ///< En route to incident location
    AT_PATIENT,     ///< On scene providing patient care
    TRANSPORTING,   ///< En route to receiving hospital with patient
    AT_HOSPITAL,    ///< Transferring patient care at hospital
    RETURNING,      ///< Returning to station or sector
    MAINTENANCE,    ///< Out of service for mechanical/equipment servicing
    OFFLINE         ///< Communication lost / heartbeat timed out
};

/**
 * @brief Medical care capability tier of an ambulance.
 */
enum class AmbulanceCapability {
    BASIC_LIFE_SUPPORT,     ///< BLS: EMT-Basic (CPR, basic airway, splinting)
    ADVANCED_LIFE_SUPPORT,  ///< ALS: Paramedic (IV, intubation, ECG, cardiac meds)
    ICU,                    ///< Mobile Intensive Care: Critical Care Transport
    CARDIAC,                ///< Specialized mobile coronary / STEMI care
    PEDIATRIC,              ///< Specialized pediatric life support
    NEONATAL                ///< Specialized neonatal transport incubator
};

/**
 * @brief Comprehensive record describing a single ambulance in the city fleet.
 */
struct AmbulanceRecord {
    std::string ambulanceId{""};
    std::string crewId{""};
    std::string driverName{""};
    double currentLatitude{0.0};
    double currentLongitude{0.0};
    AmbulanceStatus status{AmbulanceStatus::AVAILABLE};
    AmbulanceCapability capability{AmbulanceCapability::BASIC_LIFE_SUPPORT};
    double fuelLevel{100.0};              ///< Fuel percentage: 0.0 - 100.0%
    double batteryLevel{100.0};           ///< Battery percentage: 0.0 - 100.0%
    double currentSpeed{0.0};              ///< Current vehicle speed in km/h
    double remainingRange{500.0};          ///< Estimated driving range in km
    std::string assignedHospital{""};
    std::string assignedIncident{""};
    uint64_t lastHeartbeat{0};             ///< Timestamp in milliseconds (UTC or monotonic)
    uint64_t availabilityTimestamp{0};     ///< Timestamp when vehicle became available
    double totalDistanceToday{0.0};        ///< Total distance driven today in km
    uint32_t totalPatientsHandled{0};      ///< Count of patients treated/transported today
    bool maintenanceDue{false};            ///< Flag indicating scheduled/urgent maintenance needed

    AmbulanceRecord() = default;

    AmbulanceRecord(std::string id,
                    std::string crew,
                    std::string driver,
                    double lat,
                    double lon,
                    AmbulanceStatus st = AmbulanceStatus::AVAILABLE,
                    AmbulanceCapability cap = AmbulanceCapability::BASIC_LIFE_SUPPORT,
                    double fuel = 100.0,
                    double battery = 100.0,
                    double speed = 0.0,
                    double range = 500.0,
                    uint64_t heartbeat = 0)
        : ambulanceId(std::move(id)),
          crewId(std::move(crew)),
          driverName(std::move(driver)),
          currentLatitude(lat),
          currentLongitude(lon),
          status(st),
          capability(cap),
          fuelLevel(fuel),
          batteryLevel(battery),
          currentSpeed(speed),
          remainingRange(range),
          lastHeartbeat(heartbeat) {}
};

/**
 * @brief Criteria specifying requirements for an emergency incident response.
 */
struct DispatchRequest {
    std::string incidentId{""};
    double incidentLatitude{0.0};
    double incidentLongitude{0.0};
    AmbulanceCapability requiredCapability{AmbulanceCapability::BASIC_LIFE_SUPPORT};
    std::string targetHospital{""};
    double minimumFuelPercent{15.0};
    double maxDistanceKm{100.0};

    DispatchRequest() = default;
    DispatchRequest(std::string incId,
                    double lat,
                    double lon,
                    AmbulanceCapability reqCap = AmbulanceCapability::BASIC_LIFE_SUPPORT,
                    std::string hospital = "",
                    double minFuel = 15.0)
        : incidentId(std::move(incId)),
          incidentLatitude(lat),
          incidentLongitude(lon),
          requiredCapability(reqCap),
          targetHospital(std::move(hospital)),
          minimumFuelPercent(minFuel) {}
};

/**
 * @brief Evaluation result for a candidate ambulance ranked during dispatch.
 */
struct RankedAmbulance {
    std::string ambulanceId{""};
    AmbulanceCapability capability{AmbulanceCapability::BASIC_LIFE_SUPPORT};
    double distanceKm{0.0};
    double etaMinutes{0.0};
    double compositeScore{0.0};            ///< Lower score indicates higher dispatch priority
    bool eligible{false};
    std::string ineligibilityReason{""};
};

/**
 * @brief Final dispatch assignment decision generated by the fleet engine.
 */
struct DispatchAssignment {
    std::string incidentId{""};
    std::string assignedAmbulanceId{""};
    AmbulanceCapability capability{AmbulanceCapability::BASIC_LIFE_SUPPORT};
    double distanceKm{0.0};
    double etaMinutes{0.0};
    double rankingScore{0.0};
    bool assigned{false};
    std::string reason{""};
    AmbulanceRecord vehicleSnapshot{};
};

/**
 * @brief Aggregate fleet operational statistics.
 */
struct FleetStatistics {
    uint32_t totalVehicles{0};
    uint32_t availableCount{0};
    uint32_t dispatchedCount{0};
    uint32_t atPatientCount{0};
    uint32_t transportingCount{0};
    uint32_t atHospitalCount{0};
    uint32_t returningCount{0};
    uint32_t maintenanceCount{0};
    uint32_t offlineCount{0};
    uint32_t busyCount{0};                 ///< Sum of dispatched, at_patient, transporting, at_hospital, returning
    double averageFuelLevel{0.0};
    double averageBatteryLevel{0.0};
    double totalDistanceTodayKm{0.0};
    uint32_t totalPatientsHandledToday{0};
};

// =============================================================================
// Helper Functions
// =============================================================================

inline std::string AmbulanceStatusToString(AmbulanceStatus status) {
    switch (status) {
        case AmbulanceStatus::AVAILABLE:    return "AVAILABLE";
        case AmbulanceStatus::DISPATCHED:   return "DISPATCHED";
        case AmbulanceStatus::AT_PATIENT:   return "AT_PATIENT";
        case AmbulanceStatus::TRANSPORTING: return "TRANSPORTING";
        case AmbulanceStatus::AT_HOSPITAL:  return "AT_HOSPITAL";
        case AmbulanceStatus::RETURNING:    return "RETURNING";
        case AmbulanceStatus::MAINTENANCE:  return "MAINTENANCE";
        case AmbulanceStatus::OFFLINE:      return "OFFLINE";
    }
    return "UNKNOWN";
}

inline AmbulanceStatus StringToAmbulanceStatus(const std::string& str) {
    if (str == "AVAILABLE")    return AmbulanceStatus::AVAILABLE;
    if (str == "DISPATCHED")   return AmbulanceStatus::DISPATCHED;
    if (str == "AT_PATIENT")   return AmbulanceStatus::AT_PATIENT;
    if (str == "TRANSPORTING") return AmbulanceStatus::TRANSPORTING;
    if (str == "AT_HOSPITAL")  return AmbulanceStatus::AT_HOSPITAL;
    if (str == "RETURNING")    return AmbulanceStatus::RETURNING;
    if (str == "MAINTENANCE")  return AmbulanceStatus::MAINTENANCE;
    return AmbulanceStatus::OFFLINE;
}

inline std::string AmbulanceCapabilityToString(AmbulanceCapability cap) {
    switch (cap) {
        case AmbulanceCapability::BASIC_LIFE_SUPPORT:    return "BASIC_LIFE_SUPPORT";
        case AmbulanceCapability::ADVANCED_LIFE_SUPPORT: return "ADVANCED_LIFE_SUPPORT";
        case AmbulanceCapability::ICU:                   return "ICU";
        case AmbulanceCapability::CARDIAC:               return "CARDIAC";
        case AmbulanceCapability::PEDIATRIC:             return "PEDIATRIC";
        case AmbulanceCapability::NEONATAL:              return "NEONATAL";
    }
    return "UNKNOWN";
}

inline std::string CapabilityToShortString(AmbulanceCapability cap) {
    switch (cap) {
        case AmbulanceCapability::BASIC_LIFE_SUPPORT:    return "BLS";
        case AmbulanceCapability::ADVANCED_LIFE_SUPPORT: return "ALS";
        case AmbulanceCapability::ICU:                   return "ICU";
        case AmbulanceCapability::CARDIAC:               return "CARDIAC";
        case AmbulanceCapability::PEDIATRIC:             return "PEDIATRIC";
        case AmbulanceCapability::NEONATAL:              return "NEONATAL";
    }
    return "UNKNOWN";
}

inline AmbulanceCapability StringToAmbulanceCapability(const std::string& str) {
    if (str == "BASIC_LIFE_SUPPORT" || str == "BLS")       return AmbulanceCapability::BASIC_LIFE_SUPPORT;
    if (str == "ADVANCED_LIFE_SUPPORT" || str == "ALS")    return AmbulanceCapability::ADVANCED_LIFE_SUPPORT;
    if (str == "ICU")                                      return AmbulanceCapability::ICU;
    if (str == "CARDIAC")                                  return AmbulanceCapability::CARDIAC;
    if (str == "PEDIATRIC")                                return AmbulanceCapability::PEDIATRIC;
    if (str == "NEONATAL")                                 return AmbulanceCapability::NEONATAL;
    return AmbulanceCapability::BASIC_LIFE_SUPPORT;
}

/**
 * @brief Checks if an ambulance capability satisfies or exceeds the required capability.
 */
inline bool CanSatisfyCapability(AmbulanceCapability vehicleCap, AmbulanceCapability requiredCap) {
    if (vehicleCap == requiredCap) return true;
    if (vehicleCap == AmbulanceCapability::ICU) return true;
    if (vehicleCap == AmbulanceCapability::ADVANCED_LIFE_SUPPORT) {
        return (requiredCap == AmbulanceCapability::BASIC_LIFE_SUPPORT ||
                requiredCap == AmbulanceCapability::ADVANCED_LIFE_SUPPORT);
    }
    if (vehicleCap == AmbulanceCapability::CARDIAC) {
        return (requiredCap == AmbulanceCapability::BASIC_LIFE_SUPPORT ||
                requiredCap == AmbulanceCapability::ADVANCED_LIFE_SUPPORT ||
                requiredCap == AmbulanceCapability::CARDIAC);
    }
    if (vehicleCap == AmbulanceCapability::PEDIATRIC) {
        return (requiredCap == AmbulanceCapability::BASIC_LIFE_SUPPORT ||
                requiredCap == AmbulanceCapability::PEDIATRIC);
    }
    if (vehicleCap == AmbulanceCapability::NEONATAL) {
        return (requiredCap == AmbulanceCapability::BASIC_LIFE_SUPPORT ||
                requiredCap == AmbulanceCapability::NEONATAL);
    }
    return false;
}

// =============================================================================
// Engine Interface
// =============================================================================

/**
 * @class FleetManagementEngine
 * @brief Thread-safe centralized fleet management and dispatch decision engine.
 */
class FleetManagementEngine {
public:
    FleetManagementEngine();
    ~FleetManagementEngine() = default;

    // Prevent copy/move semantics for thread-safe singleton-like safety
    FleetManagementEngine(const FleetManagementEngine&) = delete;
    FleetManagementEngine& operator=(const FleetManagementEngine&) = delete;
    FleetManagementEngine(FleetManagementEngine&&) = delete;
    FleetManagementEngine& operator=(FleetManagementEngine&&) = delete;

    // -------------------------------------------------------------------------
    // Core Registration & Lifecycle Public APIs
    // -------------------------------------------------------------------------

    /**
     * @brief Registers a new ambulance vehicle in the municipal fleet registry.
     * @param ambulance The ambulance record containing identity, capability, and telemetry.
     * @return true if registered successfully; false if vehicle ID already exists or invalid.
     */
    bool RegisterAmbulance(const AmbulanceRecord& ambulance);

    /**
     * @brief Removes an ambulance vehicle from the fleet registry.
     * @param ambulanceId Unique identifier of the vehicle to remove.
     * @return true if found and removed; false otherwise.
     */
    bool RemoveAmbulance(const std::string& ambulanceId);

    /**
     * @brief Queries vehicle record by ID.
     * @param ambulanceId Unique identifier of vehicle.
     * @param outAmbulance Output parameter receiving the record snapshot.
     * @return true if vehicle exists; false otherwise.
     */
    bool GetAmbulance(const std::string& ambulanceId, AmbulanceRecord& outAmbulance) const;

    /**
     * @brief Updates geographic position and speed of an ambulance.
     * @param ambulanceId Unique identifier of vehicle.
     * @param latitude Current latitude in decimal degrees.
     * @param longitude Current longitude in decimal degrees.
     * @param speedKmH Current vehicle velocity in km/h.
     * @return true if updated; false if vehicle not found.
     */
    bool UpdateLocation(const std::string& ambulanceId, double latitude, double longitude, double speedKmH = 0.0);

    /**
     * @brief Updates operational status of an ambulance.
     * @param ambulanceId Unique identifier of vehicle.
     * @param newStatus New status to set.
     * @return true if updated; false if vehicle not found.
     */
    bool UpdateStatus(const std::string& ambulanceId, AmbulanceStatus newStatus);

    /**
     * @brief Updates heartbeat timestamp of an ambulance, recovering it if previously offline.
     * @param ambulanceId Unique identifier of vehicle.
     * @param timestampMs Monotonic or UTC timestamp in milliseconds (0 = current monotonic time).
     * @return true if updated; false if vehicle not found.
     */
    bool UpdateHeartbeat(const std::string& ambulanceId, uint64_t timestampMs = 0);

    /**
     * @brief Sets vehicle maintenance due flag.
     * @param ambulanceId Unique identifier of vehicle.
     * @param due True if maintenance is required.
     * @return true if updated; false if vehicle not found.
     */
    bool SetMaintenanceDue(const std::string& ambulanceId, bool due);

    /**
     * @brief Sets vehicle fuel and battery levels.
     * @param ambulanceId Unique identifier of vehicle.
     * @param fuel Fuel percentage (0.0 - 100.0).
     * @param battery Battery percentage (0.0 - 100.0).
     * @param remainingRangeKm Estimated remaining driving distance in km.
     * @return true if updated; false if vehicle not found.
     */
    bool UpdateEnergy(const std::string& ambulanceId, double fuel, double battery, double remainingRangeKm);

    // -------------------------------------------------------------------------
    // Dispatch Public APIs
    // -------------------------------------------------------------------------

    /**
     * @brief Formally assigns an ambulance to respond to an emergency incident.
     * @param ambulanceId Vehicle identifier to assign.
     * @param incidentId Incident identifier.
     * @param hospitalId Designated receiving hospital identifier (optional).
     * @return true if assigned; false if vehicle is not available, in maintenance, or offline.
     */
    bool AssignIncident(const std::string& ambulanceId, const std::string& incidentId, const std::string& hospitalId = "");

    /**
     * @brief Releases an ambulance from incident assignment back into available service.
     * @param ambulanceId Vehicle identifier to release.
     * @param hospitalId Final hospital where incident was completed.
     * @param addedDistanceKm Additional distance driven during the mission in km.
     * @return true if released; false if vehicle not found.
     */
    bool ReleaseAmbulance(const std::string& ambulanceId, const std::string& hospitalId = "", double addedDistanceKm = 0.0);

    /**
     * @brief Finds the geographically nearest available ambulance meeting capability requirements.
     * @param latitude Target incident latitude.
     * @param longitude Target incident longitude.
     * @param capability Minimum capability required.
     * @return DispatchAssignment containing nearest ambulance details, or unassigned if none available.
     */
    DispatchAssignment FindNearestAvailable(double latitude, double longitude, AmbulanceCapability capability = AmbulanceCapability::BASIC_LIFE_SUPPORT);

    /**
     * @brief Evaluates multi-criteria dispatch algorithm to select the best ambulance for an incident.
     * @param incidentId Unique incident identifier.
     * @param incidentLat Incident latitude coordinate.
     * @param incidentLon Incident longitude coordinate.
     * @param requiredCapability Medical capability required for patient.
     * @param hospitalId Destination hospital identifier (optional).
     * @return DispatchAssignment with optimal ambulance, calculated ETA, and audit justification.
     */
    DispatchAssignment FindBestAmbulance(const std::string& incidentId,
                                        double incidentLat,
                                        double incidentLon,
                                        AmbulanceCapability requiredCapability = AmbulanceCapability::BASIC_LIFE_SUPPORT,
                                        const std::string& hospitalId = "");

    /**
     * @brief Overload accepting DispatchRequest criteria.
     */
    DispatchAssignment FindBestAmbulance(const DispatchRequest& request);

    /**
     * @brief Produces a deterministically ranked list of all eligible ambulances for an incident.
     * @param incidentLat Incident latitude coordinate.
     * @param incidentLon Incident longitude coordinate.
     * @param requiredCapability Medical capability required.
     * @return Vector of RankedAmbulance sorted by priority (lowest composite score first).
     */
    std::vector<RankedAmbulance> RankAmbulances(double incidentLat,
                                               double incidentLon,
                                               AmbulanceCapability requiredCapability);

    // -------------------------------------------------------------------------
    // Fleet Inspection & Reporting Public APIs
    // -------------------------------------------------------------------------

    /**
     * @brief Retrieves a snapshot of the entire fleet.
     * @return Vector of all registered ambulance records.
     */
    std::vector<AmbulanceRecord> GetFleetStatus() const;

    /**
     * @brief Retrieves all ambulances currently in AVAILABLE status and ready for dispatch.
     */
    std::vector<AmbulanceRecord> GetAvailableAmbulances() const;

    /**
     * @brief Retrieves all ambulances currently active on a mission (DISPATCHED, AT_PATIENT, TRANSPORTING, AT_HOSPITAL, RETURNING).
     */
    std::vector<AmbulanceRecord> GetBusyAmbulances() const;

    /**
     * @brief Retrieves all ambulances currently marked OFFLINE.
     */
    std::vector<AmbulanceRecord> GetOfflineAmbulances() const;

    /**
     * @brief Retrieves all ambulances currently in MAINTENANCE status or marked maintenanceDue.
     */
    std::vector<AmbulanceRecord> GetMaintenanceVehicles() const;

    /**
     * @brief Computes aggregate operational fleet metrics and counts.
     */
    FleetStatistics GetFleetStatistics() const;

    /**
     * @brief Generates a comprehensive human-readable fleet report.
     */
    std::string GenerateFleetReport() const;

    /**
     * @brief Scans all vehicles and automatically flags any whose lastHeartbeat exceeds timeout as OFFLINE.
     * @param currentTimestampMs Current timestamp (0 = uses monotonic time).
     * @return Number of vehicles newly transitioned to OFFLINE.
     */
    uint32_t CheckHeartbeatTimeouts(uint64_t currentTimestampMs = 0);

    /**
     * @brief Sets the heartbeat timeout duration in seconds.
     * @param seconds Timeout threshold (default = 30 seconds).
     */
    void SetHeartbeatTimeoutSeconds(uint32_t seconds);

    /**
     * @brief Gets current heartbeat timeout threshold in seconds.
     */
    uint32_t GetHeartbeatTimeoutSeconds() const;

    /**
     * @brief Sets simulated monotonic time for testing and simulations (0 = real system time).
     * @param ms Monotonic time in milliseconds.
     */
    void SetSimulatedTimeMs(uint64_t ms);

    /**
     * @brief Gets simulated monotonic time in milliseconds (0 = real system time active).
     */
    uint64_t GetSimulatedTimeMs() const;

    /**
     * @brief Formats structured audit log entry matching specification.
     */
    static std::string FormatAuditLog(const std::string& incidentId,
                                      const std::string& ambulanceId,
                                      AmbulanceCapability cap,
                                      double etaMin,
                                      const std::string& reason);

    /**
     * @brief Returns complete history of structured audit logs emitted.
     */
    std::vector<std::string> GetAuditLogs() const;

    /**
     * @brief Clears audit log history.
     */
    void ClearHistory();

    /**
     * @brief Resets entire fleet registry and audit logs.
     */
    void ClearAll();

private:
    // Internal helper methods (called with m_mutex held)
    static double CalculateDistanceKmInternal(double lat1, double lon1, double lat2, double lon2);
    static double CalculateEtaMinutesInternal(double distanceKm, double speedKmH);
    double ScoreAmbulanceInternal(const AmbulanceRecord& record,
                                  double incidentLat,
                                  double incidentLon,
                                  AmbulanceCapability reqCap,
                                  double& outDistanceKm,
                                  double& outEtaMin,
                                  bool& outEligible,
                                  std::string& outReason) const;
    void CheckHeartbeatTimeoutsInternal(uint64_t nowMs);
    uint64_t GetCurrentTimeMsInternal() const;

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, AmbulanceRecord> m_ambulances;
    std::vector<std::string> m_auditLogs;
    uint32_t m_heartbeatTimeoutSeconds{30};
    double m_defaultEmergencySpeedKmH{50.0};
    uint64_t m_simulatedTimeMs{0};
};

} // namespace fleet
} // namespace aegis

#endif // AEGIS_FLEET_MANAGEMENT_ENGINE_H
