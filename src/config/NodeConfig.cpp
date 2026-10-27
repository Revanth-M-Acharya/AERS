/**
 * @file NodeConfig.cpp
 * @brief Implementation of NodeConfigLoader.
 */

#include "NodeConfig.h"
#include "aegis/common/json_helper.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

namespace aegis {
namespace config {

bool NodeConfigLoader::LoadFromFile(const std::string& filePath, NodeConfig& outConfig) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return LoadFromString(buffer.str(), outConfig);
}

bool NodeConfigLoader::LoadFromString(const std::string& jsonContent, NodeConfig& outConfig) {
    if (jsonContent.empty()) {
        return false;
    }

    using common::JsonHelper;

    // 1. Mandatory Node ID
    auto nodeIdOpt = JsonHelper::ExtractString(jsonContent, "nodeId");
    if (nodeIdOpt && !nodeIdOpt->empty()) {
        outConfig.nodeId = *nodeIdOpt;
    } else {
        return false;
    }

    // 2. Role (Coordinator / Intersection)
    auto roleOpt = JsonHelper::ExtractString(jsonContent, "role");
    if (roleOpt) {
        outConfig.role = StringToNodeRole(*roleOpt);
    } else {
        return false;
    }

    // 3. Local IP Address
    auto ipOpt = JsonHelper::ExtractString(jsonContent, "ipAddress");
    if (ipOpt && !ipOpt->empty()) {
        outConfig.ipAddress = *ipOpt;
    }

    // 4. Local Port
    auto portOpt = JsonHelper::ExtractInt64(jsonContent, "port");
    if (portOpt && *portOpt > 0 && *portOpt <= 65535) {
        outConfig.port = static_cast<uint16_t>(*portOpt);
    }

    // 5. Coordinator IP Address
    auto coordIpOpt = JsonHelper::ExtractString(jsonContent, "coordinatorIp");
    if (coordIpOpt && !coordIpOpt->empty()) {
        outConfig.coordinatorIp = *coordIpOpt;
    }

    // 6. Coordinator Port
    auto coordPortOpt = JsonHelper::ExtractInt64(jsonContent, "coordinatorPort");
    if (coordPortOpt && *coordPortOpt > 0 && *coordPortOpt <= 65535) {
        outConfig.coordinatorPort = static_cast<uint16_t>(*coordPortOpt);
    }

    // 7. Heartbeat Interval (ms)
    auto hbOpt = JsonHelper::ExtractInt64(jsonContent, "heartbeatIntervalMs");
    if (hbOpt && *hbOpt > 0) {
        outConfig.heartbeatIntervalMs = static_cast<uint32_t>(*hbOpt);
    }

    // 8. Optional parameters
    auto connectTimeoutOpt = JsonHelper::ExtractInt64(jsonContent, "connectTimeoutMs");
    if (connectTimeoutOpt && *connectTimeoutOpt > 0) {
        outConfig.connectTimeoutMs = static_cast<uint32_t>(*connectTimeoutOpt);
    }

    auto ackTimeoutOpt = JsonHelper::ExtractInt64(jsonContent, "ackTimeoutMs");
    if (ackTimeoutOpt && *ackTimeoutOpt > 0) {
        outConfig.ackTimeoutMs = static_cast<uint32_t>(*ackTimeoutOpt);
    }

    auto autoReconOpt = JsonHelper::ExtractBool(jsonContent, "autoReconnect");
    if (autoReconOpt) {
        outConfig.autoReconnect = *autoReconOpt;
    }

    auto logOpt = JsonHelper::ExtractString(jsonContent, "logFilePath");
    if (logOpt) {
        outConfig.logFilePath = *logOpt;
    }

    return outConfig.IsValid();
}

bool NodeConfigLoader::SaveToFile(const std::string& filePath, const NodeConfig& config) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    file << Serialize(config);
    file.close();
    return true;
}

std::string NodeConfigLoader::Serialize(const NodeConfig& config) {
    std::ostringstream ss;
    ss << "{\n"
       << "  \"nodeId\": \"" << config.nodeId << "\",\n"
       << "  \"role\": \"" << NodeRoleToString(config.role) << "\",\n"
       << "  \"ipAddress\": \"" << config.ipAddress << "\",\n"
       << "  \"port\": " << config.port << ",\n"
       << "  \"coordinatorIp\": \"" << config.coordinatorIp << "\",\n"
       << "  \"coordinatorPort\": " << config.coordinatorPort << ",\n"
       << "  \"heartbeatIntervalMs\": " << config.heartbeatIntervalMs << ",\n"
       << "  \"connectTimeoutMs\": " << config.connectTimeoutMs << ",\n"
       << "  \"ackTimeoutMs\": " << config.ackTimeoutMs << ",\n"
       << "  \"autoReconnect\": " << (config.autoReconnect ? "true" : "false");

    if (!config.logFilePath.empty()) {
        ss << ",\n  \"logFilePath\": \"" << config.logFilePath << "\"";
    }

    ss << "\n}\n";
    return ss.str();
}

} // namespace config
} // namespace aegis
