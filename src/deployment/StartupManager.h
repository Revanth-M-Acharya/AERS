/**
 * @file StartupManager.h
 * @brief SMART AEGIS (AERS) Phase 16D - Node Startup & Environment Supervisor.
 *
 * Prepares system runtime environment, PID recording, log directory hierarchy,
 * and cluster readiness synchronization for the Central Coordinator.
 */

#ifndef AEGIS_DEPLOYMENT_STARTUP_MANAGER_H
#define AEGIS_DEPLOYMENT_STARTUP_MANAGER_H

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace aegis {
namespace deployment {

class StartupManager {
public:
    StartupManager();
    ~StartupManager();

    /**
     * @brief Ensure log directory hierarchy and environment permissions are established.
     * @param logDirectory Target directory path.
     * @return true on success; false otherwise.
     */
    static bool InitializeEnvironment(const std::string& logDirectory = "logs");

    /**
     * @brief Record current process PID to a pidfile for process management.
     * @param pidFilePath File path to record PID.
     * @return true on success; false on write failure.
     */
    static bool CreatePidFile(const std::string& pidFilePath);

    /**
     * @brief Remove PID file upon clean termination.
     * @param pidFilePath File path to remove.
     * @return true on removal; false otherwise.
     */
    static bool RemovePidFile(const std::string& pidFilePath);

    /**
     * @brief Setup POSIX/QNX signal handlers for graceful shutdown (SIGINT, SIGTERM).
     * @param handler Callback to invoke on signal delivery.
     * @return true on success.
     */
    static bool SetupSignalHandlers(std::function<void(int)> handler);

    /**
     * @brief Coordinator gating: wait until every expected node is online before accepting emergencies.
     * @param expectedNodes List of expected Node IDs (e.g. {"INT_A", "INT_B", "INT_C"}).
     * @param isNodeOnline Lambda returning true if a specific Node ID is online and registered.
     * @param timeoutMs Timeout in milliseconds to wait for full cluster readiness.
     * @return true if all expected nodes became online within timeout; false otherwise.
     */
    static bool WaitForClusterOnline(const std::vector<std::string>& expectedNodes,
                                     std::function<bool(const std::string&)> isNodeOnline,
                                     uint32_t timeoutMs = 5000);
};

} // namespace deployment
} // namespace aegis

#endif // AEGIS_DEPLOYMENT_STARTUP_MANAGER_H
