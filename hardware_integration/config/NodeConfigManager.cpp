/**
 * @file NodeConfigManager.cpp
 * @brief Implementation of NodeConfigManager.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "hardware_integration/config/NodeConfigManager.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_set>

namespace aegis::hardware_integration {

static inline std::string Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

NodeConfigManager::NodeConfigManager() {
    profile_ = CreateDefaultTrafficNodeProfile("INT_A", "192.168.1.101", 9001);
}

ClusterNodeProfile NodeConfigManager::CreateDefaultCoordinatorProfile() {
    ClusterNodeProfile p;
    p.nodeId = "RPI_COORD";
    p.role = NodeRole::COORDINATOR;
    p.ipAddress = "192.168.1.100";
    p.listenPort = 9000;
    p.coordinatorIp = "192.168.1.100";
    p.coordinatorPort = 9000;
    p.heartbeatIntervalMs = 1000;
    p.connectTimeoutMs = 3000;
    p.ackTimeoutMs = 3000;
    p.watchdogTimeoutMs = 5000;
    return p;
}

ClusterNodeProfile NodeConfigManager::CreateDefaultTrafficNodeProfile(const std::string& nodeId,
                                                                     const std::string& ip,
                                                                     uint16_t port) {
    ClusterNodeProfile p;
    p.nodeId = nodeId;
    p.role = NodeRole::TRAFFIC_NODE;
    p.ipAddress = ip;
    p.listenPort = port;
    p.coordinatorIp = "192.168.1.100";
    p.coordinatorPort = 9000;
    p.heartbeatIntervalMs = 1000;
    p.connectTimeoutMs = 3000;
    p.ackTimeoutMs = 3000;
    p.watchdogTimeoutMs = 5000;

    // Standard BCM2711 GPIO assignments
    p.pinAssignments[StandardSignals::ROAD_A_RED] = {StandardSignals::ROAD_A_RED, 17, PinDirection::OUTPUT, ActivePolarity::ACTIVE_LOW, "Relay Ch 1"};
    p.pinAssignments[StandardSignals::ROAD_A_YELLOW] = {StandardSignals::ROAD_A_YELLOW, 27, PinDirection::OUTPUT, ActivePolarity::ACTIVE_LOW, "Relay Ch 2"};
    p.pinAssignments[StandardSignals::ROAD_A_GREEN] = {StandardSignals::ROAD_A_GREEN, 22, PinDirection::OUTPUT, ActivePolarity::ACTIVE_LOW, "Relay Ch 3"};
    p.pinAssignments[StandardSignals::ROAD_B_RED] = {StandardSignals::ROAD_B_RED, 23, PinDirection::OUTPUT, ActivePolarity::ACTIVE_HIGH, "Road B Red"};
    p.pinAssignments[StandardSignals::ROAD_B_YELLOW] = {StandardSignals::ROAD_B_YELLOW, 24, PinDirection::OUTPUT, ActivePolarity::ACTIVE_HIGH, "Road B Yellow"};
    p.pinAssignments[StandardSignals::ROAD_B_GREEN] = {StandardSignals::ROAD_B_GREEN, 25, PinDirection::OUTPUT, ActivePolarity::ACTIVE_LOW, "Relay Ch 4"};
    p.pinAssignments[StandardSignals::PEDESTRIAN_WALK] = {StandardSignals::PEDESTRIAN_WALK, 5, PinDirection::OUTPUT, ActivePolarity::ACTIVE_HIGH, "Pedestrian Walk"};
    p.pinAssignments[StandardSignals::EMERGENCY_STROBE] = {StandardSignals::EMERGENCY_STROBE, 26, PinDirection::OUTPUT, ActivePolarity::ACTIVE_HIGH, "Strobe LED"};
    p.pinAssignments[StandardSignals::ACOUSTIC_BUZZER] = {StandardSignals::ACOUSTIC_BUZZER, 6, PinDirection::OUTPUT, ActivePolarity::ACTIVE_HIGH, "Acoustic Buzzer"};
    p.pinAssignments[StandardSignals::APPROACH_IR_SENSOR] = {StandardSignals::APPROACH_IR_SENSOR, 12, PinDirection::INPUT, ActivePolarity::ACTIVE_LOW, "Approach IR Sensor"};
    p.pinAssignments[StandardSignals::DEPARTURE_IR_SENSOR] = {StandardSignals::DEPARTURE_IR_SENSOR, 13, PinDirection::INPUT, ActivePolarity::ACTIVE_LOW, "Departure IR Sensor"};

    return p;
}

bool NodeConfigManager::LoadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return ParseString(buffer.str());
}

bool NodeConfigManager::ParseString(const std::string& configData) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::istringstream stream(configData);
    std::string line;

    while (std::getline(stream, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = Trim(line.substr(0, eqPos));
        std::string val = Trim(line.substr(eqPos + 1));

        if (key == "nodeId" || key == "node_id") {
            profile_.nodeId = val;
        } else if (key == "role") {
            profile_.role = (val == "COORDINATOR" || val == "Coordinator") ? NodeRole::COORDINATOR : NodeRole::TRAFFIC_NODE;
        } else if (key == "ipAddress" || key == "ip" || key == "ip_address") {
            profile_.ipAddress = val;
        } else if (key == "listenPort" || key == "port") {
            profile_.listenPort = static_cast<uint16_t>(std::stoi(val));
        } else if (key == "coordinatorIp" || key == "coordinator_ip") {
            profile_.coordinatorIp = val;
        } else if (key == "coordinatorPort" || key == "coordinator_port") {
            profile_.coordinatorPort = static_cast<uint16_t>(std::stoi(val));
        } else if (key == "heartbeatIntervalMs" || key == "heartbeat_interval_ms") {
            profile_.heartbeatIntervalMs = std::stoull(val);
        } else if (key == "connectTimeoutMs") {
            profile_.connectTimeoutMs = std::stoull(val);
        } else if (key == "ackTimeoutMs") {
            profile_.ackTimeoutMs = std::stoull(val);
        } else if (key == "watchdogTimeoutMs") {
            profile_.watchdogTimeoutMs = std::stoull(val);
        } else if (key.rfind("pin.", 0) == 0) {
            // pin.ROAD_A_RED = 17,OUTPUT,ACTIVE_LOW,Description
            std::string sigName = key.substr(4);
            std::stringstream ss(val);
            std::string pinStr, dirStr, polStr, descStr;

            std::getline(ss, pinStr, ',');
            std::getline(ss, dirStr, ',');
            std::getline(ss, polStr, ',');
            std::getline(ss, descStr);

            SymbolicPinAssignment assign;
            assign.symbolicName = sigName;
            assign.physicalPin = std::stoi(Trim(pinStr));
            assign.direction = (Trim(dirStr) == "INPUT") ? PinDirection::INPUT : PinDirection::OUTPUT;
            assign.polarity = (Trim(polStr) == "ACTIVE_LOW") ? ActivePolarity::ACTIVE_LOW : ActivePolarity::ACTIVE_HIGH;
            assign.description = Trim(descStr);

            profile_.pinAssignments[sigName] = assign;
        }
    }
    return true;
}

bool NodeConfigManager::Validate(std::vector<std::string>& outErrors) const {
    std::lock_guard<std::mutex> lock(mutex_);
    bool valid = true;

    if (profile_.nodeId.empty()) {
        outErrors.push_back("nodeId cannot be empty");
        valid = false;
    }

    if (profile_.ipAddress.empty()) {
        outErrors.push_back("ipAddress cannot be empty");
        valid = false;
    }

    if (profile_.listenPort == 0) {
        outErrors.push_back("listenPort must be > 0");
        valid = false;
    }

    if (profile_.heartbeatIntervalMs < 50 || profile_.heartbeatIntervalMs > 60000) {
        outErrors.push_back("heartbeatIntervalMs out of valid range (50ms - 60000ms)");
        valid = false;
    }

    // Pin collision check
    std::unordered_set<int> assignedPins;
    for (const auto& [name, assign] : profile_.pinAssignments) {
        if (assign.physicalPin < 0 || assign.physicalPin > 27) {
            outErrors.push_back("Pin assignment for " + name + " has invalid BCM pin " + std::to_string(assign.physicalPin));
            valid = false;
        }
        if (assignedPins.find(assign.physicalPin) != assignedPins.end()) {
            outErrors.push_back("Pin collision: Physical pin " + std::to_string(assign.physicalPin) + " assigned multiple times");
            valid = false;
        }
        assignedPins.insert(assign.physicalPin);
    }

    return valid;
}

bool NodeConfigManager::ApplyToHAL(SymbolicGpioHAL& hal) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [name, assign] : profile_.pinAssignments) {
        if (!hal.RegisterSignal(assign.symbolicName, assign.physicalPin, assign.direction, assign.polarity, assign.description)) {
            // Already registered or collision
        }
    }
    return true;
}

ClusterNodeProfile NodeConfigManager::GetProfile() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return profile_;
}

void NodeConfigManager::SetProfile(const ClusterNodeProfile& profile) {
    std::lock_guard<std::mutex> lock(mutex_);
    profile_ = profile;
}

void NodeConfigManager::SetNodeId(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    profile_.nodeId = id;
}

void NodeConfigManager::SetRole(NodeRole role) {
    std::lock_guard<std::mutex> lock(mutex_);
    profile_.role = role;
}

void NodeConfigManager::SetIpAddress(const std::string& ip) {
    std::lock_guard<std::mutex> lock(mutex_);
    profile_.ipAddress = ip;
}

void NodeConfigManager::SetPort(uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex_);
    profile_.listenPort = port;
}

void NodeConfigManager::SetCoordinatorEndpoint(const std::string& ip, uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex_);
    profile_.coordinatorIp = ip;
    profile_.coordinatorPort = port;
}

void NodeConfigManager::AssignPin(const SymbolicPinAssignment& pin) {
    std::lock_guard<std::mutex> lock(mutex_);
    profile_.pinAssignments[pin.symbolicName] = pin;
}

void NodeConfigManager::ClearPinAssignments() {
    std::lock_guard<std::mutex> lock(mutex_);
    profile_.pinAssignments.clear();
}

} // namespace aegis::hardware_integration
