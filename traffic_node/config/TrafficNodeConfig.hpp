/**
 * @file TrafficNodeConfig.hpp
 * @brief Configuration model and parser for SMART AEGIS Traffic Signal Node.
 * All timing, hardware pin mappings, and resilience thresholds are fully configurable.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <cstdint>

namespace aegis::traffic {

class TrafficNodeConfig {
public:
    TrafficNodeConfig();

    // Node Identity & Coordinator Networking
    std::string nodeId{"INT_A"};
    std::string coordinatorIp{"127.0.0.1"};
    uint16_t coordinatorPort{8080};

    // Timing & Resilience
    uint64_t heartbeatIntervalMs{1000};
    uint64_t watchdogTimeoutMs{5000};
    uint64_t reconnectDelayMs{2000};
    uint32_t connectTimeoutMs{3000};
    uint64_t heartbeatTimeoutMs{5000};
    uint32_t relaySwitchDelayMs{50};

    // Logging
    std::string logLevel{"INFO"};
    std::string logFilePath{"traffic_node.log"};
    bool logToConsole{true};

    // Relay Hardware Pin Assignments (BCM GPIO on Raspberry Pi 4)
    int relayPinRed{17};
    int relayPinYellow{27};
    int relayPinGreen{22};
    int relayPinAux{23};

    // Safety Clearance Timings (Milliseconds)
    uint32_t greenDurationMs{10000};
    uint32_t yellowDurationMs{3000};
    uint32_t redDurationMs{10000};
    uint32_t allRedDurationMs{1000};

    /**
     * @brief Load configuration from key=value formatted configuration file.
     * @param filePath Path to TrafficNode.conf file.
     * @return true if parsed and valid, false otherwise.
     */
    bool LoadFromFile(const std::string& filePath);

    /**
     * @brief Validate configuration parameters against safety ranges.
     * @return true if valid, false if out of range or malformed.
     */
    bool Validate() const;

    /**
     * @brief Format configuration summary for telemetry and logging.
     */
    std::string ToString() const;
};

} // namespace aegis::traffic
