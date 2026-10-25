/**
 * @file preemption_manager.cpp
 * @brief Implementation of PreemptionManager.
 */

#include "aegis/edge/preemption_manager.hpp"
#include "aegis/logger/logger.hpp"

namespace aegis {
namespace edge {
namespace traffic {

PreemptionManager::PreemptionManager() = default;

void PreemptionManager::StartEmergency() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_emergencyActive) {
        m_emergencyActive = true;
        logger::Logger::Instance().Log(
            logger::LogLevel::INFO, "PREEMPTION", "Emergency Activated");
    }
}

void PreemptionManager::CancelEmergency() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_emergencyActive) {
        m_emergencyActive = false;
        logger::Logger::Instance().Log(
            logger::LogLevel::INFO, "PREEMPTION", "Emergency Cleared");
    }
}

bool PreemptionManager::IsEmergencyActive() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_emergencyActive;
}

} // namespace traffic
} // namespace edge
} // namespace aegis
