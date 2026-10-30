/**
 * @file FleetManagementEngine.cpp
 * @brief Implementation of Intelligent Ambulance Fleet Management & Dispatch Engine.
 */

#include "fleet_management/FleetManagementEngine.h"
#include "aegis/common/geo_utils.hpp"
#include "aegis/common/time_utils.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace aegis {
namespace fleet {

namespace {
constexpr double EARTH_RADIUS_KM = 6371.0;
constexpr double PI_VAL = 3.14159265358979323846;

inline double DegToRad(double deg) noexcept {
    return deg * (PI_VAL / 180.0);
}
} // namespace

FleetManagementEngine::FleetManagementEngine() {
    // Initialized with default settings
}

uint64_t FleetManagementEngine::GetCurrentTimeMsInternal() const {
    if (m_simulatedTimeMs > 0) {
        return m_simulatedTimeMs;
    }
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

double FleetManagementEngine::CalculateDistanceKmInternal(double lat1, double lon1, double lat2, double lon2) {
    double dLat = DegToRad(lat2 - lat1);
    double dLon = DegToRad(lon2 - lon1);
    double rLat1 = DegToRad(lat1);
    double rLat2 = DegToRad(lat2);

    double sinDLat2 = std::sin(dLat / 2.0);
    double sinDLon2 = std::sin(dLon / 2.0);

    double a = (sinDLat2 * sinDLat2) + std::cos(rLat1) * std::cos(rLat2) * (sinDLon2 * sinDLon2);
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(std::max(0.0, 1.0 - a)));

    return EARTH_RADIUS_KM * c;
}

double FleetManagementEngine::CalculateEtaMinutesInternal(double distanceKm, double speedKmH) {
    if (speedKmH <= 1.0) {
        speedKmH = 50.0; // Default urban response speed
    }
    double hours = distanceKm / speedKmH;
    return hours * 60.0;
}

void FleetManagementEngine::CheckHeartbeatTimeoutsInternal(uint64_t nowMs) {
    if (nowMs == 0) {
        nowMs = GetCurrentTimeMsInternal();
    }
    uint64_t timeoutMs = static_cast<uint64_t>(m_heartbeatTimeoutSeconds) * 1000ULL;

    for (auto& pair : m_ambulances) {
        auto& vehicle = pair.second;
        if (vehicle.status != AmbulanceStatus::OFFLINE && vehicle.lastHeartbeat > 0) {
            if (nowMs > vehicle.lastHeartbeat && (nowMs - vehicle.lastHeartbeat) > timeoutMs) {
                vehicle.status = AmbulanceStatus::OFFLINE;
            }
        }
    }
}

bool FleetManagementEngine::RegisterAmbulance(const AmbulanceRecord& ambulance) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (ambulance.ambulanceId.empty()) {
        return false;
    }

    if (m_ambulances.find(ambulance.ambulanceId) != m_ambulances.end()) {
        return false; // Already registered
    }

    AmbulanceRecord record = ambulance;
    uint64_t nowMs = GetCurrentTimeMsInternal();
    if (record.lastHeartbeat == 0) {
        record.lastHeartbeat = nowMs;
    }
    if (record.availabilityTimestamp == 0 && record.status == AmbulanceStatus::AVAILABLE) {
        record.availabilityTimestamp = nowMs;
    }

    m_ambulances[record.ambulanceId] = record;
    return true;
}

bool FleetManagementEngine::RemoveAmbulance(const std::string& ambulanceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_ambulances.find(ambulanceId);
    if (it == m_ambulances.end()) {
        return false;
    }

    m_ambulances.erase(it);
    return true;
}

bool FleetManagementEngine::GetAmbulance(const std::string& ambulanceId, AmbulanceRecord& outAmbulance) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_ambulances.find(ambulanceId);
    if (it == m_ambulances.end()) {
        return false;
    }

    outAmbulance = it->second;
    return true;
}

bool FleetManagementEngine::UpdateLocation(const std::string& ambulanceId, double latitude, double longitude, double speedKmH) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_ambulances.find(ambulanceId);
    if (it == m_ambulances.end()) {
        return false;
    }

    it->second.currentLatitude = latitude;
    it->second.currentLongitude = longitude;
    if (speedKmH >= 0.0) {
        it->second.currentSpeed = speedKmH;
    }
    return true;
}

bool FleetManagementEngine::UpdateStatus(const std::string& ambulanceId, AmbulanceStatus newStatus) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_ambulances.find(ambulanceId);
    if (it == m_ambulances.end()) {
        return false;
    }

    if (newStatus == AmbulanceStatus::AVAILABLE && it->second.status != AmbulanceStatus::AVAILABLE) {
        it->second.availabilityTimestamp = GetCurrentTimeMsInternal();
    }

    it->second.status = newStatus;
    return true;
}

bool FleetManagementEngine::UpdateHeartbeat(const std::string& ambulanceId, uint64_t timestampMs) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_ambulances.find(ambulanceId);
    if (it == m_ambulances.end()) {
        return false;
    }

    if (timestampMs == 0) {
        timestampMs = GetCurrentTimeMsInternal();
    }

    it->second.lastHeartbeat = timestampMs;

    // Automatic recovery upon heartbeat resumption
    if (it->second.status == AmbulanceStatus::OFFLINE) {
        if (it->second.maintenanceDue) {
            it->second.status = AmbulanceStatus::MAINTENANCE;
        } else if (!it->second.assignedIncident.empty()) {
            it->second.status = AmbulanceStatus::DISPATCHED;
        } else {
            it->second.status = AmbulanceStatus::AVAILABLE;
            it->second.availabilityTimestamp = timestampMs;
        }
    }

    return true;
}

bool FleetManagementEngine::SetMaintenanceDue(const std::string& ambulanceId, bool due) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_ambulances.find(ambulanceId);
    if (it == m_ambulances.end()) {
        return false;
    }

    it->second.maintenanceDue = due;
    if (due && it->second.status == AmbulanceStatus::AVAILABLE) {
        it->second.status = AmbulanceStatus::MAINTENANCE;
    }
    return true;
}

bool FleetManagementEngine::UpdateEnergy(const std::string& ambulanceId, double fuel, double battery, double remainingRangeKm) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_ambulances.find(ambulanceId);
    if (it == m_ambulances.end()) {
        return false;
    }

    it->second.fuelLevel = std::max(0.0, std::min(100.0, fuel));
    it->second.batteryLevel = std::max(0.0, std::min(100.0, battery));
    it->second.remainingRange = std::max(0.0, remainingRangeKm);
    return true;
}

bool FleetManagementEngine::AssignIncident(const std::string& ambulanceId, const std::string& incidentId, const std::string& hospitalId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_ambulances.find(ambulanceId);
    if (it == m_ambulances.end()) {
        return false;
    }

    // Must be available, not in maintenance, and not offline
    if (it->second.status != AmbulanceStatus::AVAILABLE || it->second.maintenanceDue) {
        return false;
    }

    it->second.status = AmbulanceStatus::DISPATCHED;
    it->second.assignedIncident = incidentId;
    it->second.assignedHospital = hospitalId;
    return true;
}

bool FleetManagementEngine::ReleaseAmbulance(const std::string& ambulanceId, const std::string& hospitalId, double addedDistanceKm) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_ambulances.find(ambulanceId);
    if (it == m_ambulances.end()) {
        return false;
    }

    it->second.assignedIncident = "";
    if (!hospitalId.empty()) {
        it->second.assignedHospital = hospitalId;
    }
    if (addedDistanceKm > 0.0) {
        it->second.totalDistanceToday += addedDistanceKm;
        it->second.remainingRange = std::max(0.0, it->second.remainingRange - addedDistanceKm);
    }
    it->second.totalPatientsHandled += 1;

    if (it->second.maintenanceDue) {
        it->second.status = AmbulanceStatus::MAINTENANCE;
    } else {
        it->second.status = AmbulanceStatus::AVAILABLE;
        it->second.availabilityTimestamp = GetCurrentTimeMsInternal();
    }

    return true;
}

double FleetManagementEngine::ScoreAmbulanceInternal(const AmbulanceRecord& record,
                                                     double incidentLat,
                                                     double incidentLon,
                                                     AmbulanceCapability reqCap,
                                                     double& outDistanceKm,
                                                     double& outEtaMin,
                                                     bool& outEligible,
                                                     std::string& outReason) const {
    outDistanceKm = CalculateDistanceKmInternal(record.currentLatitude, record.currentLongitude, incidentLat, incidentLon);
    outEtaMin = CalculateEtaMinutesInternal(outDistanceKm, record.currentSpeed > 20.0 ? record.currentSpeed : m_defaultEmergencySpeedKmH);

    // Rule 6: Availability
    if (record.status != AmbulanceStatus::AVAILABLE) {
        outEligible = false;
        outReason = "Vehicle is not available (" + AmbulanceStatusToString(record.status) + ")";
        return 1e9;
    }

    // Rule 7: Maintenance status
    if (record.maintenanceDue || record.status == AmbulanceStatus::MAINTENANCE) {
        outEligible = false;
        outReason = "Vehicle requires maintenance";
        return 1e9;
    }

    // Rule 8: Heartbeat freshness
    if (record.status == AmbulanceStatus::OFFLINE) {
        outEligible = false;
        outReason = "Vehicle is offline / heartbeat expired";
        return 1e9;
    }

    // Rule 3: Medical capability
    if (!CanSatisfyCapability(record.capability, reqCap)) {
        outEligible = false;
        outReason = "Capability mismatch (" + CapabilityToShortString(record.capability) +
                    " cannot satisfy " + CapabilityToShortString(reqCap) + ")";
        return 1e9;
    }

    // Rule 4: Fuel / range check
    if (record.fuelLevel < 10.0 || (record.remainingRange > 0.0 && record.remainingRange < (outDistanceKm * 1.5))) {
        outEligible = false;
        outReason = "Insufficient fuel or remaining range";
        return 1e9;
    }

    outEligible = true;

    // Multi-factor composite cost calculation (lower is better)
    // 1. ETA primary weight
    double score = outEtaMin * 1.0;

    // 2. Distance secondary weight
    score += (outDistanceKm * 0.1);

    // 3. Medical capability match bonus/penalty:
    // If exact capability match -> 0 penalty
    // If higher capability responding to lower request (e.g. ALS to BLS) -> small reservation penalty
    if (record.capability != reqCap) {
        score += 1.5; // Small penalty to preserve high-tier units if BLS is close
    }

    // 4. Fuel penalty: mildly favor full tank
    if (record.fuelLevel < 30.0) {
        score += (30.0 - record.fuelLevel) * 0.1;
    }

    // 5. Workload balancing: favor crew with fewer calls
    score += static_cast<double>(record.totalPatientsHandled) * 0.2;
    score += record.totalDistanceToday * 0.005;

    outReason = "Eligible candidate";
    return score;
}

DispatchAssignment FleetManagementEngine::FindNearestAvailable(double latitude, double longitude, AmbulanceCapability capability) {
    std::lock_guard<std::mutex> lock(m_mutex);

    CheckHeartbeatTimeoutsInternal(GetCurrentTimeMsInternal());

    DispatchAssignment assignment{};
    assignment.incidentId = "INC_NEAREST";
    assignment.assigned = false;
    assignment.distanceKm = 1e9;

    double minDistance = 1e9;
    std::string bestId = "";
    AmbulanceRecord bestVehicle{};

    for (const auto& pair : m_ambulances) {
        const auto& vehicle = pair.second;
        if (vehicle.status != AmbulanceStatus::AVAILABLE || vehicle.maintenanceDue) {
            continue;
        }

        if (!CanSatisfyCapability(vehicle.capability, capability)) {
            continue;
        }

        double dist = CalculateDistanceKmInternal(vehicle.currentLatitude, vehicle.currentLongitude, latitude, longitude);
        if (dist < minDistance || (std::abs(dist - minDistance) < 1e-6 && vehicle.ambulanceId < bestId)) {
            minDistance = dist;
            bestId = vehicle.ambulanceId;
            bestVehicle = vehicle;
        }
    }

    if (!bestId.empty()) {
        assignment.assigned = true;
        assignment.assignedAmbulanceId = bestId;
        assignment.capability = bestVehicle.capability;
        assignment.distanceKm = minDistance;
        assignment.etaMinutes = CalculateEtaMinutesInternal(minDistance, m_defaultEmergencySpeedKmH);
        assignment.rankingScore = minDistance;
        assignment.reason = "Nearest Available " + CapabilityToShortString(bestVehicle.capability) + " Unit";
        assignment.vehicleSnapshot = bestVehicle;
    } else {
        assignment.reason = "No Available Unit Matching Criteria";
    }

    return assignment;
}

std::vector<RankedAmbulance> FleetManagementEngine::RankAmbulances(double incidentLat,
                                                                 double incidentLon,
                                                                 AmbulanceCapability requiredCapability) {
    std::vector<RankedAmbulance> rankedList;
    rankedList.reserve(m_ambulances.size());

    for (const auto& pair : m_ambulances) {
        const auto& vehicle = pair.second;
        RankedAmbulance ranked{};
        ranked.ambulanceId = vehicle.ambulanceId;
        ranked.capability = vehicle.capability;

        double distKm = 0.0;
        double etaMin = 0.0;
        bool eligible = false;
        std::string reason = "";

        double score = ScoreAmbulanceInternal(vehicle, incidentLat, incidentLon, requiredCapability,
                                              distKm, etaMin, eligible, reason);

        ranked.distanceKm = distKm;
        ranked.etaMinutes = etaMin;
        ranked.compositeScore = score;
        ranked.eligible = eligible;
        ranked.ineligibilityReason = reason;

        rankedList.push_back(ranked);
    }

    // Deterministic sort:
    // 1. Eligible first
    // 2. Lowest composite score first
    // 3. Lowest distance
    // 4. Lexicographical ambulance ID
    std::sort(rankedList.begin(), rankedList.end(), [](const RankedAmbulance& a, const RankedAmbulance& b) {
        if (a.eligible != b.eligible) {
            return a.eligible > b.eligible; // true before false
        }
        if (std::abs(a.compositeScore - b.compositeScore) > 1e-5) {
            return a.compositeScore < b.compositeScore;
        }
        if (std::abs(a.distanceKm - b.distanceKm) > 1e-5) {
            return a.distanceKm < b.distanceKm;
        }
        return a.ambulanceId < b.ambulanceId;
    });

    return rankedList;
}

DispatchAssignment FleetManagementEngine::FindBestAmbulance(const std::string& incidentId,
                                                           double incidentLat,
                                                           double incidentLon,
                                                           AmbulanceCapability requiredCapability,
                                                           const std::string& hospitalId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    CheckHeartbeatTimeoutsInternal(GetCurrentTimeMsInternal());

    DispatchAssignment assignment{};
    assignment.incidentId = incidentId;
    assignment.assigned = false;

    auto rankedList = RankAmbulances(incidentLat, incidentLon, requiredCapability);

    if (!rankedList.empty() && rankedList.front().eligible) {
        const auto& best = rankedList.front();
        assignment.assigned = true;
        assignment.assignedAmbulanceId = best.ambulanceId;
        assignment.capability = best.capability;
        assignment.distanceKm = best.distanceKm;
        assignment.etaMinutes = best.etaMinutes;
        assignment.rankingScore = best.compositeScore;
        assignment.vehicleSnapshot = m_ambulances[best.ambulanceId];

        // Format descriptive reason matching audit log
        assignment.reason = "Nearest Available " + CapabilityToShortString(best.capability) + " Unit";

        // Assign vehicle internally
        m_ambulances[best.ambulanceId].status = AmbulanceStatus::DISPATCHED;
        m_ambulances[best.ambulanceId].assignedIncident = incidentId;
        m_ambulances[best.ambulanceId].assignedHospital = hospitalId;

        // Structured Audit Log
        std::string logEntry = FormatAuditLog(incidentId, best.ambulanceId, best.capability, best.etaMinutes, assignment.reason);
        m_auditLogs.push_back(logEntry);
        std::cout << logEntry << std::endl;
    } else {
        assignment.reason = "No Available Unit Matching Criteria";
    }

    return assignment;
}

DispatchAssignment FleetManagementEngine::FindBestAmbulance(const DispatchRequest& request) {
    return FindBestAmbulance(request.incidentId,
                             request.incidentLatitude,
                             request.incidentLongitude,
                             request.requiredCapability,
                             request.targetHospital);
}

std::vector<AmbulanceRecord> FleetManagementEngine::GetFleetStatus() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<AmbulanceRecord> result;
    result.reserve(m_ambulances.size());
    for (const auto& pair : m_ambulances) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<AmbulanceRecord> FleetManagementEngine::GetAvailableAmbulances() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<AmbulanceRecord> result;
    for (const auto& pair : m_ambulances) {
        if (pair.second.status == AmbulanceStatus::AVAILABLE && !pair.second.maintenanceDue) {
            result.push_back(pair.second);
        }
    }
    return result;
}

std::vector<AmbulanceRecord> FleetManagementEngine::GetBusyAmbulances() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<AmbulanceRecord> result;
    for (const auto& pair : m_ambulances) {
        const auto& v = pair.second;
        if (v.status == AmbulanceStatus::DISPATCHED ||
            v.status == AmbulanceStatus::AT_PATIENT ||
            v.status == AmbulanceStatus::TRANSPORTING ||
            v.status == AmbulanceStatus::AT_HOSPITAL ||
            v.status == AmbulanceStatus::RETURNING) {
            result.push_back(v);
        }
    }
    return result;
}

std::vector<AmbulanceRecord> FleetManagementEngine::GetOfflineAmbulances() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<AmbulanceRecord> result;
    for (const auto& pair : m_ambulances) {
        if (pair.second.status == AmbulanceStatus::OFFLINE) {
            result.push_back(pair.second);
        }
    }
    return result;
}

std::vector<AmbulanceRecord> FleetManagementEngine::GetMaintenanceVehicles() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<AmbulanceRecord> result;
    for (const auto& pair : m_ambulances) {
        if (pair.second.status == AmbulanceStatus::MAINTENANCE || pair.second.maintenanceDue) {
            result.push_back(pair.second);
        }
    }
    return result;
}

FleetStatistics FleetManagementEngine::GetFleetStatistics() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    FleetStatistics stats{};
    stats.totalVehicles = static_cast<uint32_t>(m_ambulances.size());

    double totalFuel = 0.0;
    double totalBattery = 0.0;

    for (const auto& pair : m_ambulances) {
        const auto& v = pair.second;
        switch (v.status) {
            case AmbulanceStatus::AVAILABLE:
                if (v.maintenanceDue) {
                    stats.maintenanceCount++;
                } else {
                    stats.availableCount++;
                }
                break;
            case AmbulanceStatus::DISPATCHED:
                stats.dispatchedCount++;
                stats.busyCount++;
                break;
            case AmbulanceStatus::AT_PATIENT:
                stats.atPatientCount++;
                stats.busyCount++;
                break;
            case AmbulanceStatus::TRANSPORTING:
                stats.transportingCount++;
                stats.busyCount++;
                break;
            case AmbulanceStatus::AT_HOSPITAL:
                stats.atHospitalCount++;
                stats.busyCount++;
                break;
            case AmbulanceStatus::RETURNING:
                stats.returningCount++;
                stats.busyCount++;
                break;
            case AmbulanceStatus::MAINTENANCE:
                stats.maintenanceCount++;
                break;
            case AmbulanceStatus::OFFLINE:
                stats.offlineCount++;
                break;
        }

        totalFuel += v.fuelLevel;
        totalBattery += v.batteryLevel;
        stats.totalDistanceTodayKm += v.totalDistanceToday;
        stats.totalPatientsHandledToday += v.totalPatientsHandled;
    }

    if (stats.totalVehicles > 0) {
        stats.averageFuelLevel = totalFuel / static_cast<double>(stats.totalVehicles);
        stats.averageBatteryLevel = totalBattery / static_cast<double>(stats.totalVehicles);
    }

    return stats;
}

std::string FleetManagementEngine::GenerateFleetReport() const {
    FleetStatistics stats = GetFleetStatistics();

    std::ostringstream oss;
    oss << "==========================================================\n";
    oss << " SMART AEGIS: FLEET MANAGEMENT REPORT\n";
    oss << "==========================================================\n";
    oss << "Total Vehicles:          " << stats.totalVehicles << "\n";
    oss << "Available Units:         " << stats.availableCount << "\n";
    oss << "Busy (On Mission):       " << stats.busyCount << "\n";
    oss << "  - Dispatched:          " << stats.dispatchedCount << "\n";
    oss << "  - At Patient:          " << stats.atPatientCount << "\n";
    oss << "  - Transporting:        " << stats.transportingCount << "\n";
    oss << "  - At Hospital:         " << stats.atHospitalCount << "\n";
    oss << "  - Returning:           " << stats.returningCount << "\n";
    oss << "Maintenance Units:       " << stats.maintenanceCount << "\n";
    oss << "Offline Units:           " << stats.offlineCount << "\n";
    oss << "----------------------------------------------------------\n";
    oss << std::fixed << std::setprecision(1);
    oss << "Average Fuel Level:      " << stats.averageFuelLevel << "%\n";
    oss << "Average Battery Level:   " << stats.averageBatteryLevel << "%\n";
    oss << "Total Distance Driven:   " << stats.totalDistanceTodayKm << " km\n";
    oss << "Total Patients Handled:  " << stats.totalPatientsHandledToday << "\n";
    oss << "==========================================================\n";

    return oss.str();
}

uint32_t FleetManagementEngine::CheckHeartbeatTimeouts(uint64_t currentTimestampMs) {
    std::lock_guard<std::mutex> lock(m_mutex);

    uint32_t newlyOffline = 0;
    if (currentTimestampMs == 0) {
        currentTimestampMs = GetCurrentTimeMsInternal();
    }
    uint64_t timeoutMs = static_cast<uint64_t>(m_heartbeatTimeoutSeconds) * 1000ULL;

    for (auto& pair : m_ambulances) {
        auto& vehicle = pair.second;
        if (vehicle.status != AmbulanceStatus::OFFLINE && vehicle.lastHeartbeat > 0) {
            if (currentTimestampMs > vehicle.lastHeartbeat && (currentTimestampMs - vehicle.lastHeartbeat) > timeoutMs) {
                vehicle.status = AmbulanceStatus::OFFLINE;
                newlyOffline++;
            }
        }
    }

    return newlyOffline;
}

void FleetManagementEngine::SetHeartbeatTimeoutSeconds(uint32_t seconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_heartbeatTimeoutSeconds = seconds;
}

uint32_t FleetManagementEngine::GetHeartbeatTimeoutSeconds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_heartbeatTimeoutSeconds;
}

void FleetManagementEngine::SetSimulatedTimeMs(uint64_t ms) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_simulatedTimeMs = ms;
}

uint64_t FleetManagementEngine::GetSimulatedTimeMs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_simulatedTimeMs;
}

std::string FleetManagementEngine::FormatAuditLog(const std::string& incidentId,
                                                 const std::string& ambulanceId,
                                                 AmbulanceCapability cap,
                                                 double etaMin,
                                                 const std::string& reason) {
    std::ostringstream oss;
    oss << "[FLEET]\n\n";
    oss << "Incident:\n" << incidentId << "\n\n";
    oss << "Assigned Ambulance:\n" << ambulanceId << "\n\n";
    oss << "Capability:\n" << CapabilityToShortString(cap) << "\n\n";
    oss << "ETA:\n" << std::fixed << std::setprecision(1) << etaMin << " min\n\n";
    oss << "Reason:\n" << reason << "\n";
    return oss.str();
}

std::vector<std::string> FleetManagementEngine::GetAuditLogs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_auditLogs;
}

void FleetManagementEngine::ClearHistory() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_auditLogs.clear();
}

void FleetManagementEngine::ClearAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_ambulances.clear();
    m_auditLogs.clear();
}

} // namespace fleet
} // namespace aegis
