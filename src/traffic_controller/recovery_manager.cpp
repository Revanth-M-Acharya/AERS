/**
 * @file recovery_manager.cpp
 * @brief Implementation of RecoveryManager.
 */

#include "aegis/edge/recovery_manager.hpp"
#include "aegis/logger/logger.hpp"

namespace aegis {
namespace edge {
namespace traffic {

RecoveryManager::RecoveryManager() = default;

void RecoveryManager::StartRecovery() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_recoveryActive) {
        m_recoveryActive = true;
        logger::Logger::Instance().Log(
            logger::LogLevel::INFO, "RECOVERY", "Recovery Started");
    }
}

void RecoveryManager::RecoveryFinished() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_recoveryActive) {
        m_recoveryActive = false;
        logger::Logger::Instance().Log(
            logger::LogLevel::INFO, "RECOVERY", "Recovery Finished");
    }
}

void RecoveryManager::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_recoveryActive = false;
}

bool RecoveryManager::IsRecoveryActive() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_recoveryActive;
}

} // namespace traffic
} // namespace edge
} // namespace aegis
