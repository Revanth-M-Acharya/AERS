/**
 * @file signal_state_machine.hpp
 * @brief Deterministic finite state machine enforcing strict traffic signal phase transitions.
 */

#ifndef AEGIS_EDGE_SIGNAL_STATE_MACHINE_HPP
#define AEGIS_EDGE_SIGNAL_STATE_MACHINE_HPP

#include <cstdint>
#include <string>
#include <mutex>

namespace aegis {
namespace edge {
namespace traffic {

/**
 * @brief Formal signal states enforced by the traffic controller.
 */
enum class SignalState : uint8_t {
    RED = 0,
    YELLOW,
    GREEN,
    ALL_RED,
    FLASHING_YELLOW,
    FLASHING_RED,
    UNKNOWN
};

/**
 * @brief Converts SignalState to human-readable string representation.
 */
std::string SignalStateToString(SignalState state);

/**
 * @brief Converts string to SignalState.
 */
SignalState StringToSignalState(const std::string& str);

/**
 * @brief Deterministic state machine governing traffic signal cycles and emergency preemption.
 */
class SignalStateMachine {
public:
    explicit SignalStateMachine(SignalState initialState = SignalState::RED);
    ~SignalStateMachine() = default;

    SignalState GetCurrentState() const noexcept;
    std::string GetCurrentStateString() const;

    bool CanTransitionTo(SignalState target) const noexcept;
    bool TransitionTo(SignalState target);
    void Reset(SignalState state = SignalState::RED);

private:
    bool CanTransitionInternal(SignalState from, SignalState target) const noexcept;

    mutable std::mutex m_mutex;
    SignalState m_currentState{SignalState::RED};
};

} // namespace traffic
} // namespace edge
} // namespace aegis

#endif // AEGIS_EDGE_SIGNAL_STATE_MACHINE_HPP
