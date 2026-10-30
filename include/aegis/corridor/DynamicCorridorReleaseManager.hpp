/**
 * @file DynamicCorridorReleaseManager.hpp
 * @brief SMART AEGIS (AERS) Phase 3 - Dynamic Intersection-by-Intersection Corridor Release Manager.
 *
 * Implements deterministic dynamic corridor release: releases each intersection along an
 * emergency corridor immediately after an ambulance traverses it, restoring nominal traffic
 * without waiting for full corridor completion.
 *
 * Guarantees zero unnecessary intersection lockups, deterministic yellow/all-red clearance cycles,
 * and multi-vehicle priority handoff protection.
 *
 * Platform: Raspberry Pi 4 Model B | QNX SDP 8.0 (aarch64le) | Neutrino RTOS
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_CORRIDOR_DYNAMIC_CORRIDOR_RELEASE_MANAGER_HPP
#define AEGIS_CORRIDOR_DYNAMIC_CORRIDOR_RELEASE_MANAGER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
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
 * @brief Discrete operational state of an intersection under dynamic release supervision.
 */
enum class DynamicReleaseState {
    IDLE_NOMINAL,       ///< Normal cyclic traffic operation; no emergency lock
    LOCKED_EMERGENCY,   ///< Preempted green for active approaching emergency vehicle
    CLEARING_YELLOW,    ///< Vehicle departed; executing deterministic yellow transition
    ALL_RED_CLEARANCE,  ///< Post-emergency clearance buffer before restoring cyclic green
    RELEASED_NOMINAL    ///< Fully released; nominal cyclic traffic resumed
};

inline const char* DynamicReleaseStateToString(DynamicReleaseState state) noexcept {
    switch (state) {
        case DynamicReleaseState::IDLE_NOMINAL:      return "IDLE_NOMINAL";
        case DynamicReleaseState::LOCKED_EMERGENCY:  return "LOCKED_EMERGENCY";
        case DynamicReleaseState::CLEARING_YELLOW:   return "CLEARING_YELLOW";
        case DynamicReleaseState::ALL_RED_CLEARANCE: return "ALL_RED_CLEARANCE";
        case DynamicReleaseState::RELEASED_NOMINAL:   return "RELEASED_NOMINAL";
        default:                                     return "UNKNOWN";
    }
}

/**
 * @brief Configuration parameters for dynamic clearance and release timing.
 */
struct DynamicReleaseConfig {
    double yellowDurationSec{3.0};          ///< Deterministic yellow clearance duration
    double allRedClearanceSec{2.0};         ///< Deterministic all-red clearance duration
    double maxEmergencyHoldSec{90.0};       ///< Maximum safety hold threshold to prevent orphan locks
};

/**
 * @brief Tracking record of an individual intersection's preemption & release state.
 */
struct IntersectionReleaseRecord {
    std::string intersectionId{""};
    std::string holdingCorridorId{""};
    std::string holdingAmbulanceId{""};
    DynamicReleaseState state{DynamicReleaseState::IDLE_NOMINAL};

    uint64_t lockedTimestampUs{0};
    uint64_t departureTimestampUs{0};
    uint64_t yellowStartTimestampUs{0};
    uint64_t allRedStartTimestampUs{0};
    uint64_t releasedTimestampUs{0};

    double customYellowSec{3.0};
    double customAllRedSec{2.0};

    // Multi-vehicle overlapping reservations
    std::vector<std::string> queuedCorridorClaims{};
};

/**
 * @brief Structured audit event logged during intersection state transitions.
 */
struct DynamicReleaseEvent {
    std::string intersectionId{""};
    std::string corridorId{""};
    std::string ambulanceId{""};
    DynamicReleaseState fromState{DynamicReleaseState::IDLE_NOMINAL};
    DynamicReleaseState toState{DynamicReleaseState::IDLE_NOMINAL};
    uint64_t timestampUs{0};
    double transitionDurationMs{0.0};
    std::string actionSummary{""};

    std::string ToJson() const {
        std::ostringstream oss;
        oss << "{"
            << "\"intersectionId\":\"" << intersectionId << "\","
            << "\"corridorId\":\"" << corridorId << "\","
            << "\"ambulanceId\":\"" << ambulanceId << "\","
            << "\"fromState\":\"" << DynamicReleaseStateToString(fromState) << "\","
            << "\"toState\":\"" << DynamicReleaseStateToString(toState) << "\","
            << "\"timestampUs\":" << timestampUs << ","
            << "\"transitionDurationMs\":" << std::fixed << std::setprecision(2) << transitionDurationMs << ","
            << "\"actionSummary\":\"" << actionSummary << "\""
            << "}";
        return oss.str();
    }
};

/**
 * @class DynamicCorridorReleaseManager
 * @brief Deterministic manager releasing intersections sequentially following vehicle passage.
 */
class DynamicCorridorReleaseManager {
public:
    using SignalActionCallback = std::function<void(const std::string& intersectionId, const std::string& command)>;

    explicit DynamicCorridorReleaseManager(const DynamicReleaseConfig& config = DynamicReleaseConfig{});
    ~DynamicCorridorReleaseManager() = default;

    DynamicCorridorReleaseManager(const DynamicCorridorReleaseManager&) = delete;
    DynamicCorridorReleaseManager& operator=(const DynamicCorridorReleaseManager&) = delete;
    DynamicCorridorReleaseManager(DynamicCorridorReleaseManager&&) = delete;
    DynamicCorridorReleaseManager& operator=(DynamicCorridorReleaseManager&&) = delete;

    /**
     * @brief Locks an intersection for an approaching emergency vehicle.
     */
    bool LockIntersection(const std::string& corridorId,
                          const std::string& ambulanceId,
                          const std::string& intersectionId,
                          double yellowSec = 0.0,
                          double allRedSec = 0.0,
                          uint64_t timestampUs = 0);

    /**
     * @brief Notifies that an ambulance has traversed and cleared an intersection.
     * Initiates immediate deterministic clearance sequence without waiting for route end.
     */
    bool OnAmbulancePassed(const std::string& corridorId,
                           const std::string& intersectionId,
                           uint64_t departureTimestampUs = 0);

    /**
     * @brief Steps through active yellow/all-red clearance intervals deterministically.
     * @param currentTimestampUs QNX monotonic timestamp in microseconds.
     * @return Number of state transitions executed in this cycle.
     */
    size_t ProcessTransitions(uint64_t currentTimestampUs = 0);

    /**
     * @brief Force releases an intersection immediately into nominal cyclic mode.
     */
    bool ForceReleaseImmediate(const std::string& intersectionId, uint64_t timestampUs = 0);

    /**
     * @brief Installs physical hardware/signal controller actuation callback.
     */
    void SetSignalActionCallback(SignalActionCallback callback);

    /**
     * @brief Checks if an intersection is currently locked in emergency preemption.
     */
    bool IsIntersectionLocked(const std::string& intersectionId) const;

    /**
     * @brief Queries current dynamic release state of an intersection.
     */
    DynamicReleaseState GetIntersectionState(const std::string& intersectionId) const;

    /**
     * @brief Returns total number of currently locked or clearing intersections.
     */
    size_t GetActiveLockedCount() const;

    /**
     * @brief Retrieves detailed release record for an intersection.
     */
    bool GetIntersectionRecord(const std::string& intersectionId, IntersectionReleaseRecord& outRecord) const;

    /**
     * @brief Returns historical release audit events.
     */
    std::vector<DynamicReleaseEvent> GetReleaseEvents() const;

    /**
     * @brief Generates ASCII diagnostic status report.
     */
    std::string GenerateReleaseReport(uint64_t currentTimestampUs = 0) const;

    /**
     * @brief Resets and clears all internal tracking tables.
     */
    void ClearAll();

private:
    uint64_t GetCurrentTimeUs() const noexcept;

    mutable std::mutex m_mutex;
    DynamicReleaseConfig m_config;
    std::unordered_map<std::string, IntersectionReleaseRecord> m_intersections;
    std::vector<DynamicReleaseEvent> m_events;
    SignalActionCallback m_signalCallback{nullptr};
};

} // namespace corridor
} // namespace aegis

#endif // AEGIS_CORRIDOR_DYNAMIC_CORRIDOR_RELEASE_MANAGER_HPP
