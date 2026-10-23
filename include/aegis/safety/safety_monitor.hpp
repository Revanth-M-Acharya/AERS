#ifndef AEGIS_SAFETY_SAFETY_MONITOR_HPP
#define AEGIS_SAFETY_SAFETY_MONITOR_HPP

#include <string_view>
#include <memory>
#include "aegis/safety/watchdog.hpp"

namespace aegis {
namespace safety {

/**
 * @brief Central safety supervisor monitoring process heartbeats and enforcing fail-safe states.
 * TODO: Implement multi-thread pulse monitoring and degraded state transitions.
 */
class SafetyMonitor {
public:
    SafetyMonitor();
    ~SafetyMonitor();

    bool Initialize(uint32_t heartbeatIntervalMs, uint32_t watchdogTimeoutMs);
    void RegisterHeartbeat(std::string_view moduleName);
    void Pulse();
    bool IsSystemHealthy() const;

private:
    Watchdog m_watchdog;
};

} // namespace safety
} // namespace aegis

#endif // AEGIS_SAFETY_SAFETY_MONITOR_HPP
