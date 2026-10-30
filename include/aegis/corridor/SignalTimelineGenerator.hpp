/**
 * @file SignalTimelineGenerator.hpp
 * @brief SMART AEGIS (AERS) Phase 3 - Deterministic Real-Time Signal Timeline Generator & Replay Engine.
 *
 * Computes deterministic schedules of traffic signal transitions along an emergency corridor:
 * - Pre-clearance yellow intervals
 * - All-red clearance holds
 * - Emergency green preemption windows
 * - Post-passage clearance yellow transitions
 * - Post-clearance all-red safety buffers
 * - Return to nominal cyclic operation
 *
 * Timestamps every event using the QNX monotonic clock (CLOCK_MONOTONIC).
 * Provides a replay engine for deterministic offline debugging, telemetry audits,
 * and timing drift / jitter analysis.
 *
 * Platform: Raspberry Pi 4 Model B | QNX SDP 8.0 (aarch64le) | Neutrino RTOS
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_CORRIDOR_SIGNAL_TIMELINE_GENERATOR_HPP
#define AEGIS_CORRIDOR_SIGNAL_TIMELINE_GENERATOR_HPP

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
 * @brief Categorization of signal transition phases along a corridor timeline.
 */
enum class SignalPhaseType {
    PRE_CLEARANCE_YELLOW,   ///< Warning non-emergency traffic before red transition
    ALL_RED_HOLD,           ///< All directions held in red for safe intersection clearance
    EMERGENCY_GREEN,        ///< Preempted green along emergency approach
    POST_YELLOW_TRANSITION, ///< Post-passage yellow change interval
    POST_ALL_RED_CLEARANCE, ///< Post-passage all-red safety interval
    RESTORED_NOMINAL        ///< Normal cyclic traffic signal plan resumed
};

inline const char* SignalPhaseTypeToString(SignalPhaseType phase) noexcept {
    switch (phase) {
        case SignalPhaseType::PRE_CLEARANCE_YELLOW:   return "PRE_CLEARANCE_YELLOW";
        case SignalPhaseType::ALL_RED_HOLD:           return "ALL_RED_HOLD";
        case SignalPhaseType::EMERGENCY_GREEN:        return "EMERGENCY_GREEN";
        case SignalPhaseType::POST_YELLOW_TRANSITION: return "POST_YELLOW_TRANSITION";
        case SignalPhaseType::POST_ALL_RED_CLEARANCE: return "POST_ALL_RED_CLEARANCE";
        case SignalPhaseType::RESTORED_NOMINAL:        return "RESTORED_NOMINAL";
        default:                                      return "UNKNOWN";
    }
}

/**
 * @brief Discrete scheduled signal event within a corridor timeline.
 */
struct SignalTimelineEvent {
    uint64_t eventId{0};
    std::string intersectionId{""};
    SignalPhaseType phase{SignalPhaseType::EMERGENCY_GREEN};
    std::string commandString{""};          ///< E.g. "ROAD_A_GREEN", "ALL_RED", "YELLOW"

    uint64_t scheduledStartUs{0};           ///< Monotonic QNX clock time for start
    uint64_t durationUs{0};                 ///< Scheduled phase duration in microseconds
    uint64_t scheduledEndUs{0};             ///< Monotonic QNX clock time for completion

    uint64_t actualExecutionStartUs{0};     ///< Recorded time during execution or replay
    uint64_t actualExecutionEndUs{0};
    bool isExecuted{false};
    int64_t driftUs{0};                     ///< Jitter / drift: (actualExecutionStartUs - scheduledStartUs)

    std::string ToJson() const {
        std::ostringstream oss;
        oss << "{"
            << "\"eventId\":" << eventId << ","
            << "\"intersectionId\":\"" << intersectionId << "\","
            << "\"phase\":\"" << SignalPhaseTypeToString(phase) << "\","
            << "\"command\":\"" << commandString << "\","
            << "\"scheduledStartUs\":" << scheduledStartUs << ","
            << "\"durationUs\":" << durationUs << ","
            << "\"scheduledEndUs\":" << scheduledEndUs << ","
            << "\"isExecuted\":" << (isExecuted ? "true" : "false") << ","
            << "\"driftUs\":" << driftUs
            << "}";
        return oss.str();
    }
};

/**
 * @brief Complete time-coordinated corridor schedule encompassing all intersections.
 */
struct CorridorTimelineSchedule {
    std::string scheduleId{""};
    std::string corridorId{""};
    std::string ambulanceId{""};
    uint64_t baseTimestampUs{0};            ///< QNX clock reference point
    double totalScheduleDurationSec{0.0};
    std::vector<SignalTimelineEvent> events{};
    bool isValid{false};
};

/**
 * @class SignalTimelineGenerator
 * @brief Generates, verifies, and replays deterministic signal timing schedules.
 */
class SignalTimelineGenerator {
public:
    using ReplayStepCallback = std::function<void(const SignalTimelineEvent&)>;

    SignalTimelineGenerator() = default;
    ~SignalTimelineGenerator() = default;

    SignalTimelineGenerator(const SignalTimelineGenerator&) = delete;
    SignalTimelineGenerator& operator=(const SignalTimelineGenerator&) = delete;
    SignalTimelineGenerator(SignalTimelineGenerator&&) = delete;
    SignalTimelineGenerator& operator=(SignalTimelineGenerator&&) = delete;

    /**
     * @brief Computes a complete deterministic timeline schedule for a corridor route.
     * @param corridorId Unique corridor identifier.
     * @param ambulanceId Assigned emergency vehicle.
     * @param intersections Ordered sequence of intersection nodes.
     * @param etaOffsetsSec Arrival offset for each intersection from base timestamp.
     * @param greenDurationSec Active green window duration (default 10.0s).
     * @param yellowDurationSec Yellow warning duration (default 3.0s).
     * @param allRedDurationSec All-red clearance duration (default 2.0s).
     * @param baseTimeUs Reference QNX monotonic timestamp (0 queries clock).
     * @return Generated schedule structure.
     */
    CorridorTimelineSchedule GenerateTimeline(const std::string& corridorId,
                                              const std::string& ambulanceId,
                                              const std::vector<std::string>& intersections,
                                              const std::vector<double>& etaOffsetsSec,
                                              double greenDurationSec = 10.0,
                                              double yellowDurationSec = 3.0,
                                              double allRedDurationSec = 2.0,
                                              uint64_t baseTimeUs = 0);

    /**
     * @brief Records actual execution timing for an event to verify drift.
     */
    bool RecordExecution(const std::string& scheduleId,
                         uint64_t eventId,
                         uint64_t executionTimeUs);

    /**
     * @brief Validates timeline against safety rules (non-negative gaps, valid durations, clearance).
     */
    bool ValidateTimeline(const CorridorTimelineSchedule& schedule, std::string& failureReason) const;

    /**
     * @brief Replays all scheduled events sequentially through a visitor callback.
     */
    bool ReplayTimeline(const std::string& scheduleId, ReplayStepCallback onStep);

    /**
     * @brief Retrieves stored schedule by ID.
     */
    bool GetSchedule(const std::string& scheduleId, CorridorTimelineSchedule& outSchedule) const;

    /**
     * @brief Exports schedule and executed events to formatted JSON string.
     */
    std::string ExportToJson(const std::string& scheduleId) const;

    /**
     * @brief Exports schedule to diagnostic ASCII timeline table.
     */
    std::string ExportToAsciiTable(const std::string& scheduleId) const;

    /**
     * @brief Clears all stored schedules.
     */
    void ClearAll();

private:
    uint64_t GetCurrentTimeUs() const noexcept;

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, CorridorTimelineSchedule> m_schedules;
    uint64_t m_nextEventId{1};
};

} // namespace corridor
} // namespace aegis

#endif // AEGIS_CORRIDOR_SIGNAL_TIMELINE_GENERATOR_HPP
