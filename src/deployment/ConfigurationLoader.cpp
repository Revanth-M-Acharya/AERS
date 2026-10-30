/**
 * @file ConfigurationLoader.cpp
 * @brief SMART AEGIS (AERS) Phase 16D - Deployment Configuration Loader Implementation.
 */

#include "ConfigurationLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <sys/stat.h>

namespace aegis {
namespace deployment {

namespace {

std::string Trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n\"");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n\"");
    return str.substr(start, end - start + 1);
}

bool ExtractStringField(const std::string& json, const std::string& key, std::string& outValue) {
    std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return false;
    size_t q1 = json.find('\"', pos);
    if (q1 == std::string::npos) return false;
    size_t q2 = json.find('\"', q1 + 1);
    if (q2 == std::string::npos) return false;
    outValue = json.substr(q1 + 1, q2 - q1 - 1);
    return true;
}

bool ExtractNumberField(const std::string& json, const std::string& key, uint32_t& outValue) {
    std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return false;
    size_t start = json.find_first_of("0123456789", pos);
    if (start == std::string::npos) return false;
    size_t end = json.find_first_not_of("0123456789", start);
    std::string numStr = (end == std::string::npos) ? json.substr(start) : json.substr(start, end - start);
    try {
        outValue = static_cast<uint32_t>(std::stoul(numStr));
        return true;
    } catch (...) {
        return false;
    }
}

bool ExtractBooleanField(const std::string& json, const std::string& key, bool& outValue) {
    std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return false;
    if (json.find("true", pos) < json.find_first_of(",}\n", pos)) {
        outValue = true;
        return true;
    }
    if (json.find("false", pos) < json.find_first_of(",}\n", pos)) {
        outValue = false;
        return true;
    }
    return false;
}

bool ExtractArrayField(const std::string& json, const std::string& key, std::vector<std::string>& outArray) {
    std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) return false;
    size_t b1 = json.find('[', pos);
    if (b1 == std::string::npos) return false;
    size_t b2 = json.find(']', b1);
    if (b2 == std::string::npos) return false;
    std::string arrayContent = json.substr(b1 + 1, b2 - b1 - 1);
    outArray.clear();
    std::istringstream iss(arrayContent);
    std::string token;
    while (std::getline(iss, token, ',')) {
        std::string cleaned = Trim(token);
        if (!cleaned.empty()) {
            outArray.push_back(cleaned);
        }
    }
    return true;
}

} // anonymous namespace

bool ConfigurationLoader::LoadFromFile(const std::string& filePath,
                                       NodeConfiguration& outConfig,
                                       std::string* outError) {
    std::ifstream ifs(filePath);
    if (!ifs.is_open()) {
        if (outError) *outError = "Cannot open configuration file: " + filePath;
        return false;
    }
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return LoadFromString(ss.str(), outConfig, outError);
}

bool ConfigurationLoader::LoadFromString(const std::string& jsonStr,
                                         NodeConfiguration& outConfig,
                                         std::string* outError) {
    if (jsonStr.empty()) {
        if (outError) *outError = "Configuration JSON string is empty";
        return false;
    }

    std::string strVal;
    uint32_t numVal = 0;
    bool boolVal = false;

    // Node Identity
    if (ExtractStringField(jsonStr, "nodeId", strVal)) {
        outConfig.nodeId = strVal;
    }

    if (ExtractStringField(jsonStr, "role", strVal)) {
        outConfig.role = StringToNodeRole(strVal);
    }

    if (ExtractStringField(jsonStr, "intersectionName", strVal)) {
        outConfig.intersectionName = strVal;
    } else if (outConfig.role == NodeRole::TRAFFIC_NODE && outConfig.intersectionName.empty()) {
        outConfig.intersectionName = "INTERSECTION_" + outConfig.nodeId;
    }

    // Network Bindings
    if (ExtractStringField(jsonStr, "ipAddress", strVal)) {
        outConfig.ipAddress = strVal;
    }

    if (ExtractNumberField(jsonStr, "port", numVal)) {
        outConfig.port = static_cast<uint16_t>(numVal);
    }

    // Coordinator endpoints (check both coordinatorAddress and coordinatorIp)
    if (ExtractStringField(jsonStr, "coordinatorAddress", strVal)) {
        outConfig.coordinatorAddress = strVal;
    } else if (ExtractStringField(jsonStr, "coordinatorIp", strVal)) {
        outConfig.coordinatorAddress = strVal;
    }

    if (ExtractNumberField(jsonStr, "coordinatorPort", numVal)) {
        outConfig.coordinatorPort = static_cast<uint16_t>(numVal);
    }

    // Operational Timers
    if (ExtractNumberField(jsonStr, "heartbeatIntervalMs", numVal)) {
        outConfig.heartbeatIntervalMs = numVal;
    }

    if (ExtractNumberField(jsonStr, "reconnectIntervalMs", numVal)) {
        outConfig.reconnectIntervalMs = numVal;
    }

    if (ExtractNumberField(jsonStr, "connectTimeoutMs", numVal)) {
        outConfig.connectTimeoutMs = numVal;
    }

    // Flags & Logging
    if (ExtractStringField(jsonStr, "loggingLevel", strVal)) {
        outConfig.loggingLevel = StringToLogLevel(strVal);
    }

    if (ExtractBooleanField(jsonStr, "gpioEnabled", boolVal)) {
        outConfig.gpioEnabled = boolVal;
    }

    if (ExtractBooleanField(jsonStr, "simulationEnabled", boolVal)) {
        outConfig.simulationEnabled = boolVal;
    }

    if (ExtractStringField(jsonStr, "logDirectory", strVal)) {
        outConfig.logDirectory = strVal;
    }

    // Expected Nodes array
    std::vector<std::string> nodes;
    if (ExtractArrayField(jsonStr, "expectedNodes", nodes)) {
        outConfig.expectedNodes = nodes;
    }

    // GPIO Mapping fields (if present)
    if (ExtractNumberField(jsonStr, "redPin", numVal)) outConfig.gpioMapping.redPin = numVal;
    if (ExtractNumberField(jsonStr, "yellowPin", numVal)) outConfig.gpioMapping.yellowPin = numVal;
    if (ExtractNumberField(jsonStr, "greenPin", numVal)) outConfig.gpioMapping.greenPin = numVal;
    if (ExtractNumberField(jsonStr, "nsGreenPin", numVal)) outConfig.gpioMapping.nsGreenPin = numVal;
    if (ExtractNumberField(jsonStr, "ewGreenPin", numVal)) outConfig.gpioMapping.ewGreenPin = numVal;
    if (ExtractNumberField(jsonStr, "pedWalkPin", numVal)) outConfig.gpioMapping.pedWalkPin = numVal;
    if (ExtractNumberField(jsonStr, "pedDontWalkPin", numVal)) outConfig.gpioMapping.pedDontWalkPin = numVal;
    if (ExtractNumberField(jsonStr, "preemptStrobePin", numVal)) outConfig.gpioMapping.preemptStrobePin = numVal;
    if (ExtractNumberField(jsonStr, "irSensorNorthSouth", numVal)) outConfig.gpioMapping.irSensorNorthSouth = numVal;
    if (ExtractNumberField(jsonStr, "irSensorEastWest", numVal)) outConfig.gpioMapping.irSensorEastWest = numVal;
    if (ExtractNumberField(jsonStr, "emergencyOverrideButton", numVal)) outConfig.gpioMapping.emergencyOverrideButton = numVal;
    if (ExtractNumberField(jsonStr, "buzzerOutput", numVal)) outConfig.gpioMapping.buzzerOutput = numVal;

    return outConfig.Validate(outError);
}

bool ConfigurationLoader::SaveToFile(const std::string& filePath,
                                     const NodeConfiguration& config,
                                     std::string* outError) {
    std::ofstream ofs(filePath);
    if (!ofs.is_open()) {
        if (outError) *outError = "Failed to open target configuration file for writing: " + filePath;
        return false;
    }
    ofs << config.ToJson();
    return true;
}

std::string ConfigurationLoader::FindDefaultConfigFile() {
    const std::vector<std::string> candidates = {
        "config/node.json",
        "config/deployment/rpi1_coordinator.json",
        "config/deployment/rpi2_intersection_a.json",
        "../config/node.json",
        "/etc/aegis/node.json"
    };

    struct stat st;
    for (const auto& path : candidates) {
        if (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
            return path;
        }
    }
    return "";
}

} // namespace deployment
} // namespace aegis
