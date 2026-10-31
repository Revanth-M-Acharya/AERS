/**
 * @file TrafficNodeConfig.cpp
 * @brief Implementation of Traffic Signal Node configuration loader.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "traffic_node/config/TrafficNodeConfig.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace aegis::traffic {

namespace {

std::string Trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

} // namespace

TrafficNodeConfig::TrafficNodeConfig() = default;

bool TrafficNodeConfig::LoadFromFile(const std::string& filePath) {
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

        if (key == "NodeID" || key == "node_id") {
            nodeId = val;
        } else if (key == "CoordinatorIP" || key == "coordinator_ip") {
            coordinatorIp = val;
        } else if (key == "CoordinatorPort" || key == "coordinator_port") {
            coordinatorPort = static_cast<uint16_t>(std::stoi(val));
        } else if (key == "HeartbeatInterval" || key == "heartbeat_interval_ms") {
            heartbeatIntervalMs = std::stoull(val);
        } else if (key == "WatchdogTimeout" || key == "watchdog_timeout_ms") {
            watchdogTimeoutMs = std::stoull(val);
        } else if (key == "ReconnectDelay" || key == "reconnect_delay_ms") {
            reconnectDelayMs = std::stoull(val);
        } else if (key == "ConnectTimeout" || key == "connect_timeout_ms") {
            connectTimeoutMs = static_cast<uint32_t>(std::stoul(val));
        } else if (key == "HeartbeatTimeout" || key == "heartbeat_timeout_ms") {
            heartbeatTimeoutMs = std::stoull(val);
        } else if (key == "RelayDelay" || key == "relay_switch_delay_ms" || key == "relay_delay_ms") {
            relaySwitchDelayMs = static_cast<uint32_t>(std::stoul(val));
        } else if (key == "LogLevel" || key == "log_level") {
            logLevel = val;
        } else if (key == "LogFile" || key == "log_file") {
            logFilePath = val;
        } else if (key == "LogToConsole" || key == "log_to_console") {
            logToConsole = (val == "1" || val == "true" || val == "yes");
        } else if (key == "RelayPinRed" || key == "relay_pin_red") {
            relayPinRed = std::stoi(val);
        } else if (key == "RelayPinYellow" || key == "relay_pin_yellow") {
            relayPinYellow = std::stoi(val);
        } else if (key == "RelayPinGreen" || key == "relay_pin_green") {
            relayPinGreen = std::stoi(val);
        } else if (key == "RelayPinAux" || key == "relay_pin_aux") {
            relayPinAux = std::stoi(val);
        } else if (key == "GreenDuration" || key == "green_duration_ms") {
            greenDurationMs = static_cast<uint32_t>(std::stoul(val));
        } else if (key == "YellowDuration" || key == "yellow_duration_ms") {
            yellowDurationMs = static_cast<uint32_t>(std::stoul(val));
        } else if (key == "RedDuration" || key == "red_duration_ms") {
            redDurationMs = static_cast<uint32_t>(std::stoul(val));
        } else if (key == "AllRedDuration" || key == "all_red_duration_ms") {
            allRedDurationMs = static_cast<uint32_t>(std::stoul(val));
        }
    }

    return Validate();
}

bool TrafficNodeConfig::Validate() const {
    if (nodeId.empty()) return false;
    if (coordinatorIp.empty()) return false;
    if (coordinatorPort == 0) return false;
    if (heartbeatIntervalMs < 100) return false;
    if (reconnectDelayMs < 100) return false;
    if (heartbeatTimeoutMs < heartbeatIntervalMs) return false;
    if (yellowDurationMs < 500) return false; // Safety requirement: minimum yellow clearance
    return true;
}

std::string TrafficNodeConfig::ToString() const {
    std::ostringstream oss;
    oss << "TrafficNodeConfig [NodeID: " << nodeId
        << ", Coordinator: " << coordinatorIp << ":" << coordinatorPort
        << ", Heartbeat: " << heartbeatIntervalMs << "ms"
        << ", ReconnectDelay: " << reconnectDelayMs << "ms"
        << ", Relays: R=" << relayPinRed << " Y=" << relayPinYellow << " G=" << relayPinGreen << " Aux=" << relayPinAux
        << "]";
    return oss.str();
}

} // namespace aegis::traffic

