/**
 * @file OrchestrationConfig.cpp
 * @brief Dynamic configuration parser and validator for Coordinator Orchestration.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/orchestration/config/OrchestrationConfig.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace aegis::coordinator::orchestration {

OrchestrationConfig::OrchestrationConfig() = default;

static std::string Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

bool OrchestrationConfig::LoadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) {
            continue;
        }

        std::string key = Trim(line.substr(0, eqPos));
        std::string val = Trim(line.substr(eqPos + 1));

        try {
            if (key == "HeartbeatTimeoutMs") {
                heartbeatTimeoutMs = static_cast<uint32_t>(std::stoul(val));
            } else if (key == "RetryCount") {
                retryCount = static_cast<uint32_t>(std::stoul(val));
            } else if (key == "RetryDelayMs") {
                retryDelayMs = static_cast<uint32_t>(std::stoul(val));
            } else if (key == "DispatchTimeoutMs") {
                dispatchTimeoutMs = static_cast<uint32_t>(std::stoul(val));
            } else if (key == "ClockDriftThresholdMs") {
                clockDriftThresholdMs = static_cast<uint32_t>(std::stoul(val));
            } else if (key == "MaxNodeDelayMs") {
                maxNodeDelayMs = static_cast<uint32_t>(std::stoul(val));
            } else if (key == "RecoveryTimeoutMs") {
                recoveryTimeoutMs = static_cast<uint32_t>(std::stoul(val));
            } else if (key == "FsmTimeoutMs") {
                fsmTimeoutMs = static_cast<uint32_t>(std::stoul(val));
            } else if (key == "EventQueueSize") {
                eventQueueSize = static_cast<uint32_t>(std::stoul(val));
            } else if (key == "ThreadPoolSize") {
                threadPoolSize = static_cast<uint32_t>(std::stoul(val));
            }
        } catch (...) {
            // Invalid number format, continue or retain default
        }
    }

    return Validate();
}

bool OrchestrationConfig::Validate() const {
    if (heartbeatTimeoutMs < 500 || heartbeatTimeoutMs > 60000) return false;
    if (retryCount < 1 || retryCount > 10) return false;
    if (retryDelayMs < 10 || retryDelayMs > 10000) return false;
    if (dispatchTimeoutMs < 200 || dispatchTimeoutMs > 30000) return false;
    if (clockDriftThresholdMs < 10 || clockDriftThresholdMs > 10000) return false;
    if (maxNodeDelayMs < 100 || maxNodeDelayMs > 30000) return false;
    if (recoveryTimeoutMs < 500 || recoveryTimeoutMs > 60000) return false;
    if (fsmTimeoutMs < 1000 || fsmTimeoutMs > 120000) return false;
    if (eventQueueSize < 8 || eventQueueSize > 65536) return false;
    if (threadPoolSize < 1 || threadPoolSize > 32) return false;
    return true;
}

std::string OrchestrationConfig::ToString() const {
    std::ostringstream oss;
    oss << "OrchestrationConfig: HeartbeatTimeoutMs=" << heartbeatTimeoutMs
        << ", RetryCount=" << retryCount
        << ", RetryDelayMs=" << retryDelayMs
        << ", DispatchTimeoutMs=" << dispatchTimeoutMs
        << ", ClockDriftThresholdMs=" << clockDriftThresholdMs
        << ", MaxNodeDelayMs=" << maxNodeDelayMs
        << ", RecoveryTimeoutMs=" << recoveryTimeoutMs
        << ", FsmTimeoutMs=" << fsmTimeoutMs
        << ", EventQueueSize=" << eventQueueSize
        << ", ThreadPoolSize=" << threadPoolSize;
    return oss.str();
}

} // namespace aegis::coordinator::orchestration

