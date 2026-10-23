#ifndef AEGIS_SAFETY_SAFETY_MONITOR_HPP
#define AEGIS_SAFETY_SAFETY_MONITOR_HPP

#include <string_view>
#include "watchdog.hpp"

namespace aegis {
namespace safety {

class SafetyMonitor {
public:
    SafetyMonitor();
    ~SafetyMonitor();

    void RegisterHeartbeat(std::string_view moduleName);
    void Pulse();
    bool IsSystemHealthy() const;

private:
    Watchdog m_watchdog;
};

} // namespace safety
} // namespace aegis

#endif // AEGIS_SAFETY_SAFETY_MONITOR_HPP
