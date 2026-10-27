/**
 * @file IntersectionState.h
 * @brief Runtime signal and operational state tracker for roadside intersections.
 *
 * Maintains the current traffic signal phase, connectivity, and priority
 * metrics for all managed intersections across the SMART AEGIS network.
 */

#ifndef AEGIS_COORDINATOR_INTERSECTION_STATE_H
#define AEGIS_COORDINATOR_INTERSECTION_STATE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <cstdint>
#include <chrono>

namespace aegis {
namespace coordinator {

/**
 * @brief Discrete operational traffic signal states.
 */
enum class SignalState : uint8_t {
    RED    = 0, ///< Stop / Hold clearance
    YELLOW = 1, ///< Caution transition interval
    GREEN  = 2  ///< Proceed / Preempted green wave
};

/**
 * @brief Convert SignalState enum to human-readable string.
 */
const char* SignalStateToString(SignalState state) noexcept;

/**
 * @brief Convert string representation to SignalState enum.
 */
SignalState StringToSignalState(const std::string& str) noexcept;

/**
 * @brief Comprehensive runtime status record for a single intersection.
 */
struct IntersectionStatus {
    std::string intersectionId{""};          ///< Unique intersection ID (e.g. "INT_01")
    SignalState signalState{SignalState::RED};///< Current physical signal state (RED, YELLOW, GREEN)
    bool isOnline{false};                    ///< Node network connectivity state
    uint64_t lastHeartbeat{0};               ///< Unix epoch milliseconds of last liveness ping
    uint32_t currentPriority{0};             ///< Active corridor preemption priority level

    IntersectionStatus() = default;

    IntersectionStatus(std::string id,
                       SignalState state = SignalState::RED,
                       bool online = true,
                       uint64_t heartbeat = 0,
                       uint32_t priority = 0)
        : intersectionId(std::move(id))
        , signalState(state)
        , isOnline(online)
        , lastHeartbeat(heartbeat)
        , currentPriority(priority) {}
};

/**
 * @brief Thread-safe manager for intersection runtime signal states.
 */
class IntersectionState {
public:
    IntersectionState() = default;
    ~IntersectionState() = default;

    // Non-copyable, movable
    IntersectionState(const IntersectionState&) = delete;
    IntersectionState& operator=(const IntersectionState&) = delete;
    IntersectionState(IntersectionState&&) noexcept;
    IntersectionState& operator=(IntersectionState&&) noexcept;

    // =========================================================================
    // Intersection Registration & State Updates
    // =========================================================================

    /**
     * @brief Register a new intersection with default RED signal state.
     * @param intersectionId Unique identifier of the intersection node.
     * @param initialPriority Optional initial priority rank (default: 0).
     * @return true if registered; false if intersectionId is empty.
     */
    bool RegisterIntersection(const std::string& intersectionId, uint32_t initialPriority = 0);

    /**
     * @brief Update the signal state, heartbeat timestamp, and priority of an intersection.
     * @param intersectionId Target intersection ID.
     * @param state New traffic signal state.
     * @param heartbeat Timestamp in ms (0 = auto-set to current UTC clock).
     * @param priority Updated priority weighting.
     * @return true if intersection exists and was updated; false otherwise.
     */
    bool UpdateState(const std::string& intersectionId,
                     SignalState state,
                     uint64_t heartbeat = 0,
                     uint32_t priority = 0);

    /**
     * @brief Retrieve the current status of an intersection.
     * @param intersectionId Target intersection ID.
     * @param outStatus Populated with status record on success.
     * @return true if intersection found; false otherwise.
     */
    bool GetState(const std::string& intersectionId, IntersectionStatus& outStatus) const;

    /**
     * @brief Retrieve the current status of an intersection as optional.
     * @param intersectionId Target intersection ID.
     * @return std::optional with record if found, std::nullopt otherwise.
     */
    std::optional<IntersectionStatus> GetState(const std::string& intersectionId) const;

    /**
     * @brief Mark an intersection as online and update its heartbeat timestamp.
     * @param intersectionId Target intersection ID.
     * @param timestamp Milliseconds since epoch (0 = current time).
     * @return true if updated; false if intersection does not exist.
     */
    bool SetOnline(const std::string& intersectionId, uint64_t timestamp = 0);

    /**
     * @brief Mark an intersection as offline.
     * @param intersectionId Target intersection ID.
     * @return true if updated; false if intersection does not exist.
     */
    bool SetOffline(const std::string& intersectionId);

    // =========================================================================
    // Inspection & Queries
    // =========================================================================

    /**
     * @brief Check whether an intersection is registered.
     */
    bool HasIntersection(const std::string& intersectionId) const;

    /**
     * @brief Retrieve all registered intersection statuses.
     */
    std::vector<IntersectionStatus> GetAllStates() const;

    /**
     * @brief Total count of tracked intersections.
     */
    size_t GetTotalCount() const;

    /**
     * @brief Total count of intersections currently flagged as online.
     */
    size_t GetOnlineCount() const;

    /**
     * @brief Clear all tracked intersection records.
     */
    void Clear();

    /**
     * @brief Utility returning current system clock time in milliseconds.
     */
    static uint64_t CurrentTimestampMs() noexcept;

private:
    mutable std::mutex m_mutex{};
    std::unordered_map<std::string, IntersectionStatus> m_states{};
};

} // namespace coordinator
} // namespace aegis

#endif // AEGIS_COORDINATOR_INTERSECTION_STATE_H
