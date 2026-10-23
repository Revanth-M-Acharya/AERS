/**
 * @file state_machine.hpp
 * @brief Generic thread-safe state machine base class with state transition hooks.
 */

#ifndef AEGIS_COMMON_STATE_MACHINE_HPP
#define AEGIS_COMMON_STATE_MACHINE_HPP

#include <mutex>
#include <functional>

namespace aegis {
namespace common {

template <typename StateEnum>
class StateMachine {
public:
    explicit StateMachine(StateEnum initialState) : m_currentState(initialState) {}
    virtual ~StateMachine() = default;

    StateEnum GetCurrentState() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_currentState;
    }

    bool TransitionTo(StateEnum nextState) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!CanTransition(m_currentState, nextState)) {
            return false;
        }
        StateEnum oldState = m_currentState;
        m_currentState = nextState;
        OnStateTransition(oldState, nextState);
        return true;
    }

protected:
    virtual bool CanTransition(StateEnum /*current*/, StateEnum /*next*/) const {
        return true;
    }

    virtual void OnStateTransition(StateEnum /*oldState*/, StateEnum /*newState*/) {}

    StateEnum m_currentState;
    mutable std::mutex m_mutex;
};

} // namespace common
} // namespace aegis

#endif // AEGIS_COMMON_STATE_MACHINE_HPP
