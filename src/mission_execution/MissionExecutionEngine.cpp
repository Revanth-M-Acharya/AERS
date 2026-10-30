/**
 * @file MissionExecutionEngine.cpp
 * @brief SMART AEGIS (AERS) Phase 12A - City-Wide Emergency Mission Execution Engine Implementation
 *
 * Implements the highest-level execution layer and master mission coordinator for SMART AEGIS.
 *
 * Pure C++17, QNX SDP 8.0, Raspberry Pi 4 & POSIX compliant.
 * Zero third-party dependencies, thread-safe, deterministic.
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#include "MissionExecutionEngine.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace aegis {
namespace mission {

// =============================================================================
// Constants & Geographic Helpers
// =============================================================================

static constexpr double kEarthRadiusKm = 6371.0;
static constexpr double kDegToRad = 3.14159265358979323846 / 180.0;

const char* MissionStatusToString(MissionStatus status) {
    switch (status) {
        case MissionStatus::NEW:       return "NEW";
        case MissionStatus::READY:     return "READY";
        case MissionStatus::ACTIVE:    return "ACTIVE";
        case MissionStatus::PAUSED:    return "PAUSED";
        case MissionStatus::COMPLETED: return "COMPLETED";
        case MissionStatus::FAILED:    return "FAILED";
        case MissionStatus::ABORTED:   return "ABORTED";
        case MissionStatus::CANCELLED: return "CANCELLED";
        default:                       return "UNKNOWN";
    }
}

bool StringToMissionStatus(const std::string& str, MissionStatus& outStatus) {
    if (str == "NEW") {
        outStatus = MissionStatus::NEW;
        return true;
    } else if (str == "READY") {
        outStatus = MissionStatus::READY;
        return true;
    } else if (str == "ACTIVE") {
        outStatus = MissionStatus::ACTIVE;
        return true;
    } else if (str == "PAUSED") {
        outStatus = MissionStatus::PAUSED;
        return true;
    } else if (str == "COMPLETED") {
        outStatus = MissionStatus::COMPLETED;
        return true;
    } else if (str == "FAILED") {
        outStatus = MissionStatus::FAILED;
        return true;
    } else if (str == "ABORTED") {
        outStatus = MissionStatus::ABORTED;
        return true;
    } else if (str == "CANCELLED") {
        outStatus = MissionStatus::CANCELLED;
        return true;
    }
    return false;
}

const char* MissionStageToString(MissionStage stage) {
    switch (stage) {
        case MissionStage::CALL_RECEIVED:        return "CALL_RECEIVED";
        case MissionStage::AMBULANCE_SELECTION:  return "AMBULANCE_SELECTION";
        case MissionStage::HOSPITAL_SELECTION:   return "HOSPITAL_SELECTION";
        case MissionStage::ROUTE_PLANNING:       return "ROUTE_PLANNING";
        case MissionStage::GREEN_CORRIDOR_SETUP: return "GREEN_CORRIDOR_SETUP";
        case MissionStage::AMBULANCE_EN_ROUTE:   return "AMBULANCE_EN_ROUTE";
        case MissionStage::PATIENT_PICKUP:       return "PATIENT_PICKUP";
        case MissionStage::TRANSPORTING:         return "TRANSPORTING";
        case MissionStage::ARRIVING_HOSPITAL:    return "ARRIVING_HOSPITAL";
        case MissionStage::HANDOVER:             return "HANDOVER";
        case MissionStage::MISSION_COMPLETE:     return "MISSION_COMPLETE";
        default:                                 return "UNKNOWN";
    }
}

bool StringToMissionStage(const std::string& str, MissionStage& outStage) {
    if (str == "CALL_RECEIVED") {
        outStage = MissionStage::CALL_RECEIVED;
        return true;
    } else if (str == "AMBULANCE_SELECTION") {
        outStage = MissionStage::AMBULANCE_SELECTION;
        return true;
    } else if (str == "HOSPITAL_SELECTION") {
        outStage = MissionStage::HOSPITAL_SELECTION;
        return true;
    } else if (str == "ROUTE_PLANNING") {
        outStage = MissionStage::ROUTE_PLANNING;
        return true;
    } else if (str == "GREEN_CORRIDOR_SETUP") {
        outStage = MissionStage::GREEN_CORRIDOR_SETUP;
        return true;
    } else if (str == "AMBULANCE_EN_ROUTE") {
        outStage = MissionStage::AMBULANCE_EN_ROUTE;
        return true;
    } else if (str == "PATIENT_PICKUP") {
        outStage = MissionStage::PATIENT_PICKUP;
        return true;
    } else if (str == "TRANSPORTING") {
        outStage = MissionStage::TRANSPORTING;
        return true;
    } else if (str == "ARRIVING_HOSPITAL") {
        outStage = MissionStage::ARRIVING_HOSPITAL;
        return true;
    } else if (str == "HANDOVER") {
        outStage = MissionStage::HANDOVER;
        return true;
    } else if (str == "MISSION_COMPLETE") {
        outStage = MissionStage::MISSION_COMPLETE;
        return true;
    }
    return false;
}

const char* MissionPriorityToString(MissionPriority priority) {
    switch (priority) {
        case MissionPriority::LOW:           return "LOW";
        case MissionPriority::MEDIUM:        return "MEDIUM";
        case MissionPriority::HIGH:          return "HIGH";
        case MissionPriority::CRITICAL:      return "CRITICAL";
        case MissionPriority::MASS_CASUALTY: return "MASS_CASUALTY";
        default:                             return "UNKNOWN";
    }
}

bool StringToMissionPriority(const std::string& str, MissionPriority& outPriority) {
    if (str == "LOW") {
        outPriority = MissionPriority::LOW;
        return true;
    } else if (str == "MEDIUM") {
        outPriority = MissionPriority::MEDIUM;
        return true;
    } else if (str == "HIGH") {
        outPriority = MissionPriority::HIGH;
        return true;
    } else if (str == "CRITICAL") {
        outPriority = MissionPriority::CRITICAL;
        return true;
    } else if (str == "MASS_CASUALTY") {
        outPriority = MissionPriority::MASS_CASUALTY;
        return true;
    }
    return false;
}

const char* MissionEventTypeToString(MissionEventType eventType) {
    switch (eventType) {
        case MissionEventType::MISSION_CREATED:         return "Mission Created";
        case MissionEventType::PRIORITY_ASSIGNED:        return "Priority Assigned";
        case MissionEventType::AMBULANCE_ASSIGNED:       return "Ambulance Assigned";
        case MissionEventType::HOSPITAL_ASSIGNED:        return "Hospital Assigned";
        case MissionEventType::ROUTE_GENERATED:          return "Route Generated";
        case MissionEventType::GREEN_CORRIDOR_STARTED:   return "Green Corridor Started";
        case MissionEventType::TRAFFIC_PLAN_GENERATED:   return "Traffic Plan Generated";
        case MissionEventType::MISSION_STARTED:          return "Mission Started";
        case MissionEventType::PATIENT_PICKED_UP:        return "Patient Picked Up";
        case MissionEventType::ROUTE_REPLANNED:          return "Route Replanned";
        case MissionEventType::HOSPITAL_UPDATED:         return "Hospital Updated";
        case MissionEventType::TRAFFIC_DELAY:            return "Traffic Delay";
        case MissionEventType::ETA_UPDATED:              return "ETA Updated";
        case MissionEventType::AMBULANCE_ARRIVED:        return "Ambulance Arrived";
        case MissionEventType::PATIENT_HANDED_OVER:      return "Patient Handed Over";
        case MissionEventType::MISSION_COMPLETED:        return "Mission Completed";
        case MissionEventType::MISSION_CANCELLED:        return "Mission Cancelled";
        case MissionEventType::MISSION_FAILED:           return "Mission Failed";
        default:                                         return "Unknown Event";
    }
}

// =============================================================================
// Constructor & Destructor
// =============================================================================

MissionExecutionEngine::MissionExecutionEngine() {
    Reset();
}

uint64_t MissionExecutionEngine::GetCurrentTimeMs() const {
    auto now = std::chrono::steady_clock::now();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count());
}

std::string MissionExecutionEngine::FormatTimestamp(uint64_t timestampMs) const {
    // Format timestamp in HH:MM:SS format
    std::time_t seconds = static_cast<std::time_t>(timestampMs / 1000);
    std::tm timeInfo{};
#if defined(_WIN32)
    gmtime_s(&timeInfo, &seconds);
#else
    gmtime_r(&seconds, &timeInfo);
#endif
    std::ostringstream ss;
    ss << std::setfill('0')
       << std::setw(2) << timeInfo.tm_hour << ":"
       << std::setw(2) << timeInfo.tm_min << ":"
       << std::setw(2) << timeInfo.tm_sec;
    return ss.str();
}

double MissionExecutionEngine::CalculateDistanceKm(double lat1, double lon1, double lat2, double lon2) {
    double dLat = (lat2 - lat1) * kDegToRad;
    double dLon = (lon2 - lon1) * kDegToRad;
    double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
               std::cos(lat1 * kDegToRad) * std::cos(lat2 * kDegToRad) *
               std::sin(dLon / 2.0) * std::sin(dLon / 2.0);
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return kEarthRadiusKm * c;
}

// =============================================================================
// State Machine Transitions & Validation
// =============================================================================

bool MissionExecutionEngine::ValidateTransition(MissionStatus from, MissionStatus to) const {
    if (from == to) {
        return true;
    }

    switch (from) {
        case MissionStatus::NEW:
            return to == MissionStatus::READY ||
                   to == MissionStatus::CANCELLED ||
                   to == MissionStatus::ABORTED;

        case MissionStatus::READY:
            return to == MissionStatus::ACTIVE ||
                   to == MissionStatus::CANCELLED ||
                   to == MissionStatus::ABORTED;

        case MissionStatus::ACTIVE:
            return to == MissionStatus::PAUSED ||
                   to == MissionStatus::COMPLETED ||
                   to == MissionStatus::FAILED ||
                   to == MissionStatus::ABORTED ||
                   to == MissionStatus::CANCELLED;

        case MissionStatus::PAUSED:
            return to == MissionStatus::ACTIVE ||
                   to == MissionStatus::FAILED ||
                   to == MissionStatus::ABORTED ||
                   to == MissionStatus::CANCELLED;

        case MissionStatus::COMPLETED:
        case MissionStatus::FAILED:
        case MissionStatus::ABORTED:
        case MissionStatus::CANCELLED:
        default:
            return false; // Terminal states cannot transition to other active states
    }
}

bool MissionExecutionEngine::ValidateStageProgression(MissionStage from, MissionStage to) const {
    if (from == to) {
        return true;
    }
    // Strict forward progression across stages
    auto fromVal = static_cast<int>(from);
    auto toVal = static_cast<int>(to);
    return toVal > fromVal;
}

// =============================================================================
// Mission Creation & Lifecycle APIs
// =============================================================================

bool MissionExecutionEngine::CreateMission(const MissionRecord& mission) {
    // Rule 1: Mission ID unique & non-empty
    if (mission.missionId.empty()) {
        return false;
    }

    // Rule 2: Emergency ID valid & non-empty
    if (mission.emergencyId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> regLock(registryMutex_);
    if (missions_.find(mission.missionId) != missions_.end()) {
        return false; // Duplicate mission ID rejected
    }

    MissionRecord record = mission;
    uint64_t now = GetCurrentTimeMs();
    if (record.missionCreationTimestamp == 0) {
        record.missionCreationTimestamp = now;
    }
    record.status = MissionStatus::NEW;
    record.stage = MissionStage::CALL_RECEIVED;

    missions_[record.missionId] = record;
    if (!record.incidentId.empty()) {
        incidentMap_[record.incidentId] = record.missionId;
    }

    // Record in Timeline
    {
        std::lock_guard<std::mutex> timeLock(timelineMutex_);
        timelines_[record.missionId].emplace_back(
            now, FormatTimestamp(now), MissionEventType::MISSION_CREATED,
            "Mission Created for Emergency " + record.emergencyId,
            MissionStage::CALL_RECEIVED, MissionStatus::NEW
        );
        timelines_[record.missionId].emplace_back(
            now, FormatTimestamp(now), MissionEventType::PRIORITY_ASSIGNED,
            std::string("Priority Assigned: ") + MissionPriorityToString(record.priority),
            MissionStage::CALL_RECEIVED, MissionStatus::NEW
        );
    }

    // Update Statistics
    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        RecalculateStatisticsLocked();
    }

    return true;
}

bool MissionExecutionEngine::StartMission(const std::string& missionId) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end()) {
        return false;
    }

    MissionRecord& m = it->second;

    // Check prerequisites if coming from NEW
    if (m.status == MissionStatus::NEW) {
        // Transition to READY if assigned
        if (!m.ambulanceId.empty() && !m.hospitalId.empty()) {
            m.status = MissionStatus::READY;
        } else {
            return false; // Cannot start without required assignments
        }
    }

    if (!ValidateTransition(m.status, MissionStatus::ACTIVE)) {
        return false;
    }

    // Rule 3: Ambulance assigned
    if (m.ambulanceId.empty()) {
        return false;
    }

    // Rule 4: Hospital assigned
    if (m.hospitalId.empty()) {
        return false;
    }

    uint64_t now = GetCurrentTimeMs();
    m.status = MissionStatus::ACTIVE;
    m.stage = MissionStage::AMBULANCE_EN_ROUTE;
    if (m.dispatchTimestamp == 0) {
        m.dispatchTimestamp = now;
    }

    {
        std::lock_guard<std::mutex> timeLock(timelineMutex_);
        timelines_[m.missionId].emplace_back(
            now, FormatTimestamp(now), MissionEventType::MISSION_STARTED,
            "Mission Started: Ambulance En Route to Scene",
            MissionStage::AMBULANCE_EN_ROUTE, MissionStatus::ACTIVE
        );
    }

    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        RecalculateStatisticsLocked();
    }

    return true;
}

bool MissionExecutionEngine::PauseMission(const std::string& missionId, const std::string& reason) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end() || it->second.status != MissionStatus::ACTIVE) {
        return false;
    }

    it->second.status = MissionStatus::PAUSED;
    if (!reason.empty()) {
        it->second.missionNotes += " [Paused: " + reason + "]";
    }

    uint64_t now = GetCurrentTimeMs();
    {
        std::lock_guard<std::mutex> timeLock(timelineMutex_);
        timelines_[missionId].emplace_back(
            now, FormatTimestamp(now), MissionEventType::TRAFFIC_DELAY,
            "Mission Paused: " + (reason.empty() ? "Operational Hold" : reason),
            it->second.stage, MissionStatus::PAUSED
        );
    }

    return true;
}

bool MissionExecutionEngine::ResumeMission(const std::string& missionId) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end() || it->second.status != MissionStatus::PAUSED) {
        return false;
    }

    it->second.status = MissionStatus::ACTIVE;
    uint64_t now = GetCurrentTimeMs();
    {
        std::lock_guard<std::mutex> timeLock(timelineMutex_);
        timelines_[missionId].emplace_back(
            now, FormatTimestamp(now), MissionEventType::MISSION_STARTED,
            "Mission Resumed to Active Execution",
            it->second.stage, MissionStatus::ACTIVE
        );
    }

    return true;
}

bool MissionExecutionEngine::AbortMission(const std::string& missionId, const std::string& reason) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end() || it->second.IsTerminal()) {
        return false;
    }

    it->second.status = MissionStatus::ABORTED;
    it->second.failureReason = reason;

    uint64_t now = GetCurrentTimeMs();
    {
        std::lock_guard<std::mutex> timeLock(timelineMutex_);
        timelines_[missionId].emplace_back(
            now, FormatTimestamp(now), MissionEventType::MISSION_FAILED,
            "Mission Aborted: " + reason,
            it->second.stage, MissionStatus::ABORTED
        );
    }

    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        RecalculateStatisticsLocked();
    }

    return true;
}

bool MissionExecutionEngine::CancelMission(const std::string& missionId, const std::string& reason) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end() || it->second.IsTerminal()) {
        return false;
    }

    it->second.status = MissionStatus::CANCELLED;
    it->second.cancellationReason = reason;

    uint64_t now = GetCurrentTimeMs();
    {
        std::lock_guard<std::mutex> timeLock(timelineMutex_);
        timelines_[missionId].emplace_back(
            now, FormatTimestamp(now), MissionEventType::MISSION_CANCELLED,
            "Mission Cancelled: " + reason,
            it->second.stage, MissionStatus::CANCELLED
        );
    }

    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        RecalculateStatisticsLocked();
    }

    return true;
}

bool MissionExecutionEngine::CompleteMission(const std::string& missionId) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end() || it->second.status != MissionStatus::ACTIVE) {
        return false;
    }

    uint64_t now = GetCurrentTimeMs();
    MissionRecord& m = it->second;
    m.status = MissionStatus::COMPLETED;
    m.stage = MissionStage::MISSION_COMPLETE;
    m.missionCompletionTimestamp = now;

    if (m.missionCreationTimestamp > 0) {
        m.elapsedTime = static_cast<double>(now - m.missionCreationTimestamp) / 60000.0;
    }
    if (m.dispatchTimestamp > 0) {
        m.actualEta = static_cast<double>(now - m.dispatchTimestamp) / 60000.0;
    }

    // Rule 10: Mission Completion Archived & Logged
    {
        std::lock_guard<std::mutex> timeLock(timelineMutex_);
        timelines_[missionId].emplace_back(
            now, FormatTimestamp(now), MissionEventType::MISSION_COMPLETED,
            "Mission Completed: Patient Handover and Resource Demobilization Finished",
            MissionStage::MISSION_COMPLETE, MissionStatus::COMPLETED
        );
    }

    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        RecalculateStatisticsLocked();
    }

    return true;
}

// =============================================================================
// Mission Assignment APIs
// =============================================================================

bool MissionExecutionEngine::AssignAmbulance(const std::string& missionId, const std::string& ambulanceId,
                                            const std::string& crewId, const std::string& vehicleCapability) {
    if (ambulanceId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end() || it->second.IsTerminal()) {
        return false;
    }

    it->second.ambulanceId = ambulanceId;
    if (!crewId.empty()) it->second.crewId = crewId;
    if (!vehicleCapability.empty()) it->second.vehicleCapability = vehicleCapability;

    // Advance stage if appropriate
    if (it->second.stage == MissionStage::CALL_RECEIVED) {
        it->second.stage = MissionStage::AMBULANCE_SELECTION;
    }

    uint64_t now = GetCurrentTimeMs();
    {
        std::lock_guard<std::mutex> timeLock(timelineMutex_);
        timelines_[missionId].emplace_back(
            now, FormatTimestamp(now), MissionEventType::AMBULANCE_ASSIGNED,
            "Ambulance Assigned: " + ambulanceId,
            it->second.stage, it->second.status
        );
    }

    // Update status to READY if all prerequisites met
    if (it->second.status == MissionStatus::NEW &&
        !it->second.ambulanceId.empty() && !it->second.hospitalId.empty()) {
        it->second.status = MissionStatus::READY;
    }

    return true;
}

bool MissionExecutionEngine::AssignHospital(const std::string& missionId, const std::string& hospitalId,
                                           double hospitalLat, double hospitalLon,
                                           const std::string& hospitalCapability) {
    if (hospitalId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end() || it->second.IsTerminal()) {
        return false;
    }

    it->second.hospitalId = hospitalId;
    it->second.hospitalLatitude = hospitalLat;
    it->second.hospitalLongitude = hospitalLon;
    if (!hospitalCapability.empty()) it->second.hospitalCapability = hospitalCapability;

    if (it->second.stage == MissionStage::AMBULANCE_SELECTION ||
        it->second.stage == MissionStage::CALL_RECEIVED) {
        it->second.stage = MissionStage::HOSPITAL_SELECTION;
    }

    uint64_t now = GetCurrentTimeMs();
    {
        std::lock_guard<std::mutex> timeLock(timelineMutex_);
        timelines_[missionId].emplace_back(
            now, FormatTimestamp(now), MissionEventType::HOSPITAL_ASSIGNED,
            "Hospital Assigned: " + hospitalId,
            it->second.stage, it->second.status
        );
    }

    if (it->second.status == MissionStatus::NEW &&
        !it->second.ambulanceId.empty() && !it->second.hospitalId.empty()) {
        it->second.status = MissionStatus::READY;
    }

    return true;
}

bool MissionExecutionEngine::AssignGreenCorridor(const std::string& missionId, const std::string& corridorId,
                                                uint32_t corridorVersion) {
    if (corridorId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end() || it->second.IsTerminal()) {
        return false;
    }

    it->second.greenCorridorId = corridorId;
    it->second.assignedCorridorVersion = corridorVersion;

    if (it->second.stage == MissionStage::ROUTE_PLANNING ||
        it->second.stage == MissionStage::HOSPITAL_SELECTION) {
        it->second.stage = MissionStage::GREEN_CORRIDOR_SETUP;
    }

    uint64_t now = GetCurrentTimeMs();
    {
        std::lock_guard<std::mutex> timeLock(timelineMutex_);
        timelines_[missionId].emplace_back(
            now, FormatTimestamp(now), MissionEventType::GREEN_CORRIDOR_STARTED,
            "Green Corridor Assigned: " + corridorId + " v" + std::to_string(corridorVersion),
            it->second.stage, it->second.status
        );
    }

    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        statistics_.totalGreenCorridors++;
    }

    return true;
}

bool MissionExecutionEngine::AssignRoute(const std::string& missionId, const std::string& routeId,
                                        double distanceKm, double etaMinutes) {
    if (routeId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end() || it->second.IsTerminal()) {
        return false;
    }

    it->second.currentRouteId = routeId;
    if (distanceKm > 0.0) {
        it->second.distance = distanceKm;
        it->second.totalRouteDistance = distanceKm;
    }
    if (etaMinutes > 0.0) {
        it->second.estimatedEta = etaMinutes;
        it->second.remainingEta = etaMinutes;
    }

    if (it->second.stage == MissionStage::HOSPITAL_SELECTION) {
        it->second.stage = MissionStage::ROUTE_PLANNING;
    }

    uint64_t now = GetCurrentTimeMs();
    {
        std::lock_guard<std::mutex> timeLock(timelineMutex_);
        timelines_[missionId].emplace_back(
            now, FormatTimestamp(now), MissionEventType::ROUTE_GENERATED,
            "Route Generated: " + routeId,
            it->second.stage, it->second.status
        );
    }

    return true;
}

bool MissionExecutionEngine::AssignTrafficPlan(const std::string& missionId, uint32_t planVersion) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end() || it->second.IsTerminal()) {
        return false;
    }

    it->second.trafficPlanVersion = planVersion;
    uint64_t now = GetCurrentTimeMs();
    {
        std::lock_guard<std::mutex> timeLock(timelineMutex_);
        timelines_[missionId].emplace_back(
            now, FormatTimestamp(now), MissionEventType::TRAFFIC_PLAN_GENERATED,
            "Traffic Signal Synchronization Plan Generated v" + std::to_string(planVersion),
            it->second.stage, it->second.status
        );
    }

    return true;
}

// =============================================================================
// Mission Updates APIs
// =============================================================================

bool MissionExecutionEngine::UpdateMissionStage(const std::string& missionId, MissionStage newStage) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end() || it->second.IsTerminal()) {
        return false;
    }

    if (!ValidateStageProgression(it->second.stage, newStage)) {
        return false;
    }

    it->second.stage = newStage;
    uint64_t now = GetCurrentTimeMs();

    MissionEventType evt = MissionEventType::ETA_UPDATED;
    std::string desc = "Stage updated to " + std::string(MissionStageToString(newStage));

    if (newStage == MissionStage::PATIENT_PICKUP) {
        it->second.pickupTimestamp = now;
        evt = MissionEventType::PATIENT_PICKED_UP;
        desc = "Ambulance on Scene: Patient Picked Up";
    } else if (newStage == MissionStage::ARRIVING_HOSPITAL) {
        it->second.hospitalArrivalTimestamp = now;
        evt = MissionEventType::AMBULANCE_ARRIVED;
        desc = "Ambulance Arrived at Hospital Triage Bay";
    } else if (newStage == MissionStage::HANDOVER) {
        evt = MissionEventType::PATIENT_HANDED_OVER;
        desc = "Patient Handover Commenced with Emergency Department";
    } else if (newStage == MissionStage::MISSION_COMPLETE) {
        it->second.status = MissionStatus::COMPLETED;
        it->second.missionCompletionTimestamp = now;
        evt = MissionEventType::MISSION_COMPLETED;
        desc = "Mission Complete: Clinical Handover Finished";
    }

    {
        std::lock_guard<std::mutex> timeLock(timelineMutex_);
        timelines_[missionId].emplace_back(now, FormatTimestamp(now), evt, desc, newStage, it->second.status);
    }

    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        RecalculateStatisticsLocked();
    }

    return true;
}

bool MissionExecutionEngine::UpdateMissionStatus(const std::string& missionId, MissionStatus newStatus,
                                                const std::string& note) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end()) {
        return false;
    }

    if (!ValidateTransition(it->second.status, newStatus)) {
        return false;
    }

    it->second.status = newStatus;
    if (!note.empty()) {
        it->second.missionNotes += " [" + note + "]";
    }

    uint64_t now = GetCurrentTimeMs();
    {
        std::lock_guard<std::mutex> timeLock(timelineMutex_);
        timelines_[missionId].emplace_back(
            now, FormatTimestamp(now), MissionEventType::ETA_UPDATED,
            "Status Transitioned to " + std::string(MissionStatusToString(newStatus)),
            it->second.stage, newStatus
        );
    }

    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        RecalculateStatisticsLocked();
    }

    return true;
}

bool MissionExecutionEngine::UpdateETA(const std::string& missionId, double remainingEtaMinutes) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end() || it->second.IsTerminal()) {
        return false;
    }

    it->second.remainingEta = remainingEtaMinutes;
    uint64_t now = GetCurrentTimeMs();
    {
        std::lock_guard<std::mutex> timeLock(timelineMutex_);
        timelines_[missionId].emplace_back(
            now, FormatTimestamp(now), MissionEventType::ETA_UPDATED,
            "ETA Updated: " + std::to_string(remainingEtaMinutes) + " min",
            it->second.stage, it->second.status
        );
    }

    return true;
}

bool MissionExecutionEngine::UpdateDistance(const std::string& missionId, double remainingDistanceKm) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end() || it->second.IsTerminal()) {
        return false;
    }

    it->second.distance = remainingDistanceKm;
    return true;
}

bool MissionExecutionEngine::UpdatePatientCondition(const std::string& missionId, uint32_t patientAcuity,
                                                   const std::string& conditionNotes) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end() || it->second.IsTerminal()) {
        return false;
    }

    it->second.patientAcuity = patientAcuity;
    if (!conditionNotes.empty()) {
        it->second.missionNotes += " [Patient: " + conditionNotes + "]";
    }
    return true;
}

bool MissionExecutionEngine::UpdateHospital(const std::string& missionId, const std::string& newHospitalId,
                                           const std::string& reason) {
    if (newHospitalId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end() || it->second.IsTerminal()) {
        return false;
    }

    it->second.hospitalId = newHospitalId;
    if (!reason.empty()) {
        it->second.missionNotes += " [Hospital Diverted: " + reason + "]";
    }

    uint64_t now = GetCurrentTimeMs();
    {
        std::lock_guard<std::mutex> timeLock(timelineMutex_);
        timelines_[missionId].emplace_back(
            now, FormatTimestamp(now), MissionEventType::HOSPITAL_UPDATED,
            "Hospital Diverted to " + newHospitalId + (reason.empty() ? "" : (" (" + reason + ")")),
            it->second.stage, it->second.status
        );
    }

    return true;
}

bool MissionExecutionEngine::UpdateAmbulance(const std::string& missionId, const std::string& newAmbulanceId,
                                            const std::string& reason) {
    if (newAmbulanceId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end() || it->second.IsTerminal()) {
        return false;
    }

    it->second.ambulanceId = newAmbulanceId;
    if (!reason.empty()) {
        it->second.missionNotes += " [Ambulance Reassigned: " + reason + "]";
    }

    uint64_t now = GetCurrentTimeMs();
    {
        std::lock_guard<std::mutex> timeLock(timelineMutex_);
        timelines_[missionId].emplace_back(
            now, FormatTimestamp(now), MissionEventType::AMBULANCE_ASSIGNED,
            "Ambulance Reassigned to " + newAmbulanceId + (reason.empty() ? "" : (" (" + reason + ")")),
            it->second.stage, it->second.status
        );
    }

    return true;
}

// =============================================================================
// Mission Queries APIs
// =============================================================================

bool MissionExecutionEngine::GetMission(const std::string& missionId, MissionRecord& outMission) const {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = missions_.find(missionId);
    if (it != missions_.end()) {
        outMission = it->second;
        return true;
    }
    // Check archive
    std::lock_guard<std::mutex> histLock(historyMutex_);
    auto ait = archivedMissions_.find(missionId);
    if (ait != archivedMissions_.end()) {
        outMission = ait->second;
        return true;
    }
    return false;
}

std::optional<MissionRecord> MissionExecutionEngine::GetMission(const std::string& missionId) const {
    MissionRecord out;
    if (GetMission(missionId, out)) {
        return out;
    }
    return std::nullopt;
}

bool MissionExecutionEngine::GetMissionByIncident(const std::string& incidentId, MissionRecord& outMission) const {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = incidentMap_.find(incidentId);
    if (it == incidentMap_.end()) {
        return false;
    }
    auto mit = missions_.find(it->second);
    if (mit != missions_.end()) {
        outMission = mit->second;
        return true;
    }
    return false;
}

bool MissionExecutionEngine::GetActiveMission(const std::string& missionId, MissionRecord& outMission) const {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = missions_.find(missionId);
    if (it != missions_.end() && it->second.IsActive()) {
        outMission = it->second;
        return true;
    }
    return false;
}

std::vector<MissionRecord> MissionExecutionEngine::GetAllActiveMissions() const {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    std::vector<MissionRecord> results;
    for (const auto& pair : missions_) {
        if (pair.second.IsActive()) {
            results.push_back(pair.second);
        }
    }
    return results;
}

std::vector<MissionRecord> MissionExecutionEngine::GetCompletedMissions() const {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    std::vector<MissionRecord> results;
    for (const auto& pair : missions_) {
        if (pair.second.status == MissionStatus::COMPLETED) {
            results.push_back(pair.second);
        }
    }
    std::lock_guard<std::mutex> histLock(historyMutex_);
    for (const auto& pair : archivedMissions_) {
        if (pair.second.status == MissionStatus::COMPLETED) {
            results.push_back(pair.second);
        }
    }
    return results;
}

std::vector<MissionRecord> MissionExecutionEngine::GetFailedMissions() const {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    std::vector<MissionRecord> results;
    for (const auto& pair : missions_) {
        if (pair.second.status == MissionStatus::FAILED ||
            pair.second.status == MissionStatus::ABORTED ||
            pair.second.status == MissionStatus::CANCELLED) {
            results.push_back(pair.second);
        }
    }
    return results;
}

std::vector<MissionRecord> MissionExecutionEngine::GetAllMissions() const {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    std::vector<MissionRecord> results;
    results.reserve(missions_.size());
    for (const auto& pair : missions_) {
        results.push_back(pair.second);
    }
    return results;
}

// =============================================================================
// Mission Reports & Timelines APIs
// =============================================================================

std::string MissionExecutionEngine::GenerateMissionReport(const std::string& missionId) const {
    MissionRecord m;
    if (!GetMission(missionId, m)) {
        return "[MISSION]\n\nMission ID:\nUNKNOWN\n";
    }

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "[MISSION]\n\n"
       << "Mission ID:\n" << m.missionId << "\n\n"
       << "Incident ID:\n" << m.incidentId << "\n\n"
       << "Priority:\n" << MissionPriorityToString(m.priority) << "\n\n"
       << "Status:\n" << MissionStatusToString(m.status) << "\n\n"
       << "Stage:\n" << MissionStageToString(m.stage) << "\n\n"
       << "Ambulance:\n" << (m.ambulanceId.empty() ? "UNASSIGNED" : m.ambulanceId) << "\n\n"
       << "Hospital:\n" << (m.hospitalId.empty() ? "UNASSIGNED" : m.hospitalId) << "\n\n"
       << "ETA:\n" << (m.remainingEta > 0.0 ? m.remainingEta : m.estimatedEta) << " min\n\n"
       << "Distance:\n" << m.distance << " km\n\n"
       << "Green Corridor:\n" << (m.greenCorridorId.empty() ? "NONE" : m.greenCorridorId) << "\n\n"
       << "Route Version:\n" << m.assignedCorridorVersion << "\n\n"
       << "Traffic Version:\n" << m.trafficPlanVersion << "\n\n"
       << "Elapsed Time:\n" << m.elapsedTime << " min\n\n"
       << "Completion Time:\n" << (m.missionCompletionTimestamp > 0 ? FormatTimestamp(m.missionCompletionTimestamp) : "IN_PROGRESS") << "\n";

    std::string report = ss.str();
    {
        std::lock_guard<std::mutex> repLock(reportsMutex_);
        const_cast<MissionExecutionEngine*>(this)->generatedReports_.push_back(report);
    }
    return report;
}

std::string MissionExecutionEngine::GenerateMissionTimeline(const std::string& missionId) const {
    std::lock_guard<std::mutex> timeLock(timelineMutex_);
    auto it = timelines_.find(missionId);
    if (it == timelines_.end() || it->second.empty()) {
        return "[TIMELINE]\nMission: " + missionId + " (No events recorded)\n";
    }

    std::ostringstream ss;
    ss << "==========================================================\n"
       << " MISSION TIMELINE: " << missionId << "\n"
       << "==========================================================\n";

    for (const auto& entry : it->second) {
        ss << entry.formattedTime << "\n"
           << entry.description << "\n\n";
    }
    return ss.str();
}

std::string MissionExecutionEngine::GenerateMissionSummary(const std::string& missionId) const {
    MissionRecord m;
    if (!GetMission(missionId, m)) {
        return "Mission " + missionId + " not found.";
    }

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "[" << m.missionId << "] Incident=" << m.incidentId
       << " Priority=" << MissionPriorityToString(m.priority)
       << " Status=" << MissionStatusToString(m.status)
       << " Stage=" << MissionStageToString(m.stage)
       << " Amb=" << m.ambulanceId << " Hosp=" << m.hospitalId
       << " ETA=" << m.estimatedEta << "min";
    return ss.str();
}

std::vector<MissionTimelineEntry> MissionExecutionEngine::GetTimeline(const std::string& missionId) const {
    std::lock_guard<std::mutex> timeLock(timelineMutex_);
    auto it = timelines_.find(missionId);
    if (it != timelines_.end()) {
        return it->second;
    }
    return {};
}

std::string MissionExecutionEngine::FormatStructuredLog(const std::string& missionId) const {
    MissionRecord m;
    if (!GetMission(missionId, m)) {
        return "[MISSION]\nMission:\nUNKNOWN\n";
    }

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "[MISSION]\n\n"
       << "Mission:\n" << m.missionId << "\n\n"
       << "Incident:\n" << m.incidentId << "\n\n"
       << "Priority:\n" << MissionPriorityToString(m.priority) << "\n\n"
       << "Ambulance:\n" << (m.ambulanceId.empty() ? "UNASSIGNED" : m.ambulanceId) << "\n\n"
       << "Hospital:\n" << (m.hospitalId.empty() ? "UNASSIGNED" : m.hospitalId) << "\n\n"
       << "Status:\n" << MissionStatusToString(m.status) << "\n\n"
       << "Stage:\n" << MissionStageToString(m.stage) << "\n\n"
       << "ETA:\n" << (m.remainingEta > 0.0 ? m.remainingEta : m.estimatedEta) << " min\n";
    return ss.str();
}

// =============================================================================
// Mission Statistics APIs
// =============================================================================

void MissionExecutionEngine::RecalculateStatisticsLocked() {
    statistics_.totalMissions = static_cast<uint32_t>(missions_.size() + archivedMissions_.size());
    statistics_.activeMissions = 0;
    statistics_.completedMissions = 0;
    statistics_.cancelledMissions = 0;
    statistics_.failedMissions = 0;

    double sumEta = 0.0;
    double sumResponse = 0.0;
    double sumPickup = 0.0;
    double sumMission = 0.0;
    double sumHospital = 0.0;
    double sumDist = 0.0;
    uint32_t etaCount = 0;
    uint32_t responseCount = 0;
    uint32_t pickupCount = 0;
    uint32_t missionCount = 0;
    uint32_t hospitalCount = 0;
    uint32_t distCount = 0;

    double maxDuration = 0.0;
    double minDuration = 1e9;

    auto processRecord = [&](const MissionRecord& m) {
        if (m.IsActive()) {
            statistics_.activeMissions++;
        }
        if (m.status == MissionStatus::COMPLETED) {
            statistics_.completedMissions++;
        } else if (m.status == MissionStatus::CANCELLED) {
            statistics_.cancelledMissions++;
        } else if (m.status == MissionStatus::FAILED || m.status == MissionStatus::ABORTED) {
            statistics_.failedMissions++;
        }

        if (m.estimatedEta > 0.0) {
            sumEta += m.estimatedEta;
            etaCount++;
        }
        if (m.totalRouteDistance > 0.0 || m.distance > 0.0) {
            sumDist += (m.totalRouteDistance > 0.0 ? m.totalRouteDistance : m.distance);
            distCount++;
        }

        if (m.dispatchTimestamp > m.missionCreationTimestamp && m.missionCreationTimestamp > 0) {
            double resp = static_cast<double>(m.dispatchTimestamp - m.missionCreationTimestamp) / 60000.0;
            sumResponse += resp;
            responseCount++;
        }

        if (m.pickupTimestamp > m.dispatchTimestamp && m.dispatchTimestamp > 0) {
            double pick = static_cast<double>(m.pickupTimestamp - m.dispatchTimestamp) / 60000.0;
            sumPickup += pick;
            pickupCount++;
        }

        if (m.hospitalArrivalTimestamp > m.pickupTimestamp && m.pickupTimestamp > 0) {
            double hosp = static_cast<double>(m.hospitalArrivalTimestamp - m.pickupTimestamp) / 60000.0;
            sumHospital += hosp;
            hospitalCount++;
        }

        if (m.status == MissionStatus::COMPLETED && m.missionCompletionTimestamp > m.missionCreationTimestamp) {
            double dur = static_cast<double>(m.missionCompletionTimestamp - m.missionCreationTimestamp) / 60000.0;
            sumMission += dur;
            missionCount++;
            if (dur > maxDuration) maxDuration = dur;
            if (dur < minDuration) minDuration = dur;
        }
    };

    for (const auto& pair : missions_) {
        processRecord(pair.second);
    }
    for (const auto& pair : archivedMissions_) {
        processRecord(pair.second);
    }

    statistics_.averageEta = (etaCount > 0) ? (sumEta / static_cast<double>(etaCount)) : 0.0;
    statistics_.averageResponseTime = (responseCount > 0) ? (sumResponse / static_cast<double>(responseCount)) : 0.0;
    statistics_.averagePickupTime = (pickupCount > 0) ? (sumPickup / static_cast<double>(pickupCount)) : 0.0;
    statistics_.averageHospitalArrivalTime = (hospitalCount > 0) ? (sumHospital / static_cast<double>(hospitalCount)) : 0.0;
    statistics_.averageMissionTime = (missionCount > 0) ? (sumMission / static_cast<double>(missionCount)) : 0.0;
    statistics_.longestMission = (missionCount > 0) ? maxDuration : 0.0;
    statistics_.shortestMission = (missionCount > 0 && minDuration < 1e8) ? minDuration : 0.0;
    statistics_.totalDistance = sumDist;
    statistics_.averageDistance = (distCount > 0) ? (sumDist / static_cast<double>(distCount)) : 0.0;

    uint32_t resolvedMissions = statistics_.completedMissions + statistics_.failedMissions + statistics_.cancelledMissions;
    if (resolvedMissions > 0) {
        statistics_.missionSuccessRate = (static_cast<double>(statistics_.completedMissions) / static_cast<double>(resolvedMissions)) * 100.0;
        statistics_.missionFailureRate = (static_cast<double>(statistics_.failedMissions) / static_cast<double>(resolvedMissions)) * 100.0;
    } else {
        statistics_.missionSuccessRate = 100.0;
        statistics_.missionFailureRate = 0.0;
    }
}

MissionStatistics MissionExecutionEngine::GetStatistics() const {
    std::lock_guard<std::mutex> statsLock(statisticsMutex_);
    return statistics_;
}

double MissionExecutionEngine::GetAverageResponseTime() const {
    std::lock_guard<std::mutex> statsLock(statisticsMutex_);
    return statistics_.averageResponseTime;
}

double MissionExecutionEngine::GetAverageMissionDuration() const {
    std::lock_guard<std::mutex> statsLock(statisticsMutex_);
    return statistics_.averageMissionTime;
}

double MissionExecutionEngine::GetAveragePickupTime() const {
    std::lock_guard<std::mutex> statsLock(statisticsMutex_);
    return statistics_.averagePickupTime;
}

double MissionExecutionEngine::GetAverageHospitalTime() const {
    std::lock_guard<std::mutex> statsLock(statisticsMutex_);
    return statistics_.averageHospitalArrivalTime;
}

// =============================================================================
// Mission Management APIs
// =============================================================================

bool MissionExecutionEngine::ArchiveMission(const std::string& missionId) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = missions_.find(missionId);
    if (it == missions_.end() || !it->second.IsTerminal()) {
        return false;
    }

    std::lock_guard<std::mutex> histLock(historyMutex_);
    archivedMissions_[missionId] = it->second;
    missions_.erase(it);
    return true;
}

bool MissionExecutionEngine::RestoreMission(const std::string& missionId) {
    std::lock_guard<std::mutex> histLock(historyMutex_);
    auto it = archivedMissions_.find(missionId);
    if (it == archivedMissions_.end()) {
        return false;
    }

    std::lock_guard<std::mutex> regLock(registryMutex_);
    missions_[missionId] = it->second;
    archivedMissions_.erase(it);
    return true;
}

bool MissionExecutionEngine::DeleteMission(const std::string& missionId) {
    bool found = false;
    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        auto it = missions_.find(missionId);
        if (it != missions_.end()) {
            if (!it->second.incidentId.empty()) {
                incidentMap_.erase(it->second.incidentId);
            }
            missions_.erase(it);
            found = true;
        }
    }
    {
        std::lock_guard<std::mutex> histLock(historyMutex_);
        auto ait = archivedMissions_.find(missionId);
        if (ait != archivedMissions_.end()) {
            archivedMissions_.erase(ait);
            found = true;
        }
    }
    {
        std::lock_guard<std::mutex> timeLock(timelineMutex_);
        timelines_.erase(missionId);
    }
    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        RecalculateStatisticsLocked();
    }
    return found;
}

void MissionExecutionEngine::ClearHistory() {
    {
        std::lock_guard<std::mutex> histLock(historyMutex_);
        archivedMissions_.clear();
    }
    {
        std::lock_guard<std::mutex> repLock(reportsMutex_);
        generatedReports_.clear();
    }
    {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        RecalculateStatisticsLocked();
    }
}

void MissionExecutionEngine::Reset() {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    std::lock_guard<std::mutex> statsLock(statisticsMutex_);
    std::lock_guard<std::mutex> timeLock(timelineMutex_);
    std::lock_guard<std::mutex> repLock(reportsMutex_);
    std::lock_guard<std::mutex> histLock(historyMutex_);
    std::lock_guard<std::mutex> analLock(analyticsMutex_);

    missions_.clear();
    incidentMap_.clear();
    timelines_.clear();
    archivedMissions_.clear();
    generatedReports_.clear();
    statistics_ = MissionStatistics{};
}

// =============================================================================
// Event Logging & Timeout Monitoring
// =============================================================================

void MissionExecutionEngine::LogEvent(const std::string& missionId, MissionEventType eventType,
                                     const std::string& description) {
    uint64_t now = GetCurrentTimeMs();
    std::lock_guard<std::mutex> timeLock(timelineMutex_);
    timelines_[missionId].emplace_back(
        now, FormatTimestamp(now), eventType, description,
        MissionStage::CALL_RECEIVED, MissionStatus::NEW
    );
}

bool MissionExecutionEngine::CheckTimeouts(uint64_t currentTimestampMs) {
    uint64_t now = (currentTimestampMs > 0) ? currentTimestampMs : GetCurrentTimeMs();
    bool anyTimedOut = false;

    std::lock_guard<std::mutex> regLock(registryMutex_);
    for (auto& pair : missions_) {
        MissionRecord& m = pair.second;
        if (m.IsActive() && m.missionCreationTimestamp > 0) {
            uint64_t duration = now - m.missionCreationTimestamp;
            if (duration > m.missionTimeout) {
                m.status = MissionStatus::FAILED;
                m.failureReason = "Rule 9: Mission Lifespan Timeout Exceeded";
                anyTimedOut = true;

                std::lock_guard<std::mutex> timeLock(timelineMutex_);
                timelines_[m.missionId].emplace_back(
                    now, FormatTimestamp(now), MissionEventType::MISSION_FAILED,
                    "Mission Failed: Lifespan Timeout Exceeded",
                    m.stage, MissionStatus::FAILED
                );
            }
        }
    }

    if (anyTimedOut) {
        std::lock_guard<std::mutex> statsLock(statisticsMutex_);
        RecalculateStatisticsLocked();
    }

    return anyTimedOut;
}

} // namespace mission
} // namespace aegis

