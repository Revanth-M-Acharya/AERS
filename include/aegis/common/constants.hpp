/**
 * @file constants.hpp
 * @brief System-wide physical constants, timing thresholds, and network ports.
 */

#ifndef AEGIS_COMMON_CONSTANTS_HPP
#define AEGIS_COMMON_CONSTANTS_HPP

#include <cstdint>

namespace aegis {
namespace common {
namespace constants {

// Timing Constants (Milliseconds)
constexpr uint32_t HEARTBEAT_INTERVAL_MS = 200;
constexpr uint32_t WATCHDOG_TIMEOUT_MS = 500;
constexpr uint32_t GPS_SAMPLE_PERIOD_MS = 100;       // 10 Hz
constexpr uint32_t VITALS_SAMPLE_PERIOD_MS = 1000;    // 1 Hz
constexpr uint32_t YELLOW_CLEARANCE_MIN_MS = 3000;   // Minimum mandatory
constexpr uint32_t ALL_RED_CLEARANCE_MIN_MS = 1000;  // Minimum mandatory
constexpr uint32_t MAX_PREEMPTION_HOLD_MS = 20000;   // Safe auto-release

// Network Port Allocations
constexpr uint16_t MQTT_TLS_PORT = 8883;
constexpr uint16_t MQTT_WS_PORT = 9001;
constexpr uint16_t V2V_UDP_PORT = 8999;
constexpr uint16_t V2I_UDP_PORT = 8998;
constexpr uint16_t RPC_TCP_PORT = 50051;

} // namespace constants
} // namespace common
} // namespace aegis

#endif // AEGIS_COMMON_CONSTANTS_HPP
