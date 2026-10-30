/**
 * @file CorridorDeadlineMonitor.hpp
 * @brief SMART AEGIS (AERS) Phase 3 - Real-Time Emergency Corridor Deadline Monitor.
 *
 * Deterministically supervises active emergency ambulance corridors against strict
 * real-time deadlines. Tracks corridor creation timestamps, estimated arrival times,
 * elapsed execution intervals, and remaining deadline margins using the QNX monotonic clock.
 *
 * Detects corridor deadline timeouts and automatically triggers autonomous recovery
 * callbacks and structured audit logging without human intervention.
 *
 * Platform: Raspberry Pi 4 Model B | QNX SDP 8.0 (aarch64le) | Neutrino RTOS
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_CORRIDOR_DEADLINE_MONITOR_HPP
#define AEGIS_CORRIDOR_DEADLINE_MONITOR_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <cstdint>
#include <memory>
#include <sstream>
#include <iomanip>

#include "aegis/common/time_utils.hpp"
#include "aegis/common/qnx_compat.hpp"

namespace aegis {
namespace corridor {

/**
 * @brief Discrete operational status of a monitored emergency corridor.
 */
enum class CorridorDeadlineStatus {
    ACTIVE,                 ///< Corridor active and operating within nominal deadline
    APPROACHING_DEADLINE,   ///< Remaining time is below warning threshold (e.g. <= 20% or 10s)
    TIMED_OUT,              ///< Deadline has expired before completion
    COMPLETED,              ///< Ambulance safely traversed corridor to destination
    RECOVERED,              ///< Autonomous recovery successfully resolved the deadline expiration
    ABORTED                 ///< Operator or system aborted the corridor
};

inline const char* CorridorDeadlineStatusToString(CorridorDeadlineStatus status) noexcept {
    switch (status) {
        case CorridorDeadlineStatus::ACTIVE:               return "ACTIVE";
        case CorridorDeadlineStatus::APPROACHING_DEADLINE: return "APPROACHING_DEADLINE";
        case CorridorDeadlineStatus::TIMED_OUT:            return "TIMED_OUT";
        case CorridorDeadlineStatus::COMPLETED:            return "COMPLETED";
        case CorridorDeadlineStatus::RECOVERED:            return "RECOVERED";
        case CorridorDeadlineStatus::ABORTED:              return "ABORTED";
        default:                                           return "UNKNOWN";
    }
}

/**
 * @brief Configuration parameters for corridor deadline supervision.
 */
struct CorridorDeadlineConfig {
    double defaultDeadlineSec{60.0};        ///< Default corridor completion deadline in seconds
    double slackFactor{1.25};               ///< Multiplier applied to ETA to establish default deadline (1.25 = +25%)
    double warningThresholdSec{10.0};       ///< Seconds before deadline to trigger APPROACHING_DEADLINE
    double warningThresholdRatio{0.20};     ///< Ratio of deadline remaining to trigger warning
    bool autoTriggerRecovery{true};         ///< Automatically invoke recovery handler upon timeout
};

/**
 * @brief Structured record of an active or finalized corridor deadline tracking instance.
 */
struct CorridorTrackingRecord {
    std::string corridorId{""};
    std::string ambulanceId{""};
    std::string startIntersectionId{""};
    std::string destinationId{""};
    std::vector<std::string> routeIntersections{};

    uint64_t creationTimestampUs{0};        ///< QNX monotonic timestamp at corridor establishment
    double estimatedArrivalDurationSec{0.0};///< Estimated traversal time from route planner
    double deadlineDurationSec{0.0};        ///< Hard real-time deadline duration in seconds
    double elapsedTimeSec{0.0};             ///< Total elapsed time since creation
    double remainingTimeSec{0.0};           ///< Remaining deadline margin (negative if overdue)

    std::string currentIntersection{""};
    CorridorDeadlineStatus status{CorridorDeadlineStatus::ACTIVE};
    bool isRecovered{false};
    std::string recoveryAction{""};
    uint64_t completionTimestampUs{0};
};

/**
 * @brief Structured telemetry event emitted upon corridor deadline overrun.
 */
struct CorridorTimeoutEvent {
    std::string corridorId{""};
    std::string ambulanceId{""};
    uint64_t timestampUs{0};
    double elapsedSec{0.0};
    double deadlineSec{0.0};
    double overrunSec{0.0};
    std::string lastKnownIntersection{""};
    std::string actionTaken{""};

    std::string ToJson() const {
        std::ostringstream oss;
        oss << "{"
            << "\"corridorId\":\"" << corridorId << "\","
            << "\"ambulanceId\":\"" << ambulanceId << "\","
            << "\"timestampUs\":" << timestampUs << ","
            << "\"elapsedSec\":" << std::fixed << std::setprecision(3) << elapsedSec << ","
            << "\"deadlineSec\":" << std::fixed << std::setprecision(3) << deadlineSec << ","
            << "\"overrunSec\":" << std::fixed << std::setprecision(3) << overrunSec << ","
            << "\"lastKnownIntersection\":\"" << lastKnownIntersection << "\","
            << "\"actionTaken\":\"" << actionTaken << "\""
            << "}";
        return oss.str();
    }
};

/**
 * @class CorridorDeadlineMonitor
 * @brief Thread-safe, deterministic supervisor for emergency corridor real-time deadlines.
 */
class CorridorDeadlineMonitor {
public:
    using TimeoutRecoveryHandler = std::function<void(const CorridorTimeoutEvent&)>;

    explicit CorridorDeadlineMonitor(const CorridorDeadlineConfig& config = CorridorDeadlineConfig{});
    ~CorridorDeadlineMonitor() = default;

    // Non-copyable, non-movable due to internal mutex
    CorridorDeadlineMonitor(const CorridorDeadlineMonitor&) = delete;
    CorridorDeadlineMonitor& operator=(const CorridorDeadlineMonitor&) = delete;
    CorridorDeadlineMonitor(CorridorDeadlineMonitor&&) = delete;
    CorridorDeadlineMonitor& operator=(CorridorDeadlineMonitor&&) = delete;

    /**
     * @brief Registers and arms a new emergency corridor for deadline monitoring.
     * @param corridorId Unique identifier of the corridor.
     * @param ambulanceId Assigned ambulance.
     * @param route List of intersection IDs along the corridor path.
     * @param etaSec Estimated time of arrival in seconds.
     * @param customDeadlineSec If > 0, overrides the computed deadline.
     * @param creationTimeUs Optional custom timestamp; 0 defaults to QNX monotonic clock.
     * @return True if registered successfully, false if duplicate or invalid.
     */
    bool RegisterCorridor(const std::string& corridorId,
                          const std::string& ambulanceId,
                          const std::vector<std::string>& route,
                          double etaSec,
                          double customDeadlineSec = 0.0,
                          uint64_t creationTimeUs = 0);

    /**
     * @brief Updates the real-time spatial progress of an ambulance along its corridor.
     */
    bool UpdateProgress(const std::string& corridorId,
                        const std::string& currentIntersectionId,
                        uint64_t timestampUs = 0);

    /**
     * @brief Marks a corridor as successfully completed (ambulance arrived at destination).
     */
    bool CompleteCorridor(const std::string& corridorId, uint64_t timestampUs = 0);

    /**
     * @brief Aborts an active corridor.
     */
    bool AbortCorridor(const std::string& corridorId,
                       const std::string& reason,
                       uint64_t timestampUs = 0);

    /**
     * @brief Executes a deterministic deadline evaluation pass over all active corridors.
     * @param currentTimestampUs QNX monotonic time in microseconds (0 queries clock).
     * @return Number of corridors that timed out during this check pass.
     */
    size_t CheckDeadlines(uint64_t currentTimestampUs = 0);

    /**
     * @brief Installs a custom recovery handler invoked automatically when a corridor times out.
     */
    void SetTimeoutRecoveryHandler(TimeoutRecoveryHandler handler);

    /**
     * @brief Queries whether a specific corridor has timed out.
     */
    bool IsCorridorTimedOut(const std::string& corridorId) const;

    /**
     * @brief Returns the remaining deadline margin in seconds for a corridor.
     */
    double GetRemainingDeadlineSec(const std::string& corridorId, uint64_t currentTimestampUs = 0) const;

    /**
     * @brief Retrieves a copy of the tracking record for a corridor.
     */
    bool GetCorridorInfo(const std::string& corridorId, CorridorTrackingRecord& outRecord) const;

    /**
     * @brief Returns a snapshot of all tracked corridors.
     */
    std::vector<CorridorTrackingRecord> GetAllCorridors() const;

    /**
     * @brief Returns the historical log of generated timeout events.
     */
    std::vector<CorridorTimeoutEvent> GetTimeoutEvents() const;

    /**
     * @brief Returns count of currently active (non-completed/non-aborted) corridors.
     */
    size_t GetActiveCorridorCount() const;

    /**
     * @brief Generates an ASCII diagnostic status table of all corridors.
     */
    std::string GenerateStatusReport(uint64_t currentTimestampUs = 0) const;

    /**
     * @brief Resets and clears all internal corridor states and event logs.
     */
    void ClearAll();

private:
    uint64_t GetCurrentTimeUs() const noexcept;

    mutable std::mutex m_mutex;
    CorridorDeadlineConfig m_config;
    std::unordered_map<std::string, CorridorTrackingRecord> m_corridors;
    std::vector<CorridorTimeoutEvent> m_timeoutEvents;
    TimeoutRecoveryHandler m_recoveryHandler{nullptr};
};

} // namespace corridor
} // namespace aegis

#endif // AEGIS_CORRIDOR_DEADLINE_MONITOR_HPP
