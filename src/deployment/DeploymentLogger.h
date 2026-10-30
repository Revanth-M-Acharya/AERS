/**
 * @file DeploymentLogger.h
 * @brief SMART AEGIS (AERS) Phase 16D - Node Deployment Multi-File Logger.
 *
 * Implements deterministic multi-stream file logging for every node:
 * - runtime.log
 * - network.log
 * - gpio.log
 * - diagnostics.log
 * - crash.log
 */

#ifndef AEGIS_DEPLOYMENT_LOGGER_H
#define AEGIS_DEPLOYMENT_LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <cstdint>

namespace aegis {
namespace deployment {

class DeploymentLogger {
public:
    DeploymentLogger();
    ~DeploymentLogger();

    // Non-copyable, non-movable
    DeploymentLogger(const DeploymentLogger&) = delete;
    DeploymentLogger& operator=(const DeploymentLogger&) = delete;

    /**
     * @brief Initialize logging streams and create log files.
     * @param logDirectory Directory path where log files will reside (default "logs").
     * @param nodeId Node ID for log headers.
     * @return true on success, false if log directory or files cannot be opened.
     */
    bool Initialize(const std::string& logDirectory = "logs", const std::string& nodeId = "NODE");

    /**
     * @brief Close all open log file streams.
     */
    void Close();

    /**
     * @brief Log a runtime operational event to runtime.log.
     */
    void LogRuntime(const std::string& level, const std::string& component, const std::string& message);

    /**
     * @brief Log a network packet transaction to network.log.
     */
    void LogNetwork(const std::string& direction,
                    const std::string& peerNode,
                    const std::string& messageType,
                    size_t bytes,
                    const std::string& details);

    /**
     * @brief Log a GPIO pin state change or sensor event to gpio.log.
     */
    void LogGpio(uint32_t pin, const std::string& action, const std::string& state, const std::string& details);

    /**
     * @brief Log health telemetry to diagnostics.log.
     */
    void LogDiagnostics(const std::string& component,
                        double cpuPercent,
                        double memoryPercent,
                        double temperatureC,
                        double healthScore,
                        const std::string& notes);

    /**
     * @brief Record critical failure or emergency stop to crash.log.
     */
    void LogCrash(const std::string& reason, const std::string& location, const std::string& details);

    /**
     * @brief Query whether logger has been initialized.
     */
    bool IsInitialized() const noexcept { return m_initialized; }

    /**
     * @brief Get configured log directory path.
     */
    const std::string& GetLogDirectory() const noexcept { return m_logDir; }

private:
    std::string GetTimestampString() const;

    mutable std::mutex m_mutex;
    std::string m_logDir{"logs"};
    std::string m_nodeId{"NODE"};
    bool m_initialized{false};

    std::ofstream m_runtimeFile;
    std::ofstream m_networkFile;
    std::ofstream m_gpioFile;
    std::ofstream m_diagnosticsFile;
    std::ofstream m_crashFile;
};

} // namespace deployment
} // namespace aegis

#endif // AEGIS_DEPLOYMENT_LOGGER_H
