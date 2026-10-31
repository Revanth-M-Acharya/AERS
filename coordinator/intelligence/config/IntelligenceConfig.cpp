/**
 * @file IntelligenceConfig.cpp
 * @brief Implementation of IntelligenceConfig loader.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/intelligence/config/IntelligenceConfig.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace aegis::coordinator::intelligence {

namespace {
std::string Trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}
} // namespace

IntelligenceConfig::IntelligenceConfig() = default;

bool IntelligenceConfig::LoadFromFile(const std::string& filePath) {
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
        if (eqPos == std::string::npos) continue;

        std::string key = Trim(line.substr(0, eqPos));
        std::string val = Trim(line.substr(eqPos + 1));

        if (key == "RoadNetworkConfigFile" || key == "road_network_file") {
            roadNetworkConfigFile = val;
        } else if (key == "DefaultSpeedLimitKmh" || key == "default_speed_limit_kmh") {
            defaultSpeedLimitKmh = std::stod(val);
        } else if (key == "GreenLeadTimeMs" || key == "green_lead_time_ms") {
            greenLeadTimeMs = static_cast<uint32_t>(std::stoul(val));
        } else if (key == "GreenHoldDurationMs" || key == "green_hold_duration_ms") {
            greenHoldDurationMs = static_cast<uint32_t>(std::stoul(val));
        } else if (key == "StarvationAgingThresholdMs" || key == "starvation_threshold_ms") {
            starvationAgingThresholdMs = std::stoull(val);
        } else if (key == "EtaEmaAlpha" || key == "eta_ema_alpha") {
            etaEmaAlpha = std::stod(val);
        } else if (key == "MinSafetyHoldDurationMs" || key == "min_safety_hold_ms") {
            minSafetyHoldDurationMs = static_cast<uint32_t>(std::stoul(val));
        } else if (key == "MaxSafetyHoldDurationMs" || key == "max_safety_hold_ms") {
            maxSafetyHoldDurationMs = static_cast<uint32_t>(std::stoul(val));
        }
    }

    return Validate();
}

bool IntelligenceConfig::Validate() const {
    if (defaultSpeedLimitKmh <= 0.0) return false;
    if (greenLeadTimeMs < 500) return false;
    if (greenHoldDurationMs < 1000) return false;
    if (etaEmaAlpha <= 0.0 || etaEmaAlpha >= 1.0) return false;
    if (minSafetyHoldDurationMs < 500) return false;
    if (maxSafetyHoldDurationMs < minSafetyHoldDurationMs) return false;
    return true;
}

std::string IntelligenceConfig::ToString() const {
    std::ostringstream oss;
    oss << "IntelligenceConfig [RoadNetwork: " << roadNetworkConfigFile
        << ", GreenLead: " << greenLeadTimeMs << "ms"
        << ", GreenHold: " << greenHoldDurationMs << "ms"
        << ", StarvationThreshold: " << starvationAgingThresholdMs << "ms"
        << ", EtaAlpha: " << etaEmaAlpha
        << "]";
    return oss.str();
}

} // namespace aegis::coordinator::intelligence

