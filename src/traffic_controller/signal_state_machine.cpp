/**
 * @file signal_state_machine.cpp
 * @brief Implementation of SignalStateMachine enforcing deterministic signal transitions.
 */

#include "aegis/edge/signal_state_machine.hpp"
#include "aegis/logger/logger.hpp"

namespace aegis {
namespace edge {
namespace traffic {

std::string SignalStateToString(SignalState state) {
    switch (state) {
        case SignalState::RED: return "RED";
        case SignalState::YELLOW: return "YELLOW";
        case SignalState::GREEN: return "GREEN";
        case SignalState::ALL_RED: return "ALL_RED";
        case SignalState::FLASHING_YELLOW: return "FLASHING_YELLOW";
        case SignalState::FLASHING_RED: return "FLASHING_RED";
        case SignalState::UNKNOWN:
        default: return "UNKNOWN";
    }
}

SignalState StringToSignalState(const std::string& str) {
    if (str == "RED") return SignalState::RED;
    if (str == "YELLOW") return SignalState::YELLOW;
    if (str == "GREEN") return SignalState::GREEN;
    if (str == "ALL_RED") return SignalState::ALL_RED;
    if (str == "FLASHING_YELLOW") return SignalState::FLASHING_YELLOW;
    if (str == "FLASHING_RED") return SignalState::FLASHING_RED;
    return SignalState::UNKNOWN;
}

SignalStateMachine::SignalStateMachine(SignalState initialState)
    : m_currentState(initialState) {}

SignalState SignalStateMachine::GetCurrentState() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentState;
}

std::string SignalStateMachine::GetCurrentStateString() const {
    return SignalStateToString(GetCurrentState());
}

bool SignalStateMachine::CanTransitionInternal(SignalState from, SignalState target) const noexcept {
    if (from == target) {
        return true;
    }

    switch (from) {
        case SignalState::RED:
            // Normal: RED -> GREEN. Emergency: RED -> ALL_RED. FailSafe: FLASHING_*
            return (target == SignalState::GREEN ||
                    target == SignalState::ALL_RED ||
                    target == SignalState::FLASHING_YELLOW ||
                    target == SignalState::FLASHING_RED);

        case SignalState::GREEN:
            // Normal: GREEN -> YELLOW. Emergency: GREEN -> ALL_RED. FailSafe: FLASHING_*
            // NEVER jump directly from GREEN to RED (requires yellow clearance)
            return (target == SignalState::YELLOW ||
                    target == SignalState::ALL_RED ||
                    target == SignalState::FLASHING_YELLOW ||
                    target == SignalState::FLASHING_RED);

        case SignalState::YELLOW:
            // Normal: YELLOW -> RED. Emergency: YELLOW -> ALL_RED. FailSafe: FLASHING_*
            // NEVER jump from YELLOW back to GREEN
            return (target == SignalState::RED ||
                    target == SignalState::ALL_RED ||
                    target == SignalState::FLASHING_YELLOW ||
                    target == SignalState::FLASHING_RED);

        case SignalState::ALL_RED:
            // Emergency entry: ALL_RED -> GREEN. Restore: ALL_RED -> RED or YELLOW.
            return (target == SignalState::GREEN ||
                    target == SignalState::RED ||
                    target == SignalState::YELLOW ||
                    target == SignalState::FLASHING_YELLOW ||
                    target == SignalState::FLASHING_RED);

        case SignalState::FLASHING_YELLOW:
        case SignalState::FLASHING_RED:
        case SignalState::UNKNOWN:
            // From degraded/unknown states, can only enter ALL_RED or RED safely
            return (target == SignalState::ALL_RED ||
                    target == SignalState::RED ||
                    target == SignalState::FLASHING_YELLOW ||
                    target == SignalState::FLASHING_RED);

        default:
            return false;
    }
}

bool SignalStateMachine::CanTransitionTo(SignalState target) const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return CanTransitionInternal(m_currentState, target);
}

bool SignalStateMachine::TransitionTo(SignalState target) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_currentState == target) {
        return true;
    }

    if (!CanTransitionInternal(m_currentState, target)) {
        logger::Logger::Instance().Log(
            logger::LogLevel::WARN,
            "SIGNAL_FSM",
            "Invalid transition rejected: " + SignalStateToString(m_currentState) +
            " -> " + SignalStateToString(target));
        return false;
    }

    SignalState previous = m_currentState;
    m_currentState = target;

    logger::Logger::Instance().Log(
        logger::LogLevel::INFO,
        "SIGNAL_FSM",
        SignalStateToString(previous) + " -> " + SignalStateToString(target));

    return true;
}

void SignalStateMachine::Reset(SignalState state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    SignalState previous = m_currentState;
    m_currentState = state;
    logger::Logger::Instance().Log(
        logger::LogLevel::INFO,
        "SIGNAL_FSM",
        "State machine reset: " + SignalStateToString(previous) + " -> " + SignalStateToString(state));
}

} // namespace traffic
} // namespace edge
} // namespace aegis
