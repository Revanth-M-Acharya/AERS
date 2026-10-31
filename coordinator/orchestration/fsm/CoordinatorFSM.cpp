/**
 * @file CoordinatorFSM.cpp
 * @brief Deterministic Finite State Machine transitions implementation.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/orchestration/fsm/CoordinatorFSM.hpp"
#include "common/utils/TimeUtils.hpp"

namespace aegis::coordinator::orchestration {

CoordinatorFSM::CoordinatorFSM(std::shared_ptr<CoordinatorStateManager> stateManager,
                               std::shared_ptr<IEventBus> eventBus)
    : stateManager_(std::move(stateManager)), eventBus_(std::move(eventBus)) {
    if (stateManager_) {
        stateManager_->SetSystemState(CoordinatorStateToString(currentState_));
    }
}

bool CoordinatorFSM::IsLegalTransition(CoordinatorState from, CoordinatorState to) {
    if (from == to) return true; // Idempotent

    switch (from) {
        case CoordinatorState::IDLE:
            return (to == CoordinatorState::REGISTERING ||
                    to == CoordinatorState::ROUTE_PLANNING);

        case CoordinatorState::REGISTERING:
            return (to == CoordinatorState::IDLE ||
                    to == CoordinatorState::ROUTE_PLANNING);

        case CoordinatorState::ROUTE_PLANNING:
            return (to == CoordinatorState::PLAN_GENERATION ||
                    to == CoordinatorState::FAILED);

        case CoordinatorState::PLAN_GENERATION:
            return (to == CoordinatorState::PLAN_VALIDATION ||
                    to == CoordinatorState::FAILED);

        case CoordinatorState::PLAN_VALIDATION:
            return (to == CoordinatorState::DISPATCHING ||
                    to == CoordinatorState::REPLANNING ||
                    to == CoordinatorState::FAILED);

        case CoordinatorState::DISPATCHING:
            return (to == CoordinatorState::EXECUTING ||
                    to == CoordinatorState::REPLANNING ||
                    to == CoordinatorState::FAILED);

        case CoordinatorState::EXECUTING:
            return (to == CoordinatorState::MONITORING ||
                    to == CoordinatorState::REPLANNING ||
                    to == CoordinatorState::FAILED);

        case CoordinatorState::MONITORING:
            return (to == CoordinatorState::EXECUTING ||
                    to == CoordinatorState::REPLANNING ||
                    to == CoordinatorState::COMPLETED ||
                    to == CoordinatorState::FAILED);

        case CoordinatorState::REPLANNING:
            return (to == CoordinatorState::ROUTE_PLANNING ||
                    to == CoordinatorState::PLAN_GENERATION ||
                    to == CoordinatorState::FAILED);

        case CoordinatorState::COMPLETED:
            return (to == CoordinatorState::IDLE);

        case CoordinatorState::FAILED:
            return (to == CoordinatorState::IDLE);

        default:
            return false;
    }
}

bool CoordinatorFSM::CanTransitionTo(CoordinatorState targetState) const {
    std::lock_guard<std::mutex> lock(fsmMutex_);
    return IsLegalTransition(currentState_, targetState);
}

bool CoordinatorFSM::TransitionTo(CoordinatorState targetState, const std::string& reason) {
    CoordinatorState oldState;
    StateChangeCallback cbCopy;
    {
        std::lock_guard<std::mutex> lock(fsmMutex_);
        if (!IsLegalTransition(currentState_, targetState)) {
            return false;
        }

        oldState = currentState_;
        currentState_ = targetState;
        cbCopy = callback_;
    }

    if (stateManager_) {
        stateManager_->SetSystemState(CoordinatorStateToString(targetState));
    }

    if (cbCopy) {
        cbCopy(oldState, targetState, reason);
    }

    return true;
}

CoordinatorState CoordinatorFSM::GetCurrentState() const {
    std::lock_guard<std::mutex> lock(fsmMutex_);
    return currentState_;
}

std::string CoordinatorFSM::GetCurrentStateName() const {
    std::lock_guard<std::mutex> lock(fsmMutex_);
    return CoordinatorStateToString(currentState_);
}

void CoordinatorFSM::SetStateChangeCallback(StateChangeCallback cb) {
    std::lock_guard<std::mutex> lock(fsmMutex_);
    callback_ = std::move(cb);
}

void CoordinatorFSM::Reset() {
    std::lock_guard<std::mutex> lock(fsmMutex_);
    currentState_ = CoordinatorState::IDLE;
    if (stateManager_) {
        stateManager_->SetSystemState("IDLE");
    }
}

} // namespace aegis::coordinator::orchestration

