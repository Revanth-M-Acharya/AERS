/**
 * @file RuntimeConfiguration.cpp
 * @brief SMART AEGIS (AERS) Phase 16D - Thread-Safe Runtime Configuration Manager Implementation.
 */

#include "RuntimeConfiguration.h"

namespace aegis {
namespace deployment {

RuntimeConfiguration::RuntimeConfiguration(const NodeConfiguration& initialConfig)
    : m_config(initialConfig) {
}

void RuntimeConfiguration::SetConfiguration(const NodeConfiguration& config) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_config = config;
    }
    NotifyCallbacks();
}

NodeConfiguration RuntimeConfiguration::GetConfiguration() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config;
}

void RuntimeConfiguration::SetLogLevel(LogLevel level) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_config.loggingLevel = level;
    }
    NotifyCallbacks();
}

LogLevel RuntimeConfiguration::GetLogLevel() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config.loggingLevel;
}

void RuntimeConfiguration::SetHeartbeatInterval(uint32_t intervalMs) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_config.heartbeatIntervalMs = intervalMs;
    }
    NotifyCallbacks();
}

uint32_t RuntimeConfiguration::GetHeartbeatInterval() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config.heartbeatIntervalMs;
}

void RuntimeConfiguration::SetReconnectInterval(uint32_t intervalMs) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_config.reconnectIntervalMs = intervalMs;
    }
    NotifyCallbacks();
}

uint32_t RuntimeConfiguration::GetReconnectInterval() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config.reconnectIntervalMs;
}

void RuntimeConfiguration::SetGpioEnabled(bool enabled) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_config.gpioEnabled = enabled;
    }
    NotifyCallbacks();
}

bool RuntimeConfiguration::IsGpioEnabled() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config.gpioEnabled;
}

void RuntimeConfiguration::SetSimulationEnabled(bool enabled) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_config.simulationEnabled = enabled;
    }
    NotifyCallbacks();
}

bool RuntimeConfiguration::IsSimulationEnabled() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config.simulationEnabled;
}

void RuntimeConfiguration::RegisterChangeCallback(ChangeCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callbacks.push_back(std::move(callback));
}

void RuntimeConfiguration::NotifyCallbacks() {
    NodeConfiguration copy;
    std::vector<ChangeCallback> callbacksCopy;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        copy = m_config;
        callbacksCopy = m_callbacks;
    }
    for (auto& cb : callbacksCopy) {
        if (cb) {
            cb(copy);
        }
    }
}

} // namespace deployment
} // namespace aegis
