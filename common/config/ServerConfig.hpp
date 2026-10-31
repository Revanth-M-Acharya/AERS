/**
 * @file ServerConfig.hpp
 * @brief Configuration model and parser for SMART AEGIS Coordinator TCP Server.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <cstdint>

namespace aegis::config {

class ServerConfig {
public:
    ServerConfig();

    // Configuration Properties
    std::string bindAddress{"0.0.0.0"};
    uint16_t port{8080};
    size_t maxClients{32};
    int listenBacklog{16};
    uint64_t heartbeatTimeoutMs{5000};
    uint32_t pollTimeoutMs{100};
    bool enableTcpNoDelay{true};
    int workerThreadPriority{25}; // Real-time priority on QNX (SCHED_FIFO)
    std::string logFilePath{"coordinator.log"};
    bool logToConsole{true};

    /**
     * @brief Load configuration parameters from key-value file (e.g. key=value).
     * @param filePath Absolute or relative path to .conf file.
     * @return true if file opened and parsed successfully, false on error.
     */
    bool LoadFromFile(const std::string& filePath);

    /**
     * @brief Validate configuration parameters.
     * @return true if valid, false if invalid parameters detected.
     */
    bool Validate() const;

    /**
     * @brief Format configuration summary into string.
     */
    std::string ToString() const;
};

} // namespace aegis::config

