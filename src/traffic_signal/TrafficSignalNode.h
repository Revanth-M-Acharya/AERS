/**
 * @file TrafficSignalNode.h
 * @brief Software simulation of an intersection Traffic Signal Node for AERS (Phase 6A).
 *
 * Enforces a strict internal traffic signal state machine (RED, YELLOW, GREEN,
 * FLASHING_RED, FLASHING_YELLOW, ALL_RED), handles coordinator commands, rejects
 * invalid transitions, ignores duplicate state changes, and records structured
 * transition audit logs. Pure software simulation with zero GPIO/hardware dependencies.
 */

#ifndef AEGIS_TRAFFIC_SIGNAL_NODE_H
#define AEGIS_TRAFFIC_SIGNAL_NODE_H

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>
#include <functional>

namespace aegis {
namespace traffic_signal {

/**
 * @brief Supported discrete operational states of the traffic signal.
 */
enum class SignalState : uint8_t {
    RED = 0,
    YELLOW,
    GREEN,
    FLASHING_RED,
    FLASHING_YELLOW,
    ALL_RED
};

/**
 * @brief Convert SignalState enum to string representation.
 */
std::string SignalStateToString(SignalState state);

/**
 * @brief Parse string to SignalState enum.
 */
SignalState StringToSignalState(const std::string& stateStr);

/**
 * @brief Try parsing string to SignalState enum.
 * @return true if valid state string, false otherwise.
 */
bool TryStringToSignalState(const std::string& stateStr, SignalState& outState);

/**
 * @brief Record representing a logged successful signal state transition.
 */
struct SignalTransitionRecord {
    uint64_t timestampMs{0};
    std::string timestampStr{""};
    std::string intersectionId{""};
    SignalState previousState{SignalState::RED};
    SignalState newState{SignalState::RED};
    std::string reason{""};
    std::string formattedLog{""};
};

/**
 * @brief Software simulation of an intersection traffic signal node.
 */
class TrafficSignalNode {
public:
    using TransitionCallback = std::function<void(const SignalTransitionRecord&)>;

    /**
     * @brief Construct a TrafficSignalNode with specified intersection ID and initial state.
     * @param intersectionId Identifying string (e.g. "INT_A", "INT_B").
     * @param initialState Starting signal state (default: SignalState::RED).
     */
    explicit TrafficSignalNode(std::string intersectionId = "INT_A",
                              SignalState initialState = SignalState::RED);

    ~TrafficSignalNode() = default;

    // Non-copyable, movable
    TrafficSignalNode(const TrafficSignalNode&) = delete;
    TrafficSignalNode& operator=(const TrafficSignalNode&) = delete;
    TrafficSignalNode(TrafficSignalNode&&) noexcept;
    TrafficSignalNode& operator=(TrafficSignalNode&&) noexcept;

    // =========================================================================
    // Public APIs (Requirement 7)
    // =========================================================================

    /**
     * @brief Retrieve the current signal state.
     */
    SignalState GetCurrentState() const;

    /**
     * @brief Attempt to transition the signal state machine to a new state.
     *
     * - Rejects invalid transitions according to safety rules.
     * - Ignores duplicate state changes (returns false, emits no log).
     * - Emits structured log on every successful transition.
     *
     * @param newState Target SignalState.
     * @param reason Description/cause of transition (default: "Direct SetSignalState").
     * @return true if state changed successfully; false if invalid or duplicate.
     */
    bool SetSignalState(SignalState newState, const std::string& reason = "Direct SetSignalState");
    bool SetSignalState(const std::string& newStateStr, const std::string& reason = "Direct SetSignalState");

    /**
     * @brief Process a command received from the Central Coordinator.
     *
     * Supported commands:
     * - "SET_RED"
     * - "SET_YELLOW"
     * - "SET_GREEN"
     * - "SET_ALL_RED"
     * (Also supports "SET_FLASHING_RED", "SET_FLASHING_YELLOW")
     *
     * @param command Raw command string or JSON string from coordinator.
     * @param reason Optional reason description for the transition.
     * @return true if command was valid and resulted in a successful state change.
     */
    bool HandleCoordinatorCommand(const std::string& command, const std::string& reason = "");

    // =========================================================================
    // Inspection & Management APIs
    // =========================================================================

    /**
     * @brief Get current signal state as string.
     */
    std::string GetCurrentStateString() const;

    /**
     * @brief Get assigned intersection identifier.
     */
    const std::string& GetIntersectionId() const;

    /**
     * @brief Set assigned intersection identifier.
     */
    void SetIntersectionId(const std::string& id);

    /**
     * @brief Query whether a transition from current state to target state is permissible.
     */
    bool CanTransitionTo(SignalState target) const;

    /**
     * @brief Retrieve history of all successful transition records.
     */
    std::vector<SignalTransitionRecord> GetTransitionHistory() const;

    /**
     * @brief Retrieve count of logged transitions.
     */
    size_t GetTransitionCount() const;

    /**
     * @brief Retrieve the formatted log string of the most recent transition.
     */
    std::string GetLastTransitionLog() const;

    /**
     * @brief Clear historical transition records.
     */
    void ClearTransitionHistory();

    /**
     * @brief Reset controller state machine directly (useful in test harnesses).
     */
    void Reset(SignalState state = SignalState::RED);

    /**
     * @brief Register optional listener invoked on every successful transition.
     */
    void SetTransitionCallback(TransitionCallback callback);

private:
    bool CanTransitionInternal(SignalState from, SignalState target) const noexcept;
    static std::string CurrentTimestampString();
    static uint64_t CurrentTimestampMs();
    static std::string FormatTransitionLog(const std::string& timestampStr,
                                           const std::string& intersectionId,
                                           SignalState previousState,
                                           SignalState newState,
                                           const std::string& reason);

    mutable std::mutex m_mutex{};
    std::string m_intersectionId{"INT_A"};
    SignalState m_currentState{SignalState::RED};
    std::vector<SignalTransitionRecord> m_transitionHistory{};
    TransitionCallback m_callback{nullptr};
};

} // namespace traffic_signal

namespace intersection {
    // Convenient alias in intersection namespace for unified interoperability
    using TrafficSignalNode = aegis::traffic_signal::TrafficSignalNode;
} // namespace intersection

} // namespace aegis

#endif // AEGIS_TRAFFIC_SIGNAL_NODE_H
