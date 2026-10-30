/**
 * @file HospitalResourceEngine.cpp
 * @brief SMART AEGIS (AERS) Phase 11B - Hospital Resource Allocation & Bed Management Engine Implementation
 *
 * Implements centralized hospital resource monitoring, bed and ICU reservations,
 * and deterministic multi-criteria destination hospital selection for ambulance fleets.
 *
 * Pure C++17, QNX SDP 8.0 & Raspberry Pi 4 compliant.
 * Zero third-party dependencies, thread-safe, deterministic.
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#include "HospitalResourceEngine.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace aegis {
namespace hospital_management {

// =============================================================================
// Constants & Helpers
// =============================================================================

static constexpr double kEarthRadiusKm = 6371.0;
static constexpr double kDegToRad = 3.14159265358979323846 / 180.0;
static constexpr double kScoreEpsilon = 1e-5;

const char* HospitalStatusToString(HospitalStatus status) {
    switch (status) {
        case HospitalStatus::ONLINE:         return "ONLINE";
        case HospitalStatus::OFFLINE:        return "OFFLINE";
        case HospitalStatus::FULL:           return "FULL";
        case HospitalStatus::EMERGENCY_ONLY: return "EMERGENCY_ONLY";
        case HospitalStatus::MAINTENANCE:    return "MAINTENANCE";
        default:                             return "UNKNOWN";
    }
}

bool StringToHospitalStatus(const std::string& str, HospitalStatus& outStatus) {
    if (str == "ONLINE") {
        outStatus = HospitalStatus::ONLINE;
        return true;
    } else if (str == "OFFLINE") {
        outStatus = HospitalStatus::OFFLINE;
        return true;
    } else if (str == "FULL") {
        outStatus = HospitalStatus::FULL;
        return true;
    } else if (str == "EMERGENCY_ONLY") {
        outStatus = HospitalStatus::EMERGENCY_ONLY;
        return true;
    } else if (str == "MAINTENANCE") {
        outStatus = HospitalStatus::MAINTENANCE;
        return true;
    }
    return false;
}

const char* HospitalCapabilityToString(HospitalCapability capability) {
    switch (capability) {
        case HospitalCapability::GENERAL:        return "GENERAL";
        case HospitalCapability::TRAUMA:         return "TRAUMA";
        case HospitalCapability::CARDIAC:        return "CARDIAC";
        case HospitalCapability::PEDIATRIC:      return "PEDIATRIC";
        case HospitalCapability::NEURO:          return "NEURO";
        case HospitalCapability::BURNS:          return "BURNS";
        case HospitalCapability::ICU:            return "ICU";
        case HospitalCapability::NEONATAL:       return "NEONATAL";
        case HospitalCapability::ORTHOPEDIC:      return "ORTHOPEDIC";
        case HospitalCapability::MULTISPECIALITY: return "MULTISPECIALITY";
        default:                                 return "UNKNOWN";
    }
}

bool StringToHospitalCapability(const std::string& str, HospitalCapability& outCap) {
    if (str == "GENERAL") {
        outCap = HospitalCapability::GENERAL;
        return true;
    } else if (str == "TRAUMA") {
        outCap = HospitalCapability::TRAUMA;
        return true;
    } else if (str == "CARDIAC") {
        outCap = HospitalCapability::CARDIAC;
        return true;
    } else if (str == "PEDIATRIC") {
        outCap = HospitalCapability::PEDIATRIC;
        return true;
    } else if (str == "NEURO") {
        outCap = HospitalCapability::NEURO;
        return true;
    } else if (str == "BURNS") {
        outCap = HospitalCapability::BURNS;
        return true;
    } else if (str == "ICU") {
        outCap = HospitalCapability::ICU;
        return true;
    } else if (str == "NEONATAL") {
        outCap = HospitalCapability::NEONATAL;
        return true;
    } else if (str == "ORTHOPEDIC") {
        outCap = HospitalCapability::ORTHOPEDIC;
        return true;
    } else if (str == "MULTISPECIALITY") {
        outCap = HospitalCapability::MULTISPECIALITY;
        return true;
    }
    return false;
}

// =============================================================================
// Constructor & Destructor
// =============================================================================

HospitalResourceEngine::HospitalResourceEngine() {
    Reset();
}

uint64_t HospitalResourceEngine::GetCurrentTimeMs() const {
    auto now = std::chrono::steady_clock::now();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count());
}

double HospitalResourceEngine::CalculateDistanceKm(double lat1, double lon1, double lat2, double lon2) {
    double dLat = (lat2 - lat1) * kDegToRad;
    double dLon = (lon2 - lon1) * kDegToRad;
    double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
               std::cos(lat1 * kDegToRad) * std::cos(lat2 * kDegToRad) *
               std::sin(dLon / 2.0) * std::sin(dLon / 2.0);
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return kEarthRadiusKm * c;
}

// =============================================================================
// Hospital Management & Registry APIs
// =============================================================================

bool HospitalResourceEngine::RegisterHospital(const HospitalRecord& hospital) {
    if (hospital.hospitalId.empty() || hospital.hospitalName.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> regLock(registryMutex_);
    if (hospitals_.find(hospital.hospitalId) != hospitals_.end()) {
        return false; // Duplicate registration rejected
    }

    HospitalRecord record = hospital;
    if (record.lastUpdatedTimestamp == 0) {
        record.lastUpdatedTimestamp = GetCurrentTimeMs();
    }
    // Ensure occupancy metrics are synchronized
    if (record.bedAvailability.totalBeds > 0) {
        record.bedAvailability.occupancyRate = 
            (static_cast<double>(record.bedAvailability.totalBeds - record.bedAvailability.availableBeds) /
             static_cast<double>(record.bedAvailability.totalBeds)) * 100.0;
    }
    if (record.icuBeds.totalIcuBeds > 0) {
        record.icuBeds.icuOccupancyRate = 
            (static_cast<double>(record.icuBeds.totalIcuBeds - record.icuBeds.availableIcuBeds) /
             static_cast<double>(record.icuBeds.totalIcuBeds)) * 100.0;
    }
    if (record.emergencyDept.capacity > 0) {
        record.emergencyDept.occupancyPercentage = 
            (static_cast<double>(record.emergencyDept.currentLoad) /
             static_cast<double>(record.emergencyDept.capacity)) * 100.0;
    }

    hospitals_[record.hospitalId] = record;

    // Update statistics
    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        statistics_.totalHospitalsRegistered = static_cast<uint32_t>(hospitals_.size());
        if (record.status == HospitalStatus::ONLINE) statistics_.onlineHospitals++;
        else if (record.status == HospitalStatus::OFFLINE) statistics_.offlineHospitals++;
        else if (record.status == HospitalStatus::FULL) statistics_.fullHospitals++;
        else if (record.status == HospitalStatus::EMERGENCY_ONLY) statistics_.emergencyOnlyHospitals++;
        else if (record.status == HospitalStatus::MAINTENANCE) statistics_.maintenanceHospitals++;

        statistics_.totalBeds += record.bedAvailability.totalBeds;
        statistics_.availableBeds += record.bedAvailability.availableBeds;
        statistics_.totalIcuBeds += record.icuBeds.totalIcuBeds;
        statistics_.availableIcuBeds += record.icuBeds.availableIcuBeds;
        statistics_.totalVentilators += record.ventilatorStatus.totalVentilators;
        statistics_.availableVentilators += record.ventilatorStatus.availableVentilators;
        statistics_.totalDoctorsAvailable += record.doctorAvailability.availableDoctors;
    }

    return true;
}

bool HospitalResourceEngine::RemoveHospital(const std::string& hospitalId) {
    if (hospitalId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = hospitals_.find(hospitalId);
    if (it == hospitals_.end()) {
        return false;
    }

    HospitalRecord removed = it->second;
    hospitals_.erase(it);

    // Update statistics
    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        statistics_.totalHospitalsRegistered = static_cast<uint32_t>(hospitals_.size());
        if (removed.status == HospitalStatus::ONLINE && statistics_.onlineHospitals > 0) statistics_.onlineHospitals--;
        else if (removed.status == HospitalStatus::OFFLINE && statistics_.offlineHospitals > 0) statistics_.offlineHospitals--;
        else if (removed.status == HospitalStatus::FULL && statistics_.fullHospitals > 0) statistics_.fullHospitals--;
        else if (removed.status == HospitalStatus::EMERGENCY_ONLY && statistics_.emergencyOnlyHospitals > 0) statistics_.emergencyOnlyHospitals--;
        else if (removed.status == HospitalStatus::MAINTENANCE && statistics_.maintenanceHospitals > 0) statistics_.maintenanceHospitals--;

        if (statistics_.totalBeds >= removed.bedAvailability.totalBeds) statistics_.totalBeds -= removed.bedAvailability.totalBeds;
        if (statistics_.availableBeds >= removed.bedAvailability.availableBeds) statistics_.availableBeds -= removed.bedAvailability.availableBeds;
        if (statistics_.totalIcuBeds >= removed.icuBeds.totalIcuBeds) statistics_.totalIcuBeds -= removed.icuBeds.totalIcuBeds;
        if (statistics_.availableIcuBeds >= removed.icuBeds.availableIcuBeds) statistics_.availableIcuBeds -= removed.icuBeds.availableIcuBeds;
        if (statistics_.totalVentilators >= removed.ventilatorStatus.totalVentilators) statistics_.totalVentilators -= removed.ventilatorStatus.totalVentilators;
        if (statistics_.availableVentilators >= removed.ventilatorStatus.availableVentilators) statistics_.availableVentilators -= removed.ventilatorStatus.availableVentilators;
        if (statistics_.totalDoctorsAvailable >= removed.doctorAvailability.availableDoctors) statistics_.totalDoctorsAvailable -= removed.doctorAvailability.availableDoctors;
    }

    return true;
}

bool HospitalResourceEngine::UpdateHospitalLocation(const std::string& hospitalId, double latitude, double longitude) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = hospitals_.find(hospitalId);
    if (it == hospitals_.end()) {
        return false;
    }
    it->second.latitude = latitude;
    it->second.longitude = longitude;
    it->second.lastUpdatedTimestamp = GetCurrentTimeMs();
    return true;
}

bool HospitalResourceEngine::UpdateBedAvailability(const std::string& hospitalId, const BedAvailability& beds) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = hospitals_.find(hospitalId);
    if (it == hospitals_.end()) {
        return false;
    }

    BedAvailability oldBeds = it->second.bedAvailability;
    it->second.bedAvailability = beds;
    if (it->second.bedAvailability.totalBeds > 0) {
        it->second.bedAvailability.occupancyRate = 
            (static_cast<double>(it->second.bedAvailability.totalBeds - it->second.bedAvailability.availableBeds) /
             static_cast<double>(it->second.bedAvailability.totalBeds)) * 100.0;
    }
    it->second.lastUpdatedTimestamp = GetCurrentTimeMs();

    // Check if hospital state transitions to FULL or recovers from FULL
    if (it->second.bedAvailability.availableBeds == 0 && it->second.icuBeds.availableIcuBeds == 0 &&
        it->second.status == HospitalStatus::ONLINE) {
        it->second.status = HospitalStatus::FULL;
    } else if (it->second.status == HospitalStatus::FULL && it->second.bedAvailability.availableBeds > 0) {
        it->second.status = HospitalStatus::ONLINE;
    }

    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        if (statistics_.totalBeds >= oldBeds.totalBeds) statistics_.totalBeds -= oldBeds.totalBeds;
        statistics_.totalBeds += beds.totalBeds;
        if (statistics_.availableBeds >= oldBeds.availableBeds) statistics_.availableBeds -= oldBeds.availableBeds;
        statistics_.availableBeds += beds.availableBeds;
    }

    return true;
}

bool HospitalResourceEngine::UpdateBedAvailability(const std::string& hospitalId, uint32_t totalBeds, uint32_t availableBeds) {
    BedAvailability beds(totalBeds, availableBeds);
    return UpdateBedAvailability(hospitalId, beds);
}

bool HospitalResourceEngine::UpdateICUBeds(const std::string& hospitalId, const ICUBed& icuBeds) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = hospitals_.find(hospitalId);
    if (it == hospitals_.end()) {
        return false;
    }

    ICUBed oldIcu = it->second.icuBeds;
    it->second.icuBeds = icuBeds;
    if (it->second.icuBeds.totalIcuBeds > 0) {
        it->second.icuBeds.icuOccupancyRate = 
            (static_cast<double>(it->second.icuBeds.totalIcuBeds - it->second.icuBeds.availableIcuBeds) /
             static_cast<double>(it->second.icuBeds.totalIcuBeds)) * 100.0;
    }
    it->second.lastUpdatedTimestamp = GetCurrentTimeMs();

    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        if (statistics_.totalIcuBeds >= oldIcu.totalIcuBeds) statistics_.totalIcuBeds -= oldIcu.totalIcuBeds;
        statistics_.totalIcuBeds += icuBeds.totalIcuBeds;
        if (statistics_.availableIcuBeds >= oldIcu.availableIcuBeds) statistics_.availableIcuBeds -= oldIcu.availableIcuBeds;
        statistics_.availableIcuBeds += icuBeds.availableIcuBeds;
    }

    return true;
}

bool HospitalResourceEngine::UpdateICUBeds(const std::string& hospitalId, uint32_t totalIcuBeds, uint32_t availableIcuBeds) {
    ICUBed icu(totalIcuBeds, availableIcuBeds);
    return UpdateICUBeds(hospitalId, icu);
}

bool HospitalResourceEngine::UpdateEmergencyCapacity(const std::string& hospitalId, const EmergencyDepartment& emergencyDept) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = hospitals_.find(hospitalId);
    if (it == hospitals_.end()) {
        return false;
    }

    it->second.emergencyDept = emergencyDept;
    if (it->second.emergencyDept.capacity > 0) {
        it->second.emergencyDept.occupancyPercentage = 
            (static_cast<double>(it->second.emergencyDept.currentLoad) /
             static_cast<double>(it->second.emergencyDept.capacity)) * 100.0;
    }
    it->second.lastUpdatedTimestamp = GetCurrentTimeMs();
    return true;
}

bool HospitalResourceEngine::UpdateEmergencyCapacity(const std::string& hospitalId, uint32_t capacity, uint32_t currentLoad, uint32_t queueLength) {
    EmergencyDepartment ed(capacity, currentLoad, queueLength);
    return UpdateEmergencyCapacity(hospitalId, ed);
}

bool HospitalResourceEngine::UpdateDoctorAvailability(const std::string& hospitalId, const DoctorAvailability& doctors) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = hospitals_.find(hospitalId);
    if (it == hospitals_.end()) {
        return false;
    }

    uint32_t oldAvail = it->second.doctorAvailability.availableDoctors;
    it->second.doctorAvailability = doctors;
    it->second.lastUpdatedTimestamp = GetCurrentTimeMs();

    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        if (statistics_.totalDoctorsAvailable >= oldAvail) statistics_.totalDoctorsAvailable -= oldAvail;
        statistics_.totalDoctorsAvailable += doctors.availableDoctors;
    }

    return true;
}

bool HospitalResourceEngine::UpdateDoctorAvailability(const std::string& hospitalId, uint32_t totalDoctors, uint32_t availableDoctors) {
    DoctorAvailability doc(totalDoctors, availableDoctors);
    return UpdateDoctorAvailability(hospitalId, doc);
}

bool HospitalResourceEngine::UpdateVentilatorStatus(const std::string& hospitalId, const VentilatorStatus& ventStatus) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = hospitals_.find(hospitalId);
    if (it == hospitals_.end()) {
        return false;
    }

    VentilatorStatus oldVent = it->second.ventilatorStatus;
    it->second.ventilatorStatus = ventStatus;
    it->second.lastUpdatedTimestamp = GetCurrentTimeMs();

    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        if (statistics_.totalVentilators >= oldVent.totalVentilators) statistics_.totalVentilators -= oldVent.totalVentilators;
        statistics_.totalVentilators += ventStatus.totalVentilators;
        if (statistics_.availableVentilators >= oldVent.availableVentilators) statistics_.availableVentilators -= oldVent.availableVentilators;
        statistics_.availableVentilators += ventStatus.availableVentilators;
    }

    return true;
}

bool HospitalResourceEngine::UpdateVentilatorStatus(const std::string& hospitalId, uint32_t totalVentilators, uint32_t availableVentilators) {
    VentilatorStatus vent(totalVentilators, availableVentilators);
    return UpdateVentilatorStatus(hospitalId, vent);
}

bool HospitalResourceEngine::UpdateSpecialties(const std::string& hospitalId, const std::vector<HospitalCapability>& specialties) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = hospitals_.find(hospitalId);
    if (it == hospitals_.end()) {
        return false;
    }
    it->second.capabilities = specialties;
    it->second.lastUpdatedTimestamp = GetCurrentTimeMs();
    return true;
}

bool HospitalResourceEngine::UpdateStatus(const std::string& hospitalId, HospitalStatus status) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = hospitals_.find(hospitalId);
    if (it == hospitals_.end()) {
        return false;
    }

    HospitalStatus oldStatus = it->second.status;
    it->second.status = status;
    it->second.lastUpdatedTimestamp = GetCurrentTimeMs();

    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        if (oldStatus == HospitalStatus::ONLINE && statistics_.onlineHospitals > 0) statistics_.onlineHospitals--;
        else if (oldStatus == HospitalStatus::OFFLINE && statistics_.offlineHospitals > 0) statistics_.offlineHospitals--;
        else if (oldStatus == HospitalStatus::FULL && statistics_.fullHospitals > 0) statistics_.fullHospitals--;
        else if (oldStatus == HospitalStatus::EMERGENCY_ONLY && statistics_.emergencyOnlyHospitals > 0) statistics_.emergencyOnlyHospitals--;
        else if (oldStatus == HospitalStatus::MAINTENANCE && statistics_.maintenanceHospitals > 0) statistics_.maintenanceHospitals--;

        if (status == HospitalStatus::ONLINE) statistics_.onlineHospitals++;
        else if (status == HospitalStatus::OFFLINE) statistics_.offlineHospitals++;
        else if (status == HospitalStatus::FULL) statistics_.fullHospitals++;
        else if (status == HospitalStatus::EMERGENCY_ONLY) statistics_.emergencyOnlyHospitals++;
        else if (status == HospitalStatus::MAINTENANCE) statistics_.maintenanceHospitals++;
    }

    return true;
}

// =============================================================================
// Query & Optimization APIs
// =============================================================================

bool HospitalResourceEngine::FindNearestHospital(double latitude, double longitude,
                                                HospitalRecord& outHospital,
                                                HospitalCapability requiredCapability) const {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    if (hospitals_.empty()) {
        return false;
    }

    double minDistance = 1e9;
    bool found = false;
    HospitalRecord nearest;

    for (const auto& pair : hospitals_) {
        const auto& record = pair.second;
        if (record.status == HospitalStatus::OFFLINE || record.status == HospitalStatus::MAINTENANCE) {
            continue;
        }

        if (requiredCapability != HospitalCapability::GENERAL && !record.HasCapability(requiredCapability)) {
            continue;
        }

        double d = CalculateDistanceKm(latitude, longitude, record.latitude, record.longitude);
        if (d < minDistance) {
            minDistance = d;
            nearest = record;
            found = true;
        }
    }

    if (found) {
        outHospital = nearest;
        return true;
    }
    return false;
}

std::optional<HospitalRecord> HospitalResourceEngine::FindNearestHospital(double latitude, double longitude,
                                                                         HospitalCapability requiredCapability) const {
    HospitalRecord out;
    if (FindNearestHospital(latitude, longitude, out, requiredCapability)) {
        return out;
    }
    return std::nullopt;
}

HospitalResourceEngine::ScoredCandidate HospitalResourceEngine::EvaluateCandidate(
    const HospitalRecord& hospital, const HospitalSelection& request) const {

    ScoredCandidate cand;
    cand.hospitalId = hospital.hospitalId;
    cand.hospitalName = hospital.hospitalName;
    cand.availableBeds = hospital.bedAvailability.availableBeds;
    cand.availableIcuBeds = hospital.icuBeds.availableIcuBeds;
    cand.availableVentilators = hospital.ventilatorStatus.availableVentilators;
    cand.availableDoctors = hospital.doctorAvailability.availableDoctors;

    // Rule 9: Hospital Operational Status
    if (hospital.status == HospitalStatus::OFFLINE) {
        cand.score = -2000.0;
        cand.reason = "Disqualified: Hospital Telemetry Offline";
        return cand;
    }
    if (hospital.status == HospitalStatus::MAINTENANCE) {
        cand.score = -2000.0;
        cand.reason = "Disqualified: Hospital Under Maintenance / Diversion";
        return cand;
    }
    if (hospital.status == HospitalStatus::FULL) {
        cand.score = -2000.0;
        cand.reason = "Disqualified: Hospital Full";
        return cand;
    }
    if (hospital.status == HospitalStatus::EMERGENCY_ONLY && request.patientAcuityLevel > 1) {
        cand.score = -1000.0;
        cand.reason = "Disqualified: Emergency Only - Non-Critical Acuity Diverted";
        return cand;
    }

    // Rule 3: Hospital Specialty Compatibility
    bool hasSpecialty = hospital.HasCapability(request.requiredSpecialty);
    bool isMultispeciality = hospital.HasCapability(HospitalCapability::MULTISPECIALITY);
    if (request.requiredSpecialty != HospitalCapability::GENERAL && !hasSpecialty && !isMultispeciality) {
        if (request.patientAcuityLevel == 1) {
            cand.score = -1500.0;
            cand.reason = "Disqualified: Lacks Critical Required Specialty";
            return cand;
        } else {
            cand.score -= 40.0; // Substantial penalty for non-specialized general facility
        }
    }

    // Rule 5: ICU Availability
    if (request.requiresICU && hospital.icuBeds.availableIcuBeds == 0) {
        cand.score = -1500.0;
        cand.reason = "Disqualified: No ICU Beds Available";
        return cand;
    }

    // Rule 6: Ventilator Availability
    if (request.requiresVentilator && hospital.ventilatorStatus.availableVentilators == 0) {
        cand.score = -1500.0;
        cand.reason = "Disqualified: No Ventilators Available";
        return cand;
    }

    // Rule 4: Emergency Bed Availability
    if (hospital.bedAvailability.availableBeds == 0 && hospital.icuBeds.availableIcuBeds == 0) {
        cand.score = -1000.0;
        cand.reason = "Disqualified: Zero Total Available Beds";
        return cand;
    }

    // -------------------------------------------------------------------------
    // Rule Scoring Aggregation
    // -------------------------------------------------------------------------
    double totalScore = 0.0;

    // Rule 1: Distance (Haversine km)
    cand.distanceKm = CalculateDistanceKm(request.incidentLatitude, request.incidentLongitude,
                                          hospital.latitude, hospital.longitude);
    double scoreDistance = std::max(0.0, 50.0 - (cand.distanceKm * 2.0));
    totalScore += scoreDistance;

    // Rule 2: Travel ETA
    double speedKmH = (request.currentSpeedKmH > 5.0) ? request.currentSpeedKmH : 50.0;
    double travelMinutes = (cand.distanceKm / speedKmH) * 60.0;
    double waitDelayMinutes = hospital.emergencyDept.averageWaitTimeMinutes * 0.25;
    cand.etaMinutes = travelMinutes + waitDelayMinutes;
    double scoreEta = std::max(0.0, 60.0 - (cand.etaMinutes * 2.5));
    totalScore += scoreEta;

    // Rule 3 (Score component): Specialty match bonus
    if (hasSpecialty) {
        cand.matchedCapability = request.requiredSpecialty;
        totalScore += 35.0;
    } else if (isMultispeciality) {
        cand.matchedCapability = HospitalCapability::MULTISPECIALITY;
        totalScore += 28.0;
    } else {
        cand.matchedCapability = HospitalCapability::GENERAL;
        totalScore += 10.0;
    }

    // Rule 4 (Score component): Bed Availability
    double bedRatio = (hospital.bedAvailability.totalBeds > 0)
        ? (static_cast<double>(hospital.bedAvailability.availableBeds) / static_cast<double>(hospital.bedAvailability.totalBeds))
        : 0.0;
    double scoreBed = std::min(25.0, static_cast<double>(hospital.bedAvailability.availableBeds) * 0.8) + (bedRatio * 15.0);
    totalScore += scoreBed;

    // Rule 5 (Score component): ICU Availability
    if (request.requiresICU) {
        double scoreIcu = 25.0 + std::min(20.0, static_cast<double>(hospital.icuBeds.availableIcuBeds) * 2.0);
        totalScore += scoreIcu;
    } else {
        double scoreIcu = std::min(6.0, static_cast<double>(hospital.icuBeds.availableIcuBeds) * 0.5);
        totalScore += scoreIcu;
    }

    // Rule 6 (Score component): Ventilator Availability
    if (request.requiresVentilator) {
        double scoreVent = 20.0 + std::min(15.0, static_cast<double>(hospital.ventilatorStatus.availableVentilators) * 2.0);
        totalScore += scoreVent;
    } else {
        double scoreVent = std::min(5.0, static_cast<double>(hospital.ventilatorStatus.availableVentilators) * 0.5);
        totalScore += scoreVent;
    }

    // Rule 7: Doctor Availability
    double scoreDoctors = std::min(15.0, static_cast<double>(hospital.doctorAvailability.availableDoctors) * 1.5);
    // Specialist bonus if specific specialty matches
    if (request.requiredSpecialty == HospitalCapability::TRAUMA && hospital.doctorAvailability.traumaSurgeonsAvailable > 0) {
        scoreDoctors += 15.0;
    } else if (request.requiredSpecialty == HospitalCapability::CARDIAC && hospital.doctorAvailability.cardiologistsAvailable > 0) {
        scoreDoctors += 15.0;
    } else if (request.requiredSpecialty == HospitalCapability::NEURO && hospital.doctorAvailability.neurologistsAvailable > 0) {
        scoreDoctors += 15.0;
    } else if (request.requiredSpecialty == HospitalCapability::PEDIATRIC && hospital.doctorAvailability.pediatriciansAvailable > 0) {
        scoreDoctors += 15.0;
    }
    totalScore += scoreDoctors;

    // Rule 8: Current Emergency Load
    double occupancyPenalty = hospital.emergencyDept.occupancyPercentage * 0.3;
    double queuePenalty = static_cast<double>(hospital.emergencyDept.triageQueueLength) * 1.5;
    if (hospital.emergencyDept.diversionActive) {
        occupancyPenalty += 40.0;
    }
    totalScore -= (occupancyPenalty + queuePenalty);

    // Rule 9 (Score component): Operational Status bonus
    if (hospital.status == HospitalStatus::ONLINE) {
        totalScore += 15.0;
    } else if (hospital.status == HospitalStatus::EMERGENCY_ONLY && request.patientAcuityLevel == 1) {
        totalScore += 20.0; // High bonus for critical patient routed to specialized emergency center
    }

    // Rule 10: Predicted Arrival Readiness
    double readinessBonus = 0.0;
    if (hospital.bedAvailability.availableBeds >= 2 && hospital.emergencyDept.triageQueueLength <= 1) {
        readinessBonus = 15.0;
    } else if (hospital.emergencyDept.triageQueueLength > 4) {
        readinessBonus = -5.0;
    }
    totalScore += readinessBonus;

    cand.score = totalScore;

    // Formulate reason and recommendation
    std::ostringstream ss;
    ss << "Optimal Destination: " << HospitalCapabilityToString(cand.matchedCapability)
       << " center with " << cand.availableBeds << " beds, "
       << cand.availableIcuBeds << " ICU, " << cand.availableVentilators << " vents";
    cand.recommendationSummary = ss.str();
    cand.reason = "Multi-criteria deterministic optimization satisfied all 10 clinical and operational rules";

    return cand;
}

HospitalRecommendation HospitalResourceEngine::FindBestHospital(const HospitalSelection& request) {
    auto startTime = std::chrono::high_resolution_clock::now();

    HospitalRecommendation recommendation;
    recommendation.incidentId = request.incidentId;
    recommendation.ambulanceId = request.ambulanceId;
    recommendation.decisionTimestamp = GetCurrentTimeMs();

    std::lock_guard<std::mutex> regLock(registryMutex_);
    if (hospitals_.empty()) {
        recommendation.successful = false;
        recommendation.reason = "No hospitals registered in municipal database";
        return recommendation;
    }

    std::vector<ScoredCandidate> candidates;
    candidates.reserve(hospitals_.size());

    for (const auto& pair : hospitals_) {
        ScoredCandidate cand = EvaluateCandidate(pair.second, request);
        if (cand.score > 0.0) {
            candidates.push_back(cand);
        }
    }

    if (candidates.empty()) {
        recommendation.successful = false;
        recommendation.reason = "No eligible hospital satisfied operational and specialty constraints";
        return recommendation;
    }

    // Sort deterministically: highest score first; tie-break by shortest distance, then hospitalId
    std::sort(candidates.begin(), candidates.end(), [](const ScoredCandidate& a, const ScoredCandidate& b) {
        if (std::abs(a.score - b.score) > kScoreEpsilon) {
            return a.score > b.score;
        }
        if (std::abs(a.distanceKm - b.distanceKm) > kScoreEpsilon) {
            return a.distanceKm < b.distanceKm;
        }
        return a.hospitalId < b.hospitalId;
    });

    const ScoredCandidate& best = candidates.front();
    recommendation.selectedHospitalId = best.hospitalId;
    recommendation.hospitalName = best.hospitalName;
    recommendation.matchedCapability = best.matchedCapability;
    recommendation.distanceKm = best.distanceKm;
    recommendation.estimatedEtaMinutes = best.etaMinutes;
    recommendation.availableBeds = best.availableBeds;
    recommendation.availableIcuBeds = best.availableIcuBeds;
    recommendation.availableVentilators = best.availableVentilators;
    recommendation.availableDoctors = best.availableDoctors;
    recommendation.score = best.score;
    recommendation.recommendationSummary = best.recommendationSummary;
    recommendation.reason = best.reason;
    recommendation.successful = true;

    auto endTime = std::chrono::high_resolution_clock::now();
    double latencyMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    recommendation.calculationTimeMs = latencyMs;

    // Record to history and update statistics
    {
        std::lock_guard<std::mutex> histLock(historyMutex_);
        history_.push_back(recommendation);
    }

    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        uint64_t prevCount = statistics_.totalSelectionsMade;
        statistics_.totalSelectionsMade++;
        statistics_.averageSelectionLatencyMs = 
            ((statistics_.averageSelectionLatencyMs * static_cast<double>(prevCount)) + latencyMs) /
            static_cast<double>(statistics_.totalSelectionsMade);
    }

    return recommendation;
}

// =============================================================================
// Reservation Management APIs
// =============================================================================

bool HospitalResourceEngine::ReserveBed(const std::string& hospitalId, const std::string& reservationId) {
    std::string resId = reservationId;
    if (resId.empty()) {
        std::lock_guard<std::mutex> resLock(reservationsMutex_);
        resId = "RES_BED_" + std::to_string(++reservationCounter_);
    }

    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = hospitals_.find(hospitalId);
    if (it == hospitals_.end() || it->second.bedAvailability.availableBeds == 0) {
        return false;
    }

    it->second.bedAvailability.availableBeds--;
    it->second.bedAvailability.reservedBeds++;
    if (it->second.bedAvailability.totalBeds > 0) {
        it->second.bedAvailability.occupancyRate = 
            (static_cast<double>(it->second.bedAvailability.totalBeds - it->second.bedAvailability.availableBeds) /
             static_cast<double>(it->second.bedAvailability.totalBeds)) * 100.0;
    }
    it->second.lastUpdatedTimestamp = GetCurrentTimeMs();

    {
        std::lock_guard<std::mutex> resLock(reservationsMutex_);
        bedReservations_[resId] = hospitalId;
    }

    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        if (statistics_.availableBeds > 0) statistics_.availableBeds--;
        statistics_.reservedBeds++;
        statistics_.totalBedsReserved++;
    }

    return true;
}

bool HospitalResourceEngine::ReleaseBed(const std::string& hospitalId, const std::string& reservationId) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = hospitals_.find(hospitalId);
    if (it == hospitals_.end() || it->second.bedAvailability.reservedBeds == 0) {
        return false;
    }

    it->second.bedAvailability.reservedBeds--;
    it->second.bedAvailability.availableBeds++;
    if (it->second.bedAvailability.totalBeds > 0) {
        it->second.bedAvailability.occupancyRate = 
            (static_cast<double>(it->second.bedAvailability.totalBeds - it->second.bedAvailability.availableBeds) /
             static_cast<double>(it->second.bedAvailability.totalBeds)) * 100.0;
    }
    it->second.lastUpdatedTimestamp = GetCurrentTimeMs();

    if (!reservationId.empty()) {
        std::lock_guard<std::mutex> resLock(reservationsMutex_);
        bedReservations_.erase(reservationId);
    }

    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        if (statistics_.reservedBeds > 0) statistics_.reservedBeds--;
        statistics_.availableBeds++;
        statistics_.totalBedsReleased++;
    }

    return true;
}

bool HospitalResourceEngine::ReserveICUBed(const std::string& hospitalId, const std::string& reservationId) {
    std::string resId = reservationId;
    if (resId.empty()) {
        std::lock_guard<std::mutex> resLock(reservationsMutex_);
        resId = "RES_ICU_" + std::to_string(++reservationCounter_);
    }

    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = hospitals_.find(hospitalId);
    if (it == hospitals_.end() || it->second.icuBeds.availableIcuBeds == 0) {
        return false;
    }

    it->second.icuBeds.availableIcuBeds--;
    it->second.icuBeds.reservedIcuBeds++;
    if (it->second.icuBeds.totalIcuBeds > 0) {
        it->second.icuBeds.icuOccupancyRate = 
            (static_cast<double>(it->second.icuBeds.totalIcuBeds - it->second.icuBeds.availableIcuBeds) /
             static_cast<double>(it->second.icuBeds.totalIcuBeds)) * 100.0;
    }
    it->second.lastUpdatedTimestamp = GetCurrentTimeMs();

    {
        std::lock_guard<std::mutex> resLock(reservationsMutex_);
        icuReservations_[resId] = hospitalId;
    }

    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        if (statistics_.availableIcuBeds > 0) statistics_.availableIcuBeds--;
        statistics_.reservedIcuBeds++;
        statistics_.totalIcuBedsReserved++;
    }

    return true;
}

bool HospitalResourceEngine::ReleaseICUBed(const std::string& hospitalId, const std::string& reservationId) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = hospitals_.find(hospitalId);
    if (it == hospitals_.end() || it->second.icuBeds.reservedIcuBeds == 0) {
        return false;
    }

    it->second.icuBeds.reservedIcuBeds--;
    it->second.icuBeds.availableIcuBeds++;
    if (it->second.icuBeds.totalIcuBeds > 0) {
        it->second.icuBeds.icuOccupancyRate = 
            (static_cast<double>(it->second.icuBeds.totalIcuBeds - it->second.icuBeds.availableIcuBeds) /
             static_cast<double>(it->second.icuBeds.totalIcuBeds)) * 100.0;
    }
    it->second.lastUpdatedTimestamp = GetCurrentTimeMs();

    if (!reservationId.empty()) {
        std::lock_guard<std::mutex> resLock(reservationsMutex_);
        icuReservations_.erase(reservationId);
    }

    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        if (statistics_.reservedIcuBeds > 0) statistics_.reservedIcuBeds--;
        statistics_.availableIcuBeds++;
        statistics_.totalIcuBedsReleased++;
    }

    return true;
}

// =============================================================================
// Reporting & Statistics APIs
// =============================================================================

std::string HospitalResourceEngine::GenerateHospitalReport(const std::string& hospitalId) const {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = hospitals_.find(hospitalId);
    if (it == hospitals_.end()) {
        return "[HOSPITAL]\n\nHospital:\nUNKNOWN\n\nRecommendation:\nHospital ID not found\n";
    }

    const HospitalRecord& h = it->second;
    std::string primaryCap = h.capabilities.empty() ? "GENERAL" : HospitalCapabilityToString(h.capabilities.front());

    std::ostringstream ss;
    ss << "[HOSPITAL]\n\n"
       << "Hospital:\n" << h.hospitalName << "\n\n"
       << "Capability:\n" << primaryCap << "\n\n"
       << "Available Beds:\n" << h.bedAvailability.availableBeds << "\n\n"
       << "ICU Beds:\n" << h.icuBeds.availableIcuBeds << "\n\n"
       << "Ventilators:\n" << h.ventilatorStatus.availableVentilators << "\n\n"
       << "ETA:\n0.0 min\n\n"
       << "Recommendation:\nFacility operational with live capacity\n";

    std::string report = ss.str();
    {
        std::lock_guard<std::mutex> repLock(reportsMutex_);
        const_cast<HospitalResourceEngine*>(this)->reports_.push_back(report);
    }
    return report;
}

std::string HospitalResourceEngine::GenerateHospitalReport(const HospitalRecommendation& rec) const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "[HOSPITAL]\n\n"
       << "Hospital:\n" << (rec.hospitalName.empty() ? rec.selectedHospitalId : rec.hospitalName) << "\n\n"
       << "Capability:\n" << HospitalCapabilityToString(rec.matchedCapability) << "\n\n"
       << "Available Beds:\n" << rec.availableBeds << "\n\n"
       << "ICU Beds:\n" << rec.availableIcuBeds << "\n\n"
       << "Ventilators:\n" << rec.availableVentilators << "\n\n"
       << "ETA:\n" << rec.estimatedEtaMinutes << " min\n\n"
       << "Recommendation:\n" << (rec.recommendationSummary.empty() ? rec.reason : rec.recommendationSummary) << "\n";

    std::string report = ss.str();
    {
        std::lock_guard<std::mutex> repLock(reportsMutex_);
        const_cast<HospitalResourceEngine*>(this)->reports_.push_back(report);
    }
    return report;
}

HospitalStatistics HospitalResourceEngine::GetStatistics() const {
    std::lock_guard<std::mutex> statsLock(statisticsMutex_);
    return statistics_;
}

bool HospitalResourceEngine::GetHospitalStatus(const std::string& hospitalId, HospitalStatus& outStatus) const {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = hospitals_.find(hospitalId);
    if (it == hospitals_.end()) {
        return false;
    }
    outStatus = it->second.status;
    return true;
}

HospitalStatus HospitalResourceEngine::GetHospitalStatus(const std::string& hospitalId) const {
    HospitalStatus st = HospitalStatus::OFFLINE;
    GetHospitalStatus(hospitalId, st);
    return st;
}

bool HospitalResourceEngine::GetHospital(const std::string& hospitalId, HospitalRecord& outHospital) const {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = hospitals_.find(hospitalId);
    if (it == hospitals_.end()) {
        return false;
    }
    outHospital = it->second;
    return true;
}

std::vector<HospitalRecord> HospitalResourceEngine::GetAllHospitals() const {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    std::vector<HospitalRecord> result;
    result.reserve(hospitals_.size());
    for (const auto& pair : hospitals_) {
        result.push_back(pair.second);
    }
    return result;
}

void HospitalResourceEngine::ClearHistory() {
    {
        std::lock_guard<std::mutex> histLock(historyMutex_);
        history_.clear();
    }
    {
        std::lock_guard<std::mutex> repLock(reportsMutex_);
        reports_.clear();
    }
    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        statistics_.totalSelectionsMade = 0;
        statistics_.averageSelectionLatencyMs = 0.0;
        statistics_.totalBedsReserved = 0;
        statistics_.totalIcuBedsReserved = 0;
        statistics_.totalBedsReleased = 0;
        statistics_.totalIcuBedsReleased = 0;
    }
}

void HospitalResourceEngine::Reset() {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    std::lock_guard<std::mutex> statsLock(statisticsMutex_);
    std::lock_guard<std::mutex> repLock(reportsMutex_);
    std::lock_guard<std::mutex> resLock(reservationsMutex_);
    std::lock_guard<std::mutex> histLock(historyMutex_);

    hospitals_.clear();
    bedReservations_.clear();
    icuReservations_.clear();
    history_.clear();
    reports_.clear();
    statistics_ = HospitalStatistics{};
    reservationCounter_ = 0;
}

} // namespace hospital_management
} // namespace aegis

