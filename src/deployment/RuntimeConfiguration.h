/**
 * @file RuntimeConfiguration.h
 * @brief SMART AEGIS (AERS) Phase 16D - Thread-Safe Runtime Configuration Manager.
 *
 * Provides dynamic runtime parameter tuning, log level adjustment, and
 * configuration inspection for active nodes without restarting.
 */

#ifndef AEGIS_DEPLOYMENT_RUNTIME_CONFIGURATION_H
#define AEGIS_DEPLOYMENT_RUNTIME_CONFIGURATION_H

#include "NodeConfiguration.h"
#include <mutex>
#include <vector>
#include <functional>

namespace aegis {
namespace deployment {

class RuntimeConfiguration {
public:
    using ChangeCallback = std::function<void(const NodeConfiguration&)>;

    explicit RuntimeConfiguration(const NodeConfiguration& initialConfig = NodeConfiguration());
    ~RuntimeConfiguration() = default;

    // Non-copyable, non-movable
    RuntimeConfiguration(const RuntimeConfiguration&) = delete;
    RuntimeConfiguration& operator=(const RuntimeConfiguration&) = delete;

    /**
     * @brief Set entire active configuration.
     */
    void SetConfiguration(const NodeConfiguration& config);

    /**
     * @brief Get snapshot copy of active configuration.
     */
    NodeConfiguration GetConfiguration() const;

    /**
     * @brief Dynamically adjust logging level.
     */
    void SetLogLevel(LogLevel level);

    /**
     * @brief Get active logging level.
     */
    LogLevel GetLogLevel() const;

    /**
     * @brief Dynamically adjust heartbeat interval.
     */
    void SetHeartbeatInterval(uint32_t intervalMs);

    /**
     * @brief Get active heartbeat interval in ms.
     */
    uint32_t GetHeartbeatInterval() const;

    /**
     * @brief Dynamically adjust reconnect interval.
     */
    void SetReconnectInterval(uint32_t intervalMs);

    /**
     * @brief Get active reconnect interval in ms.
     */
    uint32_t GetReconnectInterval() const;

    /**
     * @brief Enable or disable physical GPIO actuation.
     */
    void SetGpioEnabled(bool enabled);

    /**
     * @brief Check if GPIO actuation is enabled.
     */
    bool IsGpioEnabled() const;

    /**
     * @brief Enable or disable simulated hardware drivers.
     */
    void SetSimulationEnabled(bool enabled);

    /**
     * @brief Check if simulation is enabled.
     */
    bool IsSimulationEnabled() const;

    /**
     * @brief Register a listener callback to be notified on configuration changes.
     */
    void RegisterChangeCallback(ChangeCallback callback);

private:
    void NotifyCallbacks();

    mutable std::mutex m_mutex;
    NodeConfiguration m_config;
    std::vector<ChangeCallback> m_callbacks;
};

} // namespace deployment
} // namespace aegis

#endif // AEGIS_DEPLOYMENT_RUNTIME_CONFIGURATION_H
