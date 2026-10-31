/**
 * @file ServerConfig.cpp
 * @brief Implementation of Coordinator TCP Server configuration loader.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "common/config/ServerConfig.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace aegis::config {

namespace {

std::string Trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

} // namespace

ServerConfig::ServerConfig() = default;

bool ServerConfig::LoadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) {
            continue;
        }

        std::string key = Trim(line.substr(0, eqPos));
        std::string val = Trim(line.substr(eqPos + 1));

        if (key == "bind_address" || key == "ip") {
            bindAddress = val;
        } else if (key == "port") {
            port = static_cast<uint16_t>(std::stoi(val));
        } else if (key == "max_clients") {
            maxClients = static_cast<size_t>(std::stoul(val));
        } else if (key == "listen_backlog") {
            listenBacklog = std::stoi(val);
        } else if (key == "heartbeat_timeout_ms") {
            heartbeatTimeoutMs = std::stoull(val);
        } else if (key == "poll_timeout_ms") {
            pollTimeoutMs = static_cast<uint32_t>(std::stoul(val));
        } else if (key == "tcp_nodelay") {
            enableTcpNoDelay = (val == "1" || val == "true" || val == "yes");
        } else if (key == "worker_priority") {
            workerThreadPriority = std::stoi(val);
        } else if (key == "log_file") {
            logFilePath = val;
        } else if (key == "log_to_console") {
            logToConsole = (val == "1" || val == "true" || val == "yes");
        }
    }

    return Validate();
}

bool ServerConfig::Validate() const {
    if (port == 0) return false;
    if (maxClients == 0 || maxClients > 1024) return false;
    if (listenBacklog <= 0) return false;
    if (heartbeatTimeoutMs < 100) return false;
    return true;
}

std::string ServerConfig::ToString() const {
    std::ostringstream oss;
    oss << "ServerConfig [Bind: " << bindAddress << ":" << port
        << ", MaxClients: " << maxClients
        << ", Backlog: " << listenBacklog
        << ", HeartbeatTimeout: " << heartbeatTimeoutMs << "ms"
        << ", TcpNoDelay: " << (enableTcpNoDelay ? "true" : "false")
        << ", Priority: " << workerThreadPriority << "]";
    return oss.str();
}

} // namespace aegis::config

