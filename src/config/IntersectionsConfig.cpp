/**
 * @file IntersectionsConfig.cpp
 * @brief Implementation of IntersectionsConfig parser.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "aegis/config/IntersectionsConfig.hpp"
#include "aegis/common/json_helper.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

namespace aegis::config {

std::shared_ptr<IntersectionsConfig> IntersectionsConfig::CreateDefaultCluster() {
    auto cfg = std::make_shared<IntersectionsConfig>();
    cfg->valid_ = true;

    cfg->system_.name = "SMART_AEGIS";
    cfg->system_.version = "2.5.0";
    cfg->system_.reconnectIntervalMs = 2000;
    cfg->system_.pingIntervalMs = 5000;
    cfg->system_.commandTimeoutMs = 1000;
    cfg->system_.maxRetries = 3;
    cfg->system_.coordinator = {"COORDINATOR", "10.204.179.4", false, "Central Coordinator Node"};

    // Intersection 1: 10.204.179.245:5001 (Red GPIO 5, Blue GPIO 6)
    IntersectionNodeConfig int1;
    int1.id = 1;
    int1.name = "INT_1";
    int1.ip = "10.204.179.245";
    int1.port = 5001;
    int1.gpio = {5, 6, true, true, 29, 31, 30};
    int1.description = "Intersection 1 - Northern Gateway Arterial";
    cfg->intersections_.push_back(int1);

    // Intersection 2: 10.204.179.130:5002 (Red GPIO 5, Blue GPIO 6)
    IntersectionNodeConfig int2;
    int2.id = 2;
    int2.name = "INT_2";
    int2.ip = "10.204.179.130";
    int2.port = 5002;
    int2.gpio = {5, 6, true, true, 29, 31, 30};
    int2.description = "Intersection 2 - Priority Scheduling Hub";
    cfg->intersections_.push_back(int2);

    // Intersection 3: 10.204.179.144:5003 (Blue GPIO 6 only - Hardware shortage)
    IntersectionNodeConfig int3;
    int3.id = 3;
    int3.name = "INT_3";
    int3.ip = "10.204.179.144";
    int3.port = 5003;
    int3.gpio = {-1, 6, false, true, -1, 31, 30};
    int3.description = "Intersection 3 - Hospital Gate (Hardware Shortage: Blue LED Only)";
    cfg->intersections_.push_back(int3);

    return cfg;
}

std::shared_ptr<IntersectionsConfig> IntersectionsConfig::LoadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[WARN] IntersectionsConfig: Unable to open " << filePath
                  << ", falling back to default approved cluster configuration." << std::endl;
        return CreateDefaultCluster();
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return LoadFromJsonString(buffer.str());
}

std::shared_ptr<IntersectionsConfig> IntersectionsConfig::LoadFromJsonString(const std::string& jsonContent) {
    auto cfg = std::make_shared<IntersectionsConfig>();

    // Parse system settings
    auto sysName = common::JsonHelper::ExtractString(jsonContent, "name");
    cfg->system_.name = sysName.value_or("SMART_AEGIS");

    auto sysVer = common::JsonHelper::ExtractString(jsonContent, "version");
    cfg->system_.version = sysVer.value_or("2.5.0");

    auto recMs = common::JsonHelper::ExtractInt64(jsonContent, "reconnect_interval_ms");
    cfg->system_.reconnectIntervalMs = recMs.has_value() ? static_cast<uint32_t>(*recMs) : 2000;

    auto pingMs = common::JsonHelper::ExtractInt64(jsonContent, "ping_interval_ms");
    cfg->system_.pingIntervalMs = pingMs.has_value() ? static_cast<uint32_t>(*pingMs) : 5000;

    auto cmdTimeout = common::JsonHelper::ExtractInt64(jsonContent, "command_timeout_ms");
    cfg->system_.commandTimeoutMs = cmdTimeout.has_value() ? static_cast<uint32_t>(*cmdTimeout) : 1000;

    auto maxRetries = common::JsonHelper::ExtractInt64(jsonContent, "max_retries");
    cfg->system_.maxRetries = maxRetries.has_value() ? static_cast<uint32_t>(*maxRetries) : 3;

    // Coordinator IP
    auto coordIp = common::JsonHelper::ExtractString(jsonContent, "ip");
    cfg->system_.coordinator.ip = coordIp.value_or("10.204.179.4");
    cfg->system_.coordinator.id = "COORDINATOR";
    cfg->system_.coordinator.hasLeds = false;

    // Search for intersection blocks
    // Pattern: "id": 1, ... "id": 2, ... "id": 3
    size_t pos = 0;
    while ((pos = jsonContent.find("\"id\":", pos)) != std::string::npos) {
        size_t blockStart = jsonContent.rfind('{', pos);
        size_t blockEnd = jsonContent.find('}', pos);
        if (blockStart == std::string::npos || blockEnd == std::string::npos) {
            pos += 5;
            continue;
        }

        std::string block = jsonContent.substr(blockStart, blockEnd - blockStart + 1);
        auto optId = common::JsonHelper::ExtractInt64(block, "id");
        if (optId.has_value() && *optId >= 1 && *optId <= 3) {
            int intId = static_cast<int>(*optId);

            IntersectionNodeConfig node;
            node.id = intId;
            node.name = common::JsonHelper::ExtractString(block, "name").value_or("INT_" + std::to_string(intId));
            node.ip = common::JsonHelper::ExtractString(block, "ip").value_or("127.0.0.1");

            auto portOpt = common::JsonHelper::ExtractInt64(block, "port");
            node.port = portOpt.has_value() ? static_cast<uint16_t>(*portOpt) : static_cast<uint16_t>(5000 + intId);

            // Read full intersection block including nested gpio
            size_t gpioBlockEnd = jsonContent.find("}", blockEnd + 1);
            std::string extendedBlock = (gpioBlockEnd != std::string::npos)
                                        ? jsonContent.substr(blockStart, gpioBlockEnd - blockStart + 10)
                                        : block;

            auto redPin = common::JsonHelper::ExtractInt64(extendedBlock, "red_pin");
            auto bluePin = common::JsonHelper::ExtractInt64(extendedBlock, "blue_pin");
            auto hasRed = common::JsonHelper::ExtractBool(extendedBlock, "has_red");
            auto hasBlue = common::JsonHelper::ExtractBool(extendedBlock, "has_blue");

            node.gpio.redPin = redPin.has_value() ? static_cast<int>(*redPin) : (intId == 3 ? -1 : 5);
            node.gpio.bluePin = bluePin.has_value() ? static_cast<int>(*bluePin) : 6;
            node.gpio.hasRed = hasRed.has_value() ? *hasRed : (intId != 3);
            node.gpio.hasBlue = hasBlue.has_value() ? *hasBlue : true;
            node.gpio.physicalRedPin = (node.gpio.hasRed) ? 29 : -1;
            node.gpio.physicalBluePin = 31;
            node.gpio.groundPin = 30;

            node.description = common::JsonHelper::ExtractString(extendedBlock, "description").value_or("");

            // Deduplicate if already present
            bool alreadyExists = false;
            for (auto& existing : cfg->intersections_) {
                if (existing.id == node.id) {
                    alreadyExists = true;
                    break;
                }
            }
            if (!alreadyExists) {
                cfg->intersections_.push_back(node);
            }
        }
        pos = blockEnd + 1;
    }

    // Ensure 3 nodes exist; if incomplete, fill with default approved IPs
    if (cfg->intersections_.size() < 3) {
        return CreateDefaultCluster();
    }

    cfg->valid_ = true;
    return cfg;
}

std::optional<IntersectionNodeConfig> IntersectionsConfig::GetIntersectionById(int id) const {
    for (const auto& item : intersections_) {
        if (item.id == id) {
            return item;
        }
    }
    return std::nullopt;
}

std::optional<IntersectionNodeConfig> IntersectionsConfig::GetIntersectionByName(const std::string& name) const {
    for (const auto& item : intersections_) {
        if (item.name == name) {
            return item;
        }
    }
    return std::nullopt;
}

} // namespace aegis::config
