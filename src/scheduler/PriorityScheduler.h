/**
 * @file PriorityScheduler.h
 * @brief Multi-Ambulance Priority Scheduling and Conflict Resolution Engine.
 */

#ifndef AEGIS_SCHEDULER_PRIORITY_SCHEDULER_H
#define AEGIS_SCHEDULER_PRIORITY_SCHEDULER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

#include "ambulance/AmbulanceProfile.h"
#include "scheduler/ConflictDetector.h"
#include "medical/PriorityEngine.h"

namespace aegis {
namespace scheduler {

// =============================================================================
// Phase 8A: Data Models, Comparators, and Formatting Helpers
// =============================================================================

/**
 * @brief Patient clinical severity priority levels.
 */
enum class PatientSeverity {
    CRITICAL = 1,
    HIGH = 2,
    MEDIUM = 3,
    LOW = 4
};

inline std::string PatientSeverityToString(PatientSeverity severity) {
    switch (severity) {
        case PatientSeverity::CRITICAL: return "CRITICAL";
        case PatientSeverity::HIGH:     return "HIGH";
        case PatientSeverity::MEDIUM:   return "MEDIUM";
        case PatientSeverity::LOW:      return "LOW";
        default:                        return "UNKNOWN";
    }
}

inline PatientSeverity StringToPatientSeverity(const std::string& str) {
    if (str == "CRITICAL" || str == "critical") return PatientSeverity::CRITICAL;
    if (str == "HIGH" || str == "high") return PatientSeverity::HIGH;
    if (str == "MEDIUM" || str == "medium") return PatientSeverity::MEDIUM;
    if (str == "LOW" || str == "low") return PatientSeverity::LOW;
    return PatientSeverity::LOW;
}

inline int SeverityToRank(PatientSeverity severity) {
    switch (severity) {
        case PatientSeverity::CRITICAL: return 1;
        case PatientSeverity::HIGH:     return 2;
        case PatientSeverity::MEDIUM:   return 3;
        case PatientSeverity::LOW:      return 4;
        default:                        return 5;
    }
}

/**
 * @brief Helper to format ETA seconds matching specification ("38 sec").
 */
inline std::string FormatEtaSeconds(double eta) {
    if (std::abs(eta - std::round(eta)) < 1e-4) {
        return std::to_string(static_cast<long long>(std::round(eta))) + " sec";
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << eta << " sec";
    return oss.str();
}

/**
 * @brief Comprehensive Emergency Request data model for Phase 8A Intelligent Scheduling.
 */
struct EmergencyRequest {
    std::string requestId{""};
    std::string ambulanceId{""};
    PatientSeverity severity{PatientSeverity::LOW};
    PatientSeverity patientSeverity{PatientSeverity::LOW};
    int rank{4};
    std::string hospitalId{""};
    std::string currentLocation{""};
    std::string destination{""};
    double estimatedArrivalTime{0.0};
    double eta{0.0};
    double distance{0.0};
    double distanceMeters{0.0};
    uint64_t timestamp{0};
    uint64_t timestampMs{0};

    EmergencyRequest() = default;

    EmergencyRequest(std::string reqId,
                     std::string ambId,
                     PatientSeverity sev,
                     std::string hospId,
                     std::string currLoc,
                     std::string dest,
                     double etaVal,
                     double distVal,
                     uint64_t ts)
        : requestId(std::move(reqId))
        , ambulanceId(std::move(ambId))
        , severity(sev)
        , patientSeverity(sev)
        , rank(SeverityToRank(sev))
        , hospitalId(std::move(hospId))
        , currentLocation(std::move(currLoc))
        , destination(std::move(dest))
        , estimatedArrivalTime(etaVal)
        , eta(etaVal)
        , distance(distVal)
        , distanceMeters(distVal)
        , timestamp(ts)
        , timestampMs(ts) {}

    PatientSeverity GetSeverity() const noexcept {
        return severity;
    }

    double GetEta() const noexcept {
        return estimatedArrivalTime;
    }

    double GetDistance() const noexcept {
        return distance;
    }

    uint64_t GetTimestamp() const noexcept {
        return timestamp;
    }

    bool operator==(const EmergencyRequest& other) const noexcept {
        return requestId == other.requestId;
    }
};

/**
 * @brief Deterministic 5-tier multi-attribute scheduling comparator.
 *
 * Rules:
 * 1. Priority order: CRITICAL > HIGH > MEDIUM > LOW
 * 2. If same severity: Lower ETA wins
 * 3. If ETA equal: Shorter distance wins
 * 4. If distance equal: Earlier timestamp wins
 * 5. If timestamp equal: Lexicographically smaller Ambulance ID wins
 */
struct EmergencyRequestComparator {
    bool operator()(const EmergencyRequest& a, const EmergencyRequest& b) const noexcept {
        // Rule 1: Patient Priority Level
        if (a.rank != b.rank) {
            return a.rank < b.rank; // Lower rank number = higher priority
        }

        // Rule 2: If same severity, choose lower ETA
        if (a.estimatedArrivalTime != b.estimatedArrivalTime) {
            return a.estimatedArrivalTime < b.estimatedArrivalTime;
        }

        // Rule 3: If ETA equal, choose shorter distance
        if (a.distance != b.distance) {
            return a.distance < b.distance;
        }

        // Rule 4: If distance equal, choose earlier request timestamp
        if (a.timestamp != b.timestamp) {
            return a.timestamp < b.timestamp;
        }

        // Rule 5: If timestamp equal, choose lexicographically smaller Ambulance ID
        if (a.ambulanceId != b.ambulanceId) {
            return a.ambulanceId < b.ambulanceId;
        }

        // Deterministic fallback for duplicate ambulance IDs: Request ID
        return a.requestId < b.requestId;
    }
};

/**
 * @brief Adaptable result wrapper for GetNextEmergency() allowing both std::optional and direct object semantics.
 */
class NextEmergencyResult {
public:
    NextEmergencyResult(std::optional<EmergencyRequest> opt) : m_opt(std::move(opt)) {}

    bool has_value() const noexcept { return m_opt.has_value(); }
    explicit operator bool() const noexcept { return m_opt.has_value(); }

    const EmergencyRequest& value() const { return m_opt.value(); }
    EmergencyRequest& value() { return m_opt.value(); }

    const EmergencyRequest* operator->() const { return &m_opt.value(); }
    EmergencyRequest* operator->() { return &m_opt.value(); }

    const EmergencyRequest& operator*() const { return m_opt.value(); }
    EmergencyRequest& operator*() { return m_opt.value(); }

    operator std::optional<EmergencyRequest>() const { return m_opt; }
    operator EmergencyRequest() const { return m_opt.value_or(EmergencyRequest{}); }

private:
    std::optional<EmergencyRequest> m_opt;
};

/**
 * @brief Output result of resolving a junction contention between two or more ambulances.
 */
struct ConflictResolution {
    std::string intersectionId{""};
    std::string winnerAmbulanceId{""};
    std::vector<std::string> delayedAmbulanceIds{};
    uint32_t delayDurationSeconds{15};                                 ///< Clearance window for winner
    std::unordered_map<std::string, uint32_t> assignedDelaysSeconds{}; ///< Delay assigned to each delayed unit
    std::string resolutionReason{""};
    uint64_t timestampEpochMs{0};

    bool HasDelayedAmbulance(const std::string& ambId) const {
        for (const auto& delayed : delayedAmbulanceIds) {
            if (delayed == ambId) return true;
        }
        return false;
    }
};

/**
 * @brief Real-time scheduling engine that arbitrates junction contention and prioritizes emergency corridors.
 */
class PriorityScheduler {
public:
    static constexpr uint32_t DEFAULT_CLEARANCE_SECONDS = 15;

    PriorityScheduler();
    explicit PriorityScheduler(uint32_t conflictWindowSeconds,
                               uint32_t clearanceDurationSeconds = DEFAULT_CLEARANCE_SECONDS);
    ~PriorityScheduler() = default;

    // Configuration
    void SetConflictWindowSeconds(uint32_t seconds) noexcept;
    uint32_t GetConflictWindowSeconds() const noexcept;

    void SetClearanceDurationSeconds(uint32_t seconds) noexcept;
    uint32_t GetClearanceDurationSeconds() const noexcept;

    // Arbitration & Resolution
    ConflictResolution ResolveConflict(const IntersectionConflict& conflict);
    std::vector<ConflictResolution> ScheduleAmbulances(const std::vector<ambulance::AmbulanceProfile>& ambulances);

    // Green Corridor Grants & Safety Guarantees
    bool IsAmbulanceGrantedGreen(const std::string& ambulanceId, const std::string& intersectionId) const;
    std::string GetGrantedAmbulanceForIntersection(const std::string& intersectionId) const;
    void RevokeGreenGrant(const std::string& intersectionId);
    void ForceGreenGrant(const std::string& intersectionId, const std::string& ambulanceId);
    
    // Inspection & Queries
    std::vector<ConflictResolution> GetActiveResolutions() const;
    std::optional<ConflictResolution> GetResolutionForIntersection(const std::string& intersectionId) const;
    void Clear();

    // =========================================================================
    // Phase 8A: Intelligent Priority Scheduling Engine Public APIs
    // =========================================================================

    /**
     * @brief Submits a new or updated emergency request to the scheduler.
     * @param request Emergency request containing ambulance, severity, ETA, distance, and timestamp.
     * @return true if accepted, false if request ID or ambulance ID is empty.
     */
    bool SubmitEmergencyRequest(const EmergencyRequest& request);

    /**
     * @brief Removes an emergency request by Request ID or Ambulance ID.
     * @param requestId Request ID or Ambulance ID to cancel.
     * @return true if request was found and removed, false otherwise.
     */
    bool RemoveRequest(const std::string& requestId);

    /**
     * @brief Retrieves the next highest-priority emergency request.
     * @param pop If true, removes the returned request from the queue.
     * @return Result object convertible to std::optional<EmergencyRequest> or EmergencyRequest.
     */
    NextEmergencyResult GetNextEmergency(bool pop = false);

    /**
     * @brief Retrieves the next highest-priority emergency request via output parameter.
     * @param outRequest Assigned the top emergency request if available.
     * @param pop If true, removes the returned request from the queue.
     * @return true if an emergency was available, false if queue is empty.
     */
    bool GetNextEmergency(EmergencyRequest& outRequest, bool pop = false);

    /**
     * @brief Computes deterministic multi-factor priority schedule across all requests.
     * Prints structured [SCHEDULER] log if logOutput is enabled.
     * @param logOutput Whether to print structured audit log to stdout (default: true).
     * @return Ordered priority queue of EmergencyRequests.
     */
    const std::vector<EmergencyRequest>& ScheduleRequests(bool logOutput = true);

    /**
     * @brief Returns a snapshot of the current queue in sorted priority order.
     */
    std::vector<EmergencyRequest> GetCurrentQueue() const;

    /**
     * @brief Clears all pending emergency requests from the queue.
     */
    void ClearQueue();

    /**
     * @brief Formats the current scheduled queue into the specified output string representation.
     */
    std::string FormatQueue() const;

    /**
     * @brief Formats an arbitrary queue of requests into the specified output string representation.
     */
    std::string FormatQueue(const std::vector<EmergencyRequest>& queue) const;

    /**
     * @brief Returns formatted [SCHEDULER] audit log string for current queue.
     */
    std::string FormatSchedulerLog() const;

    /**
     * @brief Enables or disables automatic console logging during ScheduleRequests.
     */
    void SetConsoleLogging(bool enabled) noexcept;
    bool GetConsoleLogging() const noexcept;

    /**
     * @brief Returns the total number of queued emergency requests.
     */
    size_t GetQueueSize() const;

private:
    static int PriorityToRank(medical::PriorityLevel priority);
    static bool CompareAmbulanceUrgency(const AmbulanceArrival& a, const AmbulanceArrival& b);

    mutable std::mutex m_mutex;
    ConflictDetector m_detector;
    uint32_t m_clearanceDurationSeconds{DEFAULT_CLEARANCE_SECONDS};
    std::unordered_map<std::string, std::string> m_activeGreenGrants{};       ///< intersectionId -> winnerAmbulanceId
    std::unordered_map<std::string, ConflictResolution> m_lastResolutions{}; ///< intersectionId -> ConflictResolution

    // Phase 8A internal state
    std::vector<EmergencyRequest> m_requestsQueue{};
    bool m_enableConsoleLog{true};
};

} // namespace scheduler
} // namespace aegis

#endif // AEGIS_SCHEDULER_PRIORITY_SCHEDULER_H
