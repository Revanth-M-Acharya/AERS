/**
 * @file GracefulShutdownManager.h
 * @brief SMART AEGIS (AERS) Phase 16D - Graceful Shutdown Manager.
 *
 * Coordinates deterministic, multi-phase safe termination:
 * CLEARING_TRAFFIC -> NOTIFYING_PEERS -> RELEASING_GPIO -> CLOSING_NETWORK -> FLUSHING_LOGS -> TERMINATED.
 */

#ifndef AEGIS_DEPLOYMENT_GRACEFUL_SHUTDOWN_MANAGER_H
#define AEGIS_DEPLOYMENT_GRACEFUL_SHUTDOWN_MANAGER_H

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>

namespace aegis {
namespace deployment {

enum class ShutdownPhase {
    NOT_SHUTTING_DOWN = 0,
    CLEARING_TRAFFIC,
    NOTIFYING_PEERS,
    RELEASING_GPIO,
    CLOSING_NETWORK,
    FLUSHING_LOGS,
    TERMINATED
};

inline std::string ShutdownPhaseToString(ShutdownPhase phase) {
    switch (phase) {
        case ShutdownPhase::NOT_SHUTTING_DOWN: return "NOT_SHUTTING_DOWN";
        case ShutdownPhase::CLEARING_TRAFFIC:  return "CLEARING_TRAFFIC";
        case ShutdownPhase::NOTIFYING_PEERS:   return "NOTIFYING_PEERS";
        case ShutdownPhase::RELEASING_GPIO:    return "RELEASING_GPIO";
        case ShutdownPhase::CLOSING_NETWORK:   return "CLOSING_NETWORK";
        case ShutdownPhase::FLUSHING_LOGS:     return "FLUSHING_LOGS";
        case ShutdownPhase::TERMINATED:        return "TERMINATED";
        default:                               return "UNKNOWN";
    }
}

class GracefulShutdownManager {
public:
    using CleanupHook = std::function<void()>;

    GracefulShutdownManager();
    ~GracefulShutdownManager() = default;

    // Non-copyable, non-movable
    GracefulShutdownManager(const GracefulShutdownManager&) = delete;
    GracefulShutdownManager& operator=(const GracefulShutdownManager&) = delete;

    /**
     * @brief Register a cleanup hook to execute during shutdown.
     */
    void RegisterCleanupHook(CleanupHook hook);

    /**
     * @brief Execute full deterministic graceful shutdown sequence.
     * @param isCoordinator Whether this node is the Central Coordinator.
     * @return true if shutdown sequence finished with all phases completed.
     */
    bool ExecuteShutdown(bool isCoordinator);

    /**
     * @brief Query current shutdown phase.
     */
    ShutdownPhase GetCurrentPhase() const noexcept { return m_currentPhase; }

    /**
     * @brief Query whether node has finished termination.
     */
    bool IsTerminated() const noexcept { return m_currentPhase == ShutdownPhase::TERMINATED; }

    /**
     * @brief Check if shutdown is in progress.
     */
    bool IsShuttingDown() const noexcept {
        return m_currentPhase != ShutdownPhase::NOT_SHUTTING_DOWN &&
               m_currentPhase != ShutdownPhase::TERMINATED;
    }

private:
    mutable std::mutex m_mutex;
    std::atomic<ShutdownPhase> m_currentPhase{ShutdownPhase::NOT_SHUTTING_DOWN};
    std::vector<CleanupHook> m_hooks{};
};

} // namespace deployment
} // namespace aegis

#endif // AEGIS_DEPLOYMENT_GRACEFUL_SHUTDOWN_MANAGER_H
