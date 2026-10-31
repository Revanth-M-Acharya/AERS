/**
 * @file CoordinatorFSM.hpp
 * @brief Deterministic Finite State Machine for Coordinator Orchestration.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/orchestration/state/CoordinatorStateManager.hpp"
#include "coordinator/orchestration/event_bus/IEventBus.hpp"
#include <string>
#include <mutex>
#include <functional>
#include <memory>

namespace aegis::coordinator::orchestration {

enum class CoordinatorState {
    IDLE,
    REGISTERING,
    ROUTE_PLANNING,
    PLAN_GENERATION,
    PLAN_VALIDATION,
    DISPATCHING,
    EXECUTING,
    MONITORING,
    REPLANNING,
    COMPLETED,
    FAILED
};

inline std::string CoordinatorStateToString(CoordinatorState state) {
    switch (state) {
        case CoordinatorState::IDLE:            return "IDLE";
        case CoordinatorState::REGISTERING:     return "REGISTERING";
        case CoordinatorState::ROUTE_PLANNING:   return "ROUTE_PLANNING";
        case CoordinatorState::PLAN_GENERATION: return "PLAN_GENERATION";
        case CoordinatorState::PLAN_VALIDATION: return "PLAN_VALIDATION";
        case CoordinatorState::DISPATCHING:     return "DISPATCHING";
        case CoordinatorState::EXECUTING:       return "EXECUTING";
        case CoordinatorState::MONITORING:      return "MONITORING";
        case CoordinatorState::REPLANNING:      return "REPLANNING";
        case CoordinatorState::COMPLETED:       return "COMPLETED";
        case CoordinatorState::FAILED:          return "FAILED";
        default:                                return "UNKNOWN";
    }
}

using StateChangeCallback = std::function<void(CoordinatorState from, CoordinatorState to, const std::string& reason)>;

class CoordinatorFSM {
public:
    explicit CoordinatorFSM(std::shared_ptr<CoordinatorStateManager> stateManager = nullptr,
                            std::shared_ptr<IEventBus> eventBus = nullptr);
    ~CoordinatorFSM() = default;

    /**
     * @brief Attempt state transition.
     * @return true if transition is legally permitted and executed; false if illegal.
     */
    bool TransitionTo(CoordinatorState targetState, const std::string& reason = "");

    /**
     * @brief Check whether a transition from current state to target state is legally valid.
     */
    bool CanTransitionTo(CoordinatorState targetState) const;

    CoordinatorState GetCurrentState() const;
    std::string GetCurrentStateName() const;

    /**
     * @brief Register callback for state transition events.
     */
    void SetStateChangeCallback(StateChangeCallback cb);

    /**
     * @brief Reset FSM back to IDLE state.
     */
    void Reset();

private:
    static bool IsLegalTransition(CoordinatorState from, CoordinatorState to);

    mutable std::mutex fsmMutex_;
    CoordinatorState currentState_{CoordinatorState::IDLE};
    std::shared_ptr<CoordinatorStateManager> stateManager_;
    std::shared_ptr<IEventBus> eventBus_;
    StateChangeCallback callback_;
};

} // namespace aegis::coordinator::orchestration

