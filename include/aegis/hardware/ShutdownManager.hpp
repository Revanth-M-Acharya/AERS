/**
 * @file ShutdownManager.hpp
 * @brief SMART AEGIS (AERS) Phase 20.7 - Production Hardware Safe Shutdown Manager.
 *
 * Implements the deterministic 10-step graceful safe shutdown sequence:
 * 1. Stop Coordinator Commands
 * 2. Notify Peers
 * 3. Stop Watchdog
 * 4. Save Logs
 * 5. Save Diagnostics
 * 6. Turn OFF Relays
 * 7. Force ALL RED
 * 8. Release GPIO
 * 9. Close Sockets
 * 10. Shutdown Complete
 *
 * Handles SIGINT, SIGTERM, Fatal Errors, and Operator Power-Off requests.
 */

#ifndef AEGIS_HARDWARE_SHUTDOWN_MANAGER_HPP
#define AEGIS_HARDWARE_SHUTDOWN_MANAGER_HPP

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>
#include <cstdint>

namespace aegis {
namespace hardware {

enum class ShutdownReason : uint8_t {
    NORMAL = 0,
    SIGINT_RECEIVED,
    SIGTERM_RECEIVED,
    FATAL_ERROR,
    POWER_OFF_REQUEST
};

inline std::string ShutdownReasonToString(ShutdownReason reason) {
    switch (reason) {
        case ShutdownReason::NORMAL:            return "NORMAL";
        case ShutdownReason::SIGINT_RECEIVED:   return "SIGINT_RECEIVED";
        case ShutdownReason::SIGTERM_RECEIVED:  return "SIGTERM_RECEIVED";
        case ShutdownReason::FATAL_ERROR:       return "FATAL_ERROR";
        case ShutdownReason::POWER_OFF_REQUEST: return "POWER_OFF_REQUEST";
        default:                                return "UNKNOWN";
    }
}

enum class ShutdownStep : uint8_t {
    IDLE = 0,
    STOP_COORDINATOR_COMMANDS = 1,
    NOTIFY_PEERS = 2,
    STOP_WATCHDOG = 3,
    SAVE_LOGS = 4,
    SAVE_DIAGNOSTICS = 5,
    TURN_OFF_RELAYS = 6,
    FORCE_ALL_RED = 7,
    RELEASE_GPIO = 8,
    CLOSE_SOCKETS = 9,
    SHUTDOWN_COMPLETE = 10
};

/**
 * @brief Phase 2 - 10-Stage Industrial Safe Shutdown Pipeline.
 */
enum class IndustrialShutdownStep : uint8_t {
    IDLE = 0,
    STOP_ACCEPTING_COMMANDS = 1,
    COMPLETE_ACTIVE_TRANSITION = 2,
    ALL_RED_CLEARANCE = 3,
    DEACTIVATE_RELAYS = 4,
    STORE_LOGS = 5,
    RELEASE_GPIO = 6,
    STOP_THREADS = 7,
    CLOSE_SOCKETS = 8,
    UNMAP_BSP_MEMORY = 9,
    SHUTDOWN_COMPLETE = 10
};

inline std::string IndustrialShutdownStepToString(IndustrialShutdownStep step) {
    switch (step) {
        case IndustrialShutdownStep::IDLE:                       return "IDLE";
        case IndustrialShutdownStep::STOP_ACCEPTING_COMMANDS:    return "1_STOP_ACCEPTING_COMMANDS";
        case IndustrialShutdownStep::COMPLETE_ACTIVE_TRANSITION: return "2_COMPLETE_ACTIVE_TRANSITION";
        case IndustrialShutdownStep::ALL_RED_CLEARANCE:          return "3_ALL_RED_CLEARANCE";
        case IndustrialShutdownStep::DEACTIVATE_RELAYS:          return "4_DEACTIVATE_RELAYS";
        case IndustrialShutdownStep::STORE_LOGS:                 return "5_STORE_LOGS";
        case IndustrialShutdownStep::RELEASE_GPIO:               return "6_RELEASE_GPIO";
        case IndustrialShutdownStep::STOP_THREADS:               return "7_STOP_THREADS";
        case IndustrialShutdownStep::CLOSE_SOCKETS:              return "8_CLOSE_SOCKETS";
        case IndustrialShutdownStep::UNMAP_BSP_MEMORY:           return "9_UNMAP_BSP_MEMORY";
        case IndustrialShutdownStep::SHUTDOWN_COMPLETE:          return "10_SHUTDOWN_COMPLETE";
        default:                                                 return "UNKNOWN";
    }
}

inline std::string ShutdownStepToString(ShutdownStep step) {
    switch (step) {
        case ShutdownStep::IDLE:                      return "IDLE";
        case ShutdownStep::STOP_COORDINATOR_COMMANDS: return "1_STOP_COORDINATOR_COMMANDS";
        case ShutdownStep::NOTIFY_PEERS:             return "2_NOTIFY_PEERS";
        case ShutdownStep::STOP_WATCHDOG:            return "3_STOP_WATCHDOG";
        case ShutdownStep::SAVE_LOGS:                return "4_SAVE_LOGS";
        case ShutdownStep::SAVE_DIAGNOSTICS:         return "5_SAVE_DIAGNOSTICS";
        case ShutdownStep::TURN_OFF_RELAYS:          return "6_TURN_OFF_RELAYS";
        case ShutdownStep::FORCE_ALL_RED:            return "7_FORCE_ALL_RED";
        case ShutdownStep::RELEASE_GPIO:             return "8_RELEASE_GPIO";
        case ShutdownStep::CLOSE_SOCKETS:            return "9_CLOSE_SOCKETS";
        case ShutdownStep::SHUTDOWN_COMPLETE:        return "10_SHUTDOWN_COMPLETE";
        default:                                     return "UNKNOWN";
    }
}

struct ShutdownStepReport {
    ShutdownStep step{ShutdownStep::IDLE};
    std::string stepName;
    bool passed{false};
    uint64_t durationUs{0};
    std::string details;
};

/**
 * @brief Production Safe Shutdown Manager executing deterministic multi-step safe termination.
 */
class ShutdownManager {
public:
    using ShutdownActionHook = std::function<bool(std::string& outDetails)>;

    static ShutdownManager& Instance();

    ShutdownManager();
    ~ShutdownManager() = default;

    // Non-copyable, non-movable
    ShutdownManager(const ShutdownManager&) = delete;
    ShutdownManager& operator=(const ShutdownManager&) = delete;

    /**
     * @brief Install POSIX SIGINT and SIGTERM handlers to trigger graceful safe shutdown.
     */
    void InstallSignalHandlers();

    /**
     * @brief Register specific execution hooks for any shutdown step.
     */
    void SetStepHook(ShutdownStep step, ShutdownActionHook hook);

    /**
     * @brief Register specific execution hooks for Phase 2 industrial shutdown steps.
     */
    void SetIndustrialStepHook(IndustrialShutdownStep step, ShutdownActionHook hook);

    /**
     * @brief Execute the full deterministic 10-step safe shutdown sequence.
     * @param reason The trigger cause for shutdown.
     * @return true if all steps executed successfully; false on any step failure.
     */
    bool ExecuteShutdown(ShutdownReason reason = ShutdownReason::NORMAL);

    /**
     * @brief Execute the Phase 2 non-reentrant industrial safe shutdown:
     * Stop accepting new commands -> Complete active traffic transition ->
     * All Red Clearance -> Deactivate relays (relays never energized) ->
     * Store logs -> Release GPIO -> Stop threads -> Close sockets ->
     * Unmap BSP memory -> Shutdown Complete.
     * @param reason The trigger cause for shutdown.
     * @return true if all 10 industrial steps succeed; false on any failure.
     */
    bool ExecuteIndustrialShutdown(ShutdownReason reason = ShutdownReason::NORMAL);

    // Queries
    ShutdownStep GetCurrentStep() const noexcept;
    IndustrialShutdownStep GetCurrentIndustrialStep() const noexcept;
    ShutdownReason GetShutdownReason() const noexcept;
    bool IsShuttingDown() const noexcept;
    bool IsShutdownComplete() const noexcept;

    std::vector<ShutdownStepReport> GetStepReports() const;
    std::string GenerateShutdownReport() const;
    std::string GenerateIndustrialShutdownReport() const;

    void Reset();

private:
    static void PosixSignalHandler(int signum);

    mutable std::recursive_mutex m_mutex;
    std::atomic<bool> m_isShuttingDown{false};
    std::atomic<bool> m_isTerminated{false};
    std::atomic<ShutdownStep> m_currentStep{ShutdownStep::IDLE};
    std::atomic<IndustrialShutdownStep> m_currentIndustrialStep{IndustrialShutdownStep::IDLE};
    std::atomic<ShutdownReason> m_reason{ShutdownReason::NORMAL};

    std::unordered_map<uint8_t, ShutdownActionHook> m_hooks;
    std::unordered_map<uint8_t, ShutdownActionHook> m_industrialHooks;
    std::vector<ShutdownStepReport> m_reports;
};

} // namespace hardware
} // namespace aegis

#endif // AEGIS_HARDWARE_SHUTDOWN_MANAGER_HPP
