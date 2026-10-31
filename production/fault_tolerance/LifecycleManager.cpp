/**
 * @file LifecycleManager.cpp
 * @brief Implementation of lifecycle management and shutdown sequencing.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "production/fault_tolerance/LifecycleManager.hpp"

namespace aegis::production {

LifecycleManager::LifecycleManager() = default;

void LifecycleManager::RegisterShutdownHook(const std::string& name, ShutdownHook hook) {
    std::lock_guard<std::mutex> lock(mutex_);
    shutdownHooks_.emplace_back(name, std::move(hook));
}

bool LifecycleManager::Start() {
    SystemLifecycleState expected = SystemLifecycleState::UNINITIALIZED;
    if (state_.compare_exchange_strong(expected, SystemLifecycleState::INITIALIZING)) {
        // Initialization sequencing
        state_.store(SystemLifecycleState::OPERATIONAL);
        return true;
    }
    return false;
}

void LifecycleManager::GracefulShutdown() {
    SystemLifecycleState current = state_.load();
    if (current == SystemLifecycleState::STOPPED) {
        return;
    }

    state_.store(SystemLifecycleState::DRAINING);

    std::vector<std::pair<std::string, ShutdownHook>> hooksCopy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        hooksCopy = shutdownHooks_;
    }

    // Execute shutdown hooks in reverse registration order (LIFO)
    for (auto it = hooksCopy.rbegin(); it != hooksCopy.rend(); ++it) {
        if (it->second) {
            try {
                it->second();
            } catch (...) {
                // Best effort cleanup: never propagate in shutdown
            }
        }
    }

    state_.store(SystemLifecycleState::STOPPED);
}

void LifecycleManager::TriggerFailSafe(const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    failsafeReason_ = reason;
    state_.store(SystemLifecycleState::FAILSAFE);
}

SystemLifecycleState LifecycleManager::GetState() const {
    return state_.load();
}

bool LifecycleManager::IsRunning() const {
    return state_.load() == SystemLifecycleState::OPERATIONAL;
}

const std::string& LifecycleManager::GetFailSafeReason() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return failsafeReason_;
}

} // namespace aegis::production

