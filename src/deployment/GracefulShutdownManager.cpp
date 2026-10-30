/**
 * @file GracefulShutdownManager.cpp
 * @brief SMART AEGIS (AERS) Phase 16D - Graceful Shutdown Manager Implementation.
 */

#include "GracefulShutdownManager.h"
#include <iostream>

namespace aegis {
namespace deployment {

GracefulShutdownManager::GracefulShutdownManager() = default;

void GracefulShutdownManager::RegisterCleanupHook(CleanupHook hook) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (hook) {
        m_hooks.push_back(std::move(hook));
    }
}

bool GracefulShutdownManager::ExecuteShutdown(bool isCoordinator) {
    (void)isCoordinator;

    // Phase 1: Clear traffic signals
    m_currentPhase = ShutdownPhase::CLEARING_TRAFFIC;

    // Phase 2: Notify peer nodes
    m_currentPhase = ShutdownPhase::NOTIFYING_PEERS;

    // Phase 3: Release GPIO reservations
    m_currentPhase = ShutdownPhase::RELEASING_GPIO;

    // Phase 4: Close network connections
    m_currentPhase = ShutdownPhase::CLOSING_NETWORK;

    // Phase 5: Flush logs & execute hooks
    m_currentPhase = ShutdownPhase::FLUSHING_LOGS;
    {
        std::vector<CleanupHook> hooksCopy;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            hooksCopy = m_hooks;
        }
        for (auto& hook : hooksCopy) {
            try {
                if (hook) hook();
            } catch (...) {
                // Suppress exceptions during shutdown
            }
        }
    }

    // Phase 6: Terminated
    m_currentPhase = ShutdownPhase::TERMINATED;
    return true;
}

} // namespace deployment
} // namespace aegis
