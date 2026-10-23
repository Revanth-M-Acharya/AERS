#include "aegis/safety/safety_monitor.hpp"

namespace aegis {
namespace safety {

SafetyMonitor::SafetyMonitor() = default;
SafetyMonitor::~SafetyMonitor() = default;

bool SafetyMonitor::Initialize(uint32_t /*heartbeatIntervalMs*/, uint32_t watchdogTimeoutMs) {
    return m_watchdog.Initialize(watchdogTimeoutMs);
}

void SafetyMonitor::RegisterHeartbeat(std::string_view /*moduleName*/) {
    // TODO: Record heartbeat timestamp for registered module
}

void SafetyMonitor::Pulse() {
    // TODO: Verify all registered modules are within heartbeat deadlines before kicking WDT
    m_watchdog.Kick();
}

bool SafetyMonitor::IsSystemHealthy() const {
    return true;
}

} // namespace safety
} // namespace aegis
