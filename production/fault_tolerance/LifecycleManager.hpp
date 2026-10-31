/**
 * @file LifecycleManager.hpp
 * @brief Graceful shutdown, startup sequencing, and lifecycle management for QNX RTOS.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>

namespace aegis::production {

enum class SystemLifecycleState {
    UNINITIALIZED,
    INITIALIZING,
    OPERATIONAL,
    DRAINING,
    STOPPED,
    FAILSAFE
};

inline std::string SystemLifecycleStateToString(SystemLifecycleState state) {
    switch (state) {
        case SystemLifecycleState::UNINITIALIZED: return "UNINITIALIZED";
        case SystemLifecycleState::INITIALIZING:  return "INITIALIZING";
        case SystemLifecycleState::OPERATIONAL:   return "OPERATIONAL";
        case SystemLifecycleState::DRAINING:      return "DRAINING";
        case SystemLifecycleState::STOPPED:       return "STOPPED";
        case SystemLifecycleState::FAILSAFE:      return "FAILSAFE";
        default:                                  return "UNKNOWN";
    }
}

using ShutdownHook = std::function<void()>;

class LifecycleManager {
public:
    LifecycleManager();
    ~LifecycleManager() = default;

    /**
     * @brief Register a cleanup action to execute on shutdown in reverse registration order.
     */
    void RegisterShutdownHook(const std::string& name, ShutdownHook hook);

    /**
     * @brief Transition system state from UNINITIALIZED -> INITIALIZING -> OPERATIONAL.
     */
    bool Start();

    /**
     * @brief Transition system state from OPERATIONAL -> DRAINING -> STOPPED, running all hooks.
     */
    void GracefulShutdown();

    /**
     * @brief Trigger emergency fail-safe transition.
     */
    void TriggerFailSafe(const std::string& reason);

    SystemLifecycleState GetState() const;
    bool IsRunning() const;
    const std::string& GetFailSafeReason() const;

private:
    mutable std::mutex mutex_;
    std::atomic<SystemLifecycleState> state_{SystemLifecycleState::UNINITIALIZED};
    std::vector<std::pair<std::string, ShutdownHook>> shutdownHooks_;
    std::string failsafeReason_;
};

} // namespace aegis::production

