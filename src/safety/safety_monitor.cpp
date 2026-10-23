#include "safety_monitor.hpp"

namespace aegis {
namespace safety {

SafetyMonitor::SafetyMonitor() = default;
SafetyMonitor::~SafetyMonitor() = default;

void SafetyMonitor::RegisterHeartbeat(std::string_view /*moduleName*/) {
    // Placeholder: Register module pulse check
}

void SafetyMonitor::Pulse() {
    // Placeholder: Verify all module heartbeats within deadline, then kick hardware WDT
    m_watchdog.Kick();
}

bool SafetyMonitor::IsSystemHealthy() const {
    return true;
}

} // namespace safety
} // namespace aegis
