/**
 * @file EmsFleetManager.hpp
 * @brief EMS Fleet Management subsystem tracking inventory, ambulance types, and operational states.
 * Automatically synchronizes with emergency mission assignments and completions.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <cstdint>
#include <optional>

namespace aegis::command_center {

enum class AmbulanceVehicleType {
    ICU_AMBULANCE,
    ALS_AMBULANCE,
    BLS_AMBULANCE,
    NEONATAL_AMBULANCE,
    CARDIAC_AMBULANCE,
    BIKE_RESPONDER
};

inline std::string AmbulanceVehicleTypeToString(AmbulanceVehicleType type) {
    switch (type) {
        case AmbulanceVehicleType::ICU_AMBULANCE:      return "ICU Ambulance";
        case AmbulanceVehicleType::ALS_AMBULANCE:      return "ALS Ambulance";
        case AmbulanceVehicleType::BLS_AMBULANCE:      return "BLS Ambulance";
        case AmbulanceVehicleType::NEONATAL_AMBULANCE: return "Neonatal Ambulance";
        case AmbulanceVehicleType::CARDIAC_AMBULANCE:  return "Cardiac Ambulance";
        case AmbulanceVehicleType::BIKE_RESPONDER:    return "Bike Responder";
        default:                                       return "Standard Ambulance";
    }
}

enum class AmbulanceStatus {
    AVAILABLE,
    ASSIGNED,
    EN_ROUTE_PATIENT,
    PATIENT_ON_BOARD,
    EN_ROUTE_HOSPITAL,
    RETURNING,
    OUT_OF_SERVICE,
    MAINTENANCE
};

inline std::string AmbulanceStatusToString(AmbulanceStatus status) {
    switch (status) {
        case AmbulanceStatus::AVAILABLE:         return "AVAILABLE";
        case AmbulanceStatus::ASSIGNED:          return "ASSIGNED";
        case AmbulanceStatus::EN_ROUTE_PATIENT:   return "EN_ROUTE_PATIENT";
        case AmbulanceStatus::PATIENT_ON_BOARD:   return "PATIENT_ON_BOARD";
        case AmbulanceStatus::EN_ROUTE_HOSPITAL:  return "EN_ROUTE_HOSPITAL";
        case AmbulanceStatus::RETURNING:         return "RETURNING";
        case AmbulanceStatus::OUT_OF_SERVICE:    return "OUT_OF_SERVICE";
        case AmbulanceStatus::MAINTENANCE:       return "MAINTENANCE";
        default:                                 return "UNKNOWN";
    }
}

struct AmbulanceRecord {
    std::string ambulanceId;
    std::string registrationNumber;
    AmbulanceVehicleType vehicleType{AmbulanceVehicleType::ALS_AMBULANCE};
    std::string currentDriver{"Officer Unassigned"};
    std::string currentParamedic{"Medic Unassigned"};
    AmbulanceStatus currentStatus{AmbulanceStatus::AVAILABLE};
    std::string assignedIncident;
    double latitude{12.9716};
    double longitude{77.5946};
    std::string assignedHospital;
    double etaSeconds{0.0};
    double fuelLevelPercent{100.0};
    std::string healthStatus{"OPTIMAL"};
    std::string lastServiceDate{"2026-10-01"};
    uint32_t totalMissionsCompleted{0};
};

struct FleetInventorySummary {
    size_t totalAmbulances{0};
    size_t availableCount{0};
    size_t assignedCount{0};
    size_t enRouteHospitalCount{0};
    size_t returningCount{0};
    size_t maintenanceCount{0};
    size_t outOfServiceCount{0};
};

class EmsFleetManager {
public:
    EmsFleetManager();
    ~EmsFleetManager() = default;

    // Prevent copying
    EmsFleetManager(const EmsFleetManager&) = delete;
    EmsFleetManager& operator=(const EmsFleetManager&) = delete;

    /**
     * @brief Populate realistic default ambulance fleet for emergency demonstration.
     */
    void InitializeDefaultFleet();

    /**
     * @brief Register or update an ambulance record.
     */
    bool RegisterAmbulance(const AmbulanceRecord& ambulance);

    /**
     * @brief Update the status of a specific vehicle.
     */
    bool UpdateStatus(const std::string& ambulanceId, AmbulanceStatus newStatus);

    /**
     * @brief Assign ambulance to an emergency incident and hospital.
     */
    bool AssignToMission(const std::string& ambulanceId,
                         const std::string& incidentId,
                         const std::string& hospitalId = "",
                         double etaSec = 0.0);

    /**
     * @brief Complete mission, increment counters, and set vehicle to RETURNING / AVAILABLE.
     */
    bool CompleteMission(const std::string& ambulanceId);

    /**
     * @brief Update live telemetry for a vehicle.
     */
    bool UpdateTelemetry(const std::string& ambulanceId,
                         double lat,
                         double lon,
                         double etaSec,
                         double fuelPercent = -1.0);

    /**
     * @brief Fetch single vehicle record.
     */
    std::optional<AmbulanceRecord> GetAmbulance(const std::string& ambulanceId) const;

    /**
     * @brief Retrieve snapshot of all ambulances in fleet.
     */
    std::vector<AmbulanceRecord> GetAllAmbulances() const;

    /**
     * @brief Query ambulances filtered by status.
     */
    std::vector<AmbulanceRecord> GetAmbulancesByStatus(AmbulanceStatus status) const;

    /**
     * @brief Aggregate fleet counts across all operational categories.
     */
    FleetInventorySummary GetInventorySummary() const;

    /**
     * @brief Clear all fleet records.
     */
    void Clear();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, AmbulanceRecord> fleet_;
};

} // namespace aegis::command_center
