/**
 * @file EmsFleetManager.cpp
 * @brief Implementation of EMS Fleet Management subsystem.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "command_center/fleet/EmsFleetManager.hpp"

namespace aegis::command_center {

EmsFleetManager::EmsFleetManager() {
    InitializeDefaultFleet();
}

void EmsFleetManager::InitializeDefaultFleet() {
    std::lock_guard<std::mutex> lock(mutex_);
    fleet_.clear();

    auto add = [this](const std::string& id, const std::string& reg, AmbulanceVehicleType type,
                      const std::string& driver, const std::string& medic,
                      AmbulanceStatus status, double fuel, const std::string& serviceDate) {
        AmbulanceRecord rec;
        rec.ambulanceId = id;
        rec.registrationNumber = reg;
        rec.vehicleType = type;
        rec.currentDriver = driver;
        rec.currentParamedic = medic;
        rec.currentStatus = status;
        rec.fuelLevelPercent = fuel;
        rec.healthStatus = (status == AmbulanceStatus::MAINTENANCE) ? "SERVICING" : "OPTIMAL";
        rec.lastServiceDate = serviceDate;
        fleet_[id] = rec;
    };

    // 12 Ambulance Fleet (Matches 8 Available, 3 Assigned, 1 Maintenance)
    add("AMB-01", "KA-01-EA-1001", AmbulanceVehicleType::CARDIAC_AMBULANCE,  "S. Kumar",      "Dr. A. Verma",   AmbulanceStatus::AVAILABLE, 92.0, "2026-10-10");
    add("AMB-02", "KA-01-EA-1002", AmbulanceVehicleType::ALS_AMBULANCE,      "R. Nair",       "P. Sharma",      AmbulanceStatus::AVAILABLE, 88.0, "2026-10-12");
    add("AMB-03", "KA-01-EA-1003", AmbulanceVehicleType::ICU_AMBULANCE,      "V. Patil",      "Dr. M. Joseph",  AmbulanceStatus::AVAILABLE, 95.0, "2026-10-14");
    add("AMB-04", "KA-01-EA-1004", AmbulanceVehicleType::NEONATAL_AMBULANCE, "D. Fernandes",  "S. Das",         AmbulanceStatus::AVAILABLE, 85.0, "2026-10-08");
    add("AMB-05", "KA-01-EA-1005", AmbulanceVehicleType::BIKE_RESPONDER,     "M. Rao",        "M. Rao",         AmbulanceStatus::AVAILABLE, 98.0, "2026-10-15");
    add("AMB-06", "KA-01-EA-1006", AmbulanceVehicleType::BLS_AMBULANCE,      "J. Anthony",    "K. Gowda",       AmbulanceStatus::AVAILABLE, 79.0, "2026-09-28");
    add("AMB-07", "KA-01-EA-1007", AmbulanceVehicleType::CARDIAC_AMBULANCE,  "K. Hegde",      "Dr. N. Roy",     AmbulanceStatus::AVAILABLE, 90.0, "2026-10-05");
    add("AMB-08", "KA-01-EA-1008", AmbulanceVehicleType::ALS_AMBULANCE,      "B. Reddy",      "T. Sen",         AmbulanceStatus::AVAILABLE, 84.0, "2026-10-11");
    add("AMB-09", "KA-01-EA-1009", AmbulanceVehicleType::ICU_AMBULANCE,      "T. George",     "Dr. H. Iyer",    AmbulanceStatus::ASSIGNED,  74.0, "2026-10-02");
    add("AMB-10", "KA-01-EA-1010", AmbulanceVehicleType::BLS_AMBULANCE,      "C. Joseph",     "A. Khan",        AmbulanceStatus::ASSIGNED,  80.0, "2026-10-06");
    add("AMB-11", "KA-01-EA-1011", AmbulanceVehicleType::ALS_AMBULANCE,      "P. Swamy",      "R. Paul",        AmbulanceStatus::ASSIGNED,  72.0, "2026-10-04");
    add("AMB-12", "KA-01-EA-1012", AmbulanceVehicleType::NEONATAL_AMBULANCE, "G. Pillai",     "M. Pillai",      AmbulanceStatus::MAINTENANCE, 45.0, "2026-08-15");
}

bool EmsFleetManager::RegisterAmbulance(const AmbulanceRecord& ambulance) {
    std::lock_guard<std::mutex> lock(mutex_);
    fleet_[ambulance.ambulanceId] = ambulance;
    return true;
}

bool EmsFleetManager::UpdateStatus(const std::string& ambulanceId, AmbulanceStatus newStatus) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = fleet_.find(ambulanceId);
    if (it == fleet_.end()) {
        return false;
    }
    it->second.currentStatus = newStatus;
    if (newStatus == AmbulanceStatus::MAINTENANCE) {
        it->second.healthStatus = "SERVICING";
    } else if (newStatus == AmbulanceStatus::OUT_OF_SERVICE) {
        it->second.healthStatus = "FAULTY";
    } else {
        it->second.healthStatus = "OPTIMAL";
    }
    return true;
}

bool EmsFleetManager::AssignToMission(const std::string& ambulanceId,
                                     const std::string& incidentId,
                                     const std::string& hospitalId,
                                     double etaSec) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = fleet_.find(ambulanceId);
    if (it == fleet_.end()) {
        return false;
    }
    it->second.currentStatus = AmbulanceStatus::ASSIGNED;
    it->second.assignedIncident = incidentId;
    it->second.assignedHospital = hospitalId;
    it->second.etaSeconds = etaSec;
    return true;
}

bool EmsFleetManager::CompleteMission(const std::string& ambulanceId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = fleet_.find(ambulanceId);
    if (it == fleet_.end()) {
        return false;
    }
    it->second.currentStatus = AmbulanceStatus::AVAILABLE;
    it->second.assignedIncident.clear();
    it->second.assignedHospital.clear();
    it->second.etaSeconds = 0.0;
    it->second.totalMissionsCompleted++;
    return true;
}

bool EmsFleetManager::UpdateTelemetry(const std::string& ambulanceId,
                                     double lat,
                                     double lon,
                                     double etaSec,
                                     double fuelPercent) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = fleet_.find(ambulanceId);
    if (it == fleet_.end()) {
        return false;
    }
    it->second.latitude = lat;
    it->second.longitude = lon;
    it->second.etaSeconds = etaSec;
    if (fuelPercent >= 0.0) {
        it->second.fuelLevelPercent = fuelPercent;
    }
    return true;
}

std::optional<AmbulanceRecord> EmsFleetManager::GetAmbulance(const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = fleet_.find(ambulanceId);
    if (it == fleet_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<AmbulanceRecord> EmsFleetManager::GetAllAmbulances() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AmbulanceRecord> result;
    result.reserve(fleet_.size());
    for (const auto& [_, amb] : fleet_) {
        result.push_back(amb);
    }
    return result;
}

std::vector<AmbulanceRecord> EmsFleetManager::GetAmbulancesByStatus(AmbulanceStatus status) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AmbulanceRecord> result;
    for (const auto& [_, amb] : fleet_) {
        if (amb.currentStatus == status) {
            result.push_back(amb);
        }
    }
    return result;
}

FleetInventorySummary EmsFleetManager::GetInventorySummary() const {
    std::lock_guard<std::mutex> lock(mutex_);
    FleetInventorySummary summary;
    summary.totalAmbulances = fleet_.size();

    for (const auto& [_, amb] : fleet_) {
        switch (amb.currentStatus) {
            case AmbulanceStatus::AVAILABLE:
                summary.availableCount++;
                break;
            case AmbulanceStatus::ASSIGNED:
            case AmbulanceStatus::EN_ROUTE_PATIENT:
            case AmbulanceStatus::PATIENT_ON_BOARD:
                summary.assignedCount++;
                break;
            case AmbulanceStatus::EN_ROUTE_HOSPITAL:
                summary.enRouteHospitalCount++;
                break;
            case AmbulanceStatus::RETURNING:
                summary.returningCount++;
                break;
            case AmbulanceStatus::MAINTENANCE:
                summary.maintenanceCount++;
                break;
            case AmbulanceStatus::OUT_OF_SERVICE:
                summary.outOfServiceCount++;
                break;
        }
    }
    return summary;
}

void EmsFleetManager::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    fleet_.clear();
}

} // namespace aegis::command_center
