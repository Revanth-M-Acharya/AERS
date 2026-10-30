/**
 * @file HospitalSelectionEngine.cpp
 * @brief SMART AEGIS (AERS) Phase 11B - Hospital Capacity Intelligence & Smart Hospital Selection Engine
 *
 * Implements deterministic hospital ranking, real-time capacity and queue evaluation,
 * and failover handling for emergency ambulance routing.
 *
 * Pure C++17, QNX SDP 8.0 & Raspberry Pi 4 Starter Kit compliant.
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#include "HospitalSelectionEngine.h"

#include <cmath>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace aegis {
namespace hospital {

namespace {
constexpr double PI = 3.14159265358979323846;
constexpr double EARTH_RADIUS_KM = 6371.0;
constexpr double DEFAULT_AMBULANCE_SPEED_KMH = 50.0;

double DegreesToRadians(double degrees) {
    return degrees * (PI / 180.0);
}
} // anonymous namespace

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
        case HospitalCapability::GENERAL:    return "GENERAL";
        case HospitalCapability::TRAUMA:     return "TRAUMA";
        case HospitalCapability::CARDIAC:    return "CARDIAC";
        case HospitalCapability::STROKE:     return "STROKE";
        case HospitalCapability::ICU:        return "ICU";
        case HospitalCapability::PEDIATRIC:  return "PEDIATRIC";
        case HospitalCapability::NEONATAL:   return "NEONATAL";
        case HospitalCapability::BURN:       return "BURN";
        case HospitalCapability::ORTHOPEDIC: return "ORTHOPEDIC";
        default:                             return "UNKNOWN";
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
    } else if (str == "STROKE") {
        outCap = HospitalCapability::STROKE;
        return true;
    } else if (str == "ICU") {
        outCap = HospitalCapability::ICU;
        return true;
    } else if (str == "PEDIATRIC") {
        outCap = HospitalCapability::PEDIATRIC;
        return true;
    } else if (str == "NEONATAL") {
        outCap = HospitalCapability::NEONATAL;
        return true;
    } else if (str == "BURN") {
        outCap = HospitalCapability::BURN;
        return true;
    } else if (str == "ORTHOPEDIC") {
        outCap = HospitalCapability::ORTHOPEDIC;
        return true;
    }
    return false;
}

bool HospitalRecord::HasCapability(HospitalCapability cap) const {
    for (const auto& c : capabilities) {
        if (c == cap) {
            return true;
        }
    }
    return false;
}

HospitalSelectionEngine::HospitalSelectionEngine() = default;

double HospitalSelectionEngine::CalculateDistanceKmInternal(double lat1, double lon1, double lat2, double lon2) {
    double dLat = DegreesToRadians(lat2 - lat1);
    double dLon = DegreesToRadians(lon2 - lon1);
    double radLat1 = DegreesToRadians(lat1);
    double radLat2 = DegreesToRadians(lat2);

    double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
               std::cos(radLat1) * std::cos(radLat2) *
               std::sin(dLon / 2.0) * std::sin(dLon / 2.0);

    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return EARTH_RADIUS_KM * c;
}

uint64_t HospitalSelectionEngine::GetCurrentTimeMsInternal() const {
    if (m_simulatedTimeMs > 0) {
        return m_simulatedTimeMs;
    }
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
}

void HospitalSelectionEngine::CheckHeartbeatTimeoutsInternal(uint64_t nowMs) {
    for (auto& pair : m_hospitals) {
        HospitalRecord& h = pair.second;
        if (h.status == HospitalStatus::ONLINE && h.lastHeartbeatTimestamp > 0) {
            if (nowMs >= h.lastHeartbeatTimestamp && (nowMs - h.lastHeartbeatTimestamp) > m_heartbeatTimeoutMs) {
                h.status = HospitalStatus::OFFLINE;
            }
        }
    }
}

bool HospitalSelectionEngine::RegisterHospital(const HospitalRecord& hospital) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (hospital.hospitalId.empty()) {
        return false;
    }

    HospitalRecord record = hospital;
    if (record.capacity.emergencyRoomCapacity > 0) {
        record.capacity.erOccupancyPercentage =
            (static_cast<double>(record.capacity.currentErPatients) /
             static_cast<double>(record.capacity.emergencyRoomCapacity)) * 100.0;
    } else {
        record.capacity.erOccupancyPercentage = 0.0;
    }

    if (record.lastHeartbeatTimestamp == 0) {
        record.lastHeartbeatTimestamp = GetCurrentTimeMsInternal();
    }

    m_hospitals[record.hospitalId] = record;
    return true;
}

bool HospitalSelectionEngine::RemoveHospital(const std::string& hospitalId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_hospitals.erase(hospitalId) > 0;
}

bool HospitalSelectionEngine::UpdateHospitalCapacity(const std::string& hospitalId,
                                                    const HospitalCapacitySnapshot& capacity) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_hospitals.find(hospitalId);
    if (it == m_hospitals.end()) {
        return false;
    }

    it->second.capacity = capacity;
    if (it->second.capacity.emergencyRoomCapacity > 0) {
        it->second.capacity.erOccupancyPercentage =
            (static_cast<double>(it->second.capacity.currentErPatients) /
             static_cast<double>(it->second.capacity.emergencyRoomCapacity)) * 100.0;
    } else {
        it->second.capacity.erOccupancyPercentage = 0.0;
    }

    // Auto-mark FULL if both general and ICU beds are exhausted
    if (it->second.capacity.availableBeds == 0 && it->second.capacity.availableIcuBeds == 0) {
        if (it->second.status == HospitalStatus::ONLINE) {
            it->second.status = HospitalStatus::FULL;
        }
    } else if (it->second.status == HospitalStatus::FULL &&
               (it->second.capacity.availableBeds > 0 || it->second.capacity.availableIcuBeds > 0)) {
        it->second.status = HospitalStatus::ONLINE;
    }

    return true;
}

bool HospitalSelectionEngine::UpdateQueueLength(const std::string& hospitalId,
                                               uint32_t queueLength,
                                               double avgDelayMinutes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_hospitals.find(hospitalId);
    if (it == m_hospitals.end()) {
        return false;
    }

    it->second.queue.ambulanceQueueLength = queueLength;
    it->second.queue.averageUnloadingDelayMinutes = avgDelayMinutes;
    it->second.queue.lastUpdatedTimestamp = GetCurrentTimeMsInternal();
    return true;
}

bool HospitalSelectionEngine::UpdateDoctorAvailability(const std::string& hospitalId,
                                                      uint32_t availableDoctors) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_hospitals.find(hospitalId);
    if (it == m_hospitals.end()) {
        return false;
    }

    it->second.capacity.availableDoctors = availableDoctors;
    it->second.capacity.lastUpdatedTimestamp = GetCurrentTimeMsInternal();
    return true;
}

bool HospitalSelectionEngine::UpdateEmergencyRoomLoad(const std::string& hospitalId,
                                                     uint32_t erPatients) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_hospitals.find(hospitalId);
    if (it == m_hospitals.end()) {
        return false;
    }

    it->second.capacity.currentErPatients = erPatients;
    if (it->second.capacity.emergencyRoomCapacity > 0) {
        it->second.capacity.erOccupancyPercentage =
            (static_cast<double>(erPatients) /
             static_cast<double>(it->second.capacity.emergencyRoomCapacity)) * 100.0;
    }
    it->second.capacity.lastUpdatedTimestamp = GetCurrentTimeMsInternal();
    return true;
}

bool HospitalSelectionEngine::UpdateOperatingTheatreAvailability(const std::string& hospitalId,
                                                               uint32_t availableTheatres) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_hospitals.find(hospitalId);
    if (it == m_hospitals.end()) {
        return false;
    }

    it->second.capacity.availableOperatingTheatres = availableTheatres;
    it->second.capacity.lastUpdatedTimestamp = GetCurrentTimeMsInternal();
    return true;
}

bool HospitalSelectionEngine::UpdateHospitalStatus(const std::string& hospitalId,
                                                  HospitalStatus status) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_hospitals.find(hospitalId);
    if (it == m_hospitals.end()) {
        return false;
    }

    it->second.status = status;
    return true;
}

bool HospitalSelectionEngine::UpdateHeartbeat(const std::string& hospitalId, uint64_t timestampMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_hospitals.find(hospitalId);
    if (it == m_hospitals.end()) {
        return false;
    }

    uint64_t ts = (timestampMs > 0) ? timestampMs : GetCurrentTimeMsInternal();
    it->second.lastHeartbeatTimestamp = ts;

    // Automatic recovery if vehicle/hospital was marked OFFLINE
    if (it->second.status == HospitalStatus::OFFLINE) {
        if (it->second.capacity.availableBeds == 0 && it->second.capacity.availableIcuBeds == 0) {
            it->second.status = HospitalStatus::FULL;
        } else {
            it->second.status = HospitalStatus::ONLINE;
        }
    }

    return true;
}

void HospitalSelectionEngine::SetHeartbeatTimeoutMs(uint64_t timeoutMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_heartbeatTimeoutMs = timeoutMs;
}

uint64_t HospitalSelectionEngine::GetHeartbeatTimeoutMs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_heartbeatTimeoutMs;
}

void HospitalSelectionEngine::SetSimulatedTimeMs(uint64_t simulatedTimeMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_simulatedTimeMs = simulatedTimeMs;
}

uint64_t HospitalSelectionEngine::GetSimulatedTimeMs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_simulatedTimeMs;
}

bool HospitalSelectionEngine::FindNearestHospital(double lat, double lon,
                                                 HospitalRecord& outHospital,
                                                 HospitalCapability requiredCap) {
    std::lock_guard<std::mutex> lock(m_mutex);
    CheckHeartbeatTimeoutsInternal(GetCurrentTimeMsInternal());

    double shortestDist = 1e9;
    bool found = false;

    for (const auto& pair : m_hospitals) {
        const HospitalRecord& h = pair.second;
        if (h.status != HospitalStatus::ONLINE && h.status != HospitalStatus::EMERGENCY_ONLY) {
            continue;
        }

        if (requiredCap != HospitalCapability::GENERAL && !h.HasCapability(requiredCap)) {
            continue;
        }

        double dist = CalculateDistanceKmInternal(lat, lon, h.latitude, h.longitude);
        if (dist < shortestDist) {
            shortestDist = dist;
            outHospital = h;
            found = true;
        }
    }

    return found;
}

HospitalCandidateScore HospitalSelectionEngine::EvaluateCandidateInternal(
    const HospitalRecord& hospital,
    const HospitalSelectionRequest& request,
    double distanceKm,
    double etaMinutes) const
{
    HospitalCandidateScore score;
    score.hospitalId = hospital.hospitalId;
    score.hospitalName = hospital.hospitalName;
    score.distanceKm = distanceKm;
    score.predictedEtaMinutes = etaMinutes;
    score.isEligible = true;

    // Hard Constraint 1: Operational Status
    if (hospital.status == HospitalStatus::OFFLINE) {
        score.isEligible = false;
        score.disqualificationReason = "Hospital Offline";
        score.totalScore = 10000.0;
        return score;
    }
    if (hospital.status == HospitalStatus::MAINTENANCE) {
        score.isEligible = false;
        score.disqualificationReason = "Hospital Maintenance / Diversion";
        score.totalScore = 10000.0;
        return score;
    }
    if (hospital.status == HospitalStatus::FULL) {
        score.isEligible = false;
        score.disqualificationReason = "Hospital At Full Capacity";
        score.totalScore = 9000.0;
        return score;
    }
    if (hospital.status == HospitalStatus::EMERGENCY_ONLY && request.patientSeverity != "CRITICAL") {
        score.isEligible = false;
        score.disqualificationReason = "Hospital Reserved For Critical Emergencies Only";
        score.totalScore = 8000.0;
        return score;
    }

    // Hard Constraint 2: Required Medical Capabilities
    for (const auto& reqCap : request.requiredCapabilities) {
        if (!hospital.HasCapability(reqCap)) {
            score.isEligible = false;
            score.disqualificationReason = std::string("Missing Required Medical Capability: ") +
                                          HospitalCapabilityToString(reqCap);
            score.totalScore = 7000.0;
            return score;
        }
    }

    // Hard Constraint 3: ICU Availability
    if (request.requiresIcu && hospital.capacity.availableIcuBeds == 0) {
        score.isEligible = false;
        score.disqualificationReason = "No Available ICU Beds";
        score.totalScore = 6000.0;
        return score;
    }

    // Hard Constraint 4: Operating Theatre Availability
    if (request.requiresOperatingTheatre && hospital.capacity.availableOperatingTheatres == 0) {
        score.isEligible = false;
        score.disqualificationReason = "No Operating Theatres Available";
        score.totalScore = 5000.0;
        return score;
    }

    // Hard Constraint 5: Bed Availability for non-ICU patients
    if (!request.requiresIcu && hospital.capacity.availableBeds == 0 && hospital.capacity.availableIcuBeds == 0) {
        score.isEligible = false;
        score.disqualificationReason = "No Inpatient Beds Available";
        score.totalScore = 4000.0;
        return score;
    }

    // Rule 1: Shortest Predicted ETA (weight 1.0)
    score.predictedEtaMinutes = etaMinutes;

    // Rule 2: Medical Capability Match
    // Bonus for dedicated centers matching request
    double capBonus = 0.0;
    if (request.requiresIcu && hospital.HasCapability(HospitalCapability::ICU)) {
        capBonus -= 1.0;
    }
    if (hospital.HasCapability(HospitalCapability::TRAUMA)) {
        capBonus -= 0.5;
    }
    score.capabilityMatchScore = capBonus;

    // Rule 3: ICU Availability
    if (request.requiresIcu) {
        // Lower penalty as more ICU beds are available
        score.icuAvailabilityScore = 15.0 / (1.0 + static_cast<double>(hospital.capacity.availableIcuBeds));
    } else {
        score.icuAvailabilityScore = (hospital.capacity.availableIcuBeds == 0) ? 2.0 : 0.0;
    }

    // Rule 4: Bed Availability
    double bedRatio = 0.0;
    if (hospital.capacity.totalBeds > 0) {
        bedRatio = static_cast<double>(hospital.capacity.availableBeds) /
                   static_cast<double>(hospital.capacity.totalBeds);
    }
    score.bedAvailabilityScore = (1.0 - bedRatio) * 8.0;

    // Rule 5: Emergency Room Load
    double erLoadRatio = 0.0;
    if (hospital.capacity.emergencyRoomCapacity > 0) {
        erLoadRatio = static_cast<double>(hospital.capacity.currentErPatients) /
                      static_cast<double>(hospital.capacity.emergencyRoomCapacity);
    }
    score.erLoadScore = erLoadRatio * 10.0;

    // Rule 6: Queue Waiting Time / Ambulance Unload Delay
    double offloadDelay = static_cast<double>(hospital.queue.ambulanceQueueLength) *
                          hospital.queue.averageUnloadingDelayMinutes;
    score.queueDelayScore = offloadDelay * 1.2;

    // Rule 7: Doctor Availability
    if (hospital.capacity.availableDoctors >= 5) {
        score.doctorAvailabilityScore = 0.0;
    } else if (hospital.capacity.availableDoctors > 0) {
        score.doctorAvailabilityScore = static_cast<double>(5 - hospital.capacity.availableDoctors) * 2.0;
    } else {
        score.doctorAvailabilityScore = 15.0; // Physician shortage penalty
    }

    // Rule 8: Operating Theatre Availability
    if (request.requiresOperatingTheatre) {
        score.operatingTheatreScore = 10.0 / (1.0 + static_cast<double>(hospital.capacity.availableOperatingTheatres));
    } else {
        double otRatio = 0.0;
        if (hospital.capacity.totalOperatingTheatres > 0) {
            otRatio = static_cast<double>(hospital.capacity.availableOperatingTheatres) /
                      static_cast<double>(hospital.capacity.totalOperatingTheatres);
        }
        score.operatingTheatreScore = (1.0 - otRatio) * 2.0;
    }

    // Rule 9: Hospital Online Status
    if (hospital.status == HospitalStatus::ONLINE) {
        score.totalScore += 0.0;
    } else if (hospital.status == HospitalStatus::EMERGENCY_ONLY) {
        score.totalScore += 2.0;
    }

    // Rule 10: Hospital Workload Balancing
    double inboundAmbulancePenalty = static_cast<double>(hospital.assignedAmbulancesCount) * 2.5;
    double throughputWorkload = (static_cast<double>(hospital.totalPatientsHandledToday) / 50.0) * 1.0;
    score.workloadBalanceScore = inboundAmbulancePenalty + throughputWorkload;

    // Total Composite Weighted Score (lower = preferred)
    score.totalScore = score.predictedEtaMinutes +
                       score.capabilityMatchScore +
                       score.icuAvailabilityScore +
                       score.bedAvailabilityScore +
                       score.erLoadScore +
                       score.queueDelayScore +
                       score.doctorAvailabilityScore +
                       score.operatingTheatreScore +
                       score.workloadBalanceScore;

    return score;
}

std::string HospitalSelectionEngine::FormatAuditLogInternal(const HospitalSelectionResult& result) const {
    std::ostringstream oss;
    oss << "[HOSPITAL]\n\n"
        << "Incident:\n" << result.incidentId << "\n\n"
        << "Hospital:\n" << result.selectedHospitalId << "\n\n"
        << "ETA:\n" << std::fixed << std::setprecision(1) << result.predictedEtaMinutes << " min\n\n"
        << "ICU Beds:\n" << result.availableIcuBeds << "\n\n"
        << "ER Load:\n" << static_cast<int>(std::round(result.erOccupancyPercentage)) << "%\n\n"
        << "Reason:\n" << result.selectionReason << "\n";
    return oss.str();
}

HospitalSelectionResult HospitalSelectionEngine::FindBestHospital(const HospitalSelectionRequest& request) {
    std::lock_guard<std::mutex> lock(m_mutex);
    uint64_t nowMs = GetCurrentTimeMsInternal();
    CheckHeartbeatTimeoutsInternal(nowMs);

    HospitalSelectionResult result;
    result.incidentId = request.incidentId;
    result.selectionTimestamp = nowMs;
    result.success = false;

    if (m_hospitals.empty()) {
        result.selectionReason = "No Hospitals Registered";
        return result;
    }

    // Evaluate each hospital candidate
    std::vector<HospitalCandidateScore> scoredCandidates;
    scoredCandidates.reserve(m_hospitals.size());

    for (const auto& pair : m_hospitals) {
        const HospitalRecord& h = pair.second;
        double distKm = CalculateDistanceKmInternal(request.incidentLatitude, request.incidentLongitude,
                                                   h.latitude, h.longitude);
        double etaMin = (distKm / DEFAULT_AMBULANCE_SPEED_KMH) * 60.0;

        HospitalCandidateScore cand = EvaluateCandidateInternal(h, request, distKm, etaMin);
        scoredCandidates.push_back(cand);
    }

    // Deterministic sorting comparator
    std::sort(scoredCandidates.begin(), scoredCandidates.end(),
              [this](const HospitalCandidateScore& a, const HospitalCandidateScore& b) {
                  // 1. Eligibility first
                  if (a.isEligible != b.isEligible) {
                      return a.isEligible > b.isEligible;
                  }
                  // 2. Total composite score (lower is better)
                  if (std::abs(a.totalScore - b.totalScore) > 1e-4) {
                      return a.totalScore < b.totalScore;
                  }
                  // 3. Predicted ETA (shorter is better)
                  if (std::abs(a.predictedEtaMinutes - b.predictedEtaMinutes) > 1e-4) {
                      return a.predictedEtaMinutes < b.predictedEtaMinutes;
                  }
                  // 4. ICU beds available (higher is better)
                  auto itA = m_hospitals.find(a.hospitalId);
                  auto itB = m_hospitals.find(b.hospitalId);
                  uint32_t icuA = (itA != m_hospitals.end()) ? itA->second.capacity.availableIcuBeds : 0;
                  uint32_t icuB = (itB != m_hospitals.end()) ? itB->second.capacity.availableIcuBeds : 0;
                  if (icuA != icuB) {
                      return icuA > icuB;
                  }
                  // 5. General beds available (higher is better)
                  uint32_t bedsA = (itA != m_hospitals.end()) ? itA->second.capacity.availableBeds : 0;
                  uint32_t bedsB = (itB != m_hospitals.end()) ? itB->second.capacity.availableBeds : 0;
                  if (bedsA != bedsB) {
                      return bedsA > bedsB;
                  }
                  // 6. Lexicographical tie-breaker
                  return a.hospitalId < b.hospitalId;
              });

    result.rankedCandidates = scoredCandidates;

    if (!scoredCandidates.empty() && scoredCandidates.front().isEligible) {
        const HospitalCandidateScore& best = scoredCandidates.front();
        result.success = true;
        result.selectedHospitalId = best.hospitalId;
        result.selectedHospitalName = best.hospitalName;
        result.distanceKm = best.distanceKm;
        result.predictedEtaMinutes = best.predictedEtaMinutes;

        auto it = m_hospitals.find(best.hospitalId);
        if (it != m_hospitals.end()) {
            result.availableIcuBeds = it->second.capacity.availableIcuBeds;
            result.availableGeneralBeds = it->second.capacity.availableBeds;
            result.erOccupancyPercentage = it->second.capacity.erOccupancyPercentage;

            // Increment assigned ambulances counter for load tracking
            it->second.assignedAmbulancesCount++;

            // Deduce comprehensive selection reason
            if (!request.requiredCapabilities.empty() || request.requiresIcu) {
                result.selectionReason = "Best Capability Match";
            } else if (best.predictedEtaMinutes < 10.0) {
                result.selectionReason = "Optimal Proximity & ETA";
            } else {
                result.selectionReason = "Optimal Balanced Capacity";
            }
        }

        std::string auditLog = FormatAuditLogInternal(result);
        m_structuredLogs.push_back(auditLog);
        m_selectionHistory.push_back(result);
    } else {
        result.success = false;
        result.selectionReason = "No Eligible Hospitals Found";
    }

    return result;
}

HospitalSelectionResult HospitalSelectionEngine::RecomputeDestination(
    const std::string& previousHospitalId,
    const HospitalSelectionRequest& request)
{
    // FindBestHospital already evaluates current status of all hospitals in memory.
    // If previousHospitalId is now FULL or OFFLINE, it will be automatically filtered out.
    (void)previousHospitalId;
    return FindBestHospital(request);
}

std::string HospitalSelectionEngine::GenerateHospitalReport() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::ostringstream oss;
    oss << "========================================================================================\n"
        << "                 SMART AEGIS - MUNICIPAL HOSPITAL CAPACITY REPORT                       \n"
        << "========================================================================================\n"
        << std::left << std::setw(14) << "HOSPITAL ID"
        << std::setw(24) << "FACILITY NAME"
        << std::setw(12) << "STATUS"
        << std::setw(10) << "BEDS"
        << std::setw(10) << "ICU"
        << std::setw(10) << "ER LOAD"
        << std::setw(10) << "QUEUE"
        << "\n"
        << "----------------------------------------------------------------------------------------\n";

    for (const auto& pair : m_hospitals) {
        const HospitalRecord& h = pair.second;
        std::string bedStr = std::to_string(h.capacity.availableBeds) + "/" + std::to_string(h.capacity.totalBeds);
        std::string icuStr = std::to_string(h.capacity.availableIcuBeds) + "/" + std::to_string(h.capacity.totalIcuBeds);
        std::string erStr = std::to_string(static_cast<int>(std::round(h.capacity.erOccupancyPercentage))) + "%";
        std::string qStr = std::to_string(h.queue.ambulanceQueueLength);

        oss << std::left << std::setw(14) << h.hospitalId
            << std::setw(24) << h.hospitalName.substr(0, 22)
            << std::setw(12) << HospitalStatusToString(h.status)
            << std::setw(10) << bedStr
            << std::setw(10) << icuStr
            << std::setw(10) << erStr
            << std::setw(10) << qStr
            << "\n";
    }
    oss << "========================================================================================\n";
    return oss.str();
}

HospitalStatistics HospitalSelectionEngine::GetHospitalStatistics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    HospitalStatistics stats;
    stats.totalHospitals = m_hospitals.size();

    double totalErPct = 0.0;
    double totalDelayMinutes = 0.0;

    for (const auto& pair : m_hospitals) {
        const HospitalRecord& h = pair.second;
        switch (h.status) {
            case HospitalStatus::ONLINE:         stats.onlineHospitals++; break;
            case HospitalStatus::OFFLINE:        stats.offlineHospitals++; break;
            case HospitalStatus::FULL:           stats.fullHospitals++; break;
            case HospitalStatus::EMERGENCY_ONLY: stats.emergencyOnlyHospitals++; break;
            case HospitalStatus::MAINTENANCE:    stats.maintenanceHospitals++; break;
            default:                             break;
        }

        stats.totalBedsCapacity += h.capacity.totalBeds;
        stats.totalAvailableBeds += h.capacity.availableBeds;
        stats.totalIcuCapacity += h.capacity.totalIcuBeds;
        stats.totalAvailableIcuBeds += h.capacity.availableIcuBeds;
        stats.totalOperatingTheatres += h.capacity.totalOperatingTheatres;
        stats.totalAvailableOperatingTheatres += h.capacity.availableOperatingTheatres;
        totalErPct += h.capacity.erOccupancyPercentage;
        stats.totalAmbulanceQueueLength += h.queue.ambulanceQueueLength;
        totalDelayMinutes += h.queue.averageUnloadingDelayMinutes;
        stats.totalPatientsHandledToday += h.totalPatientsHandledToday;
    }

    if (stats.totalHospitals > 0) {
        stats.averageErOccupancyPercentage = totalErPct / static_cast<double>(stats.totalHospitals);
        stats.averageAmbulanceUnloadDelayMinutes = totalDelayMinutes / static_cast<double>(stats.totalHospitals);
    }

    return stats;
}

std::vector<HospitalRecord> HospitalSelectionEngine::GetHospitalRecords() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<HospitalRecord> list;
    list.reserve(m_hospitals.size());
    for (const auto& pair : m_hospitals) {
        list.push_back(pair.second);
    }
    return list;
}

bool HospitalSelectionEngine::GetHospitalRecord(const std::string& hospitalId,
                                              HospitalRecord& outHospital) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_hospitals.find(hospitalId);
    if (it != m_hospitals.end()) {
        outHospital = it->second;
        return true;
    }
    return false;
}

std::vector<std::string> HospitalSelectionEngine::GetStructuredLogs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_structuredLogs;
}

void HospitalSelectionEngine::ClearHistory() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_structuredLogs.clear();
    m_selectionHistory.clear();
}

} // namespace hospital
} // namespace aegis
