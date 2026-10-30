/**
 * @file NodeConfiguration.cpp
 * @brief SMART AEGIS (AERS) Phase 16D - Deployment Node Configuration Model Implementation.
 */

#include "NodeConfiguration.h"
#include <sstream>
#include <set>

namespace aegis {
namespace deployment {

NodeConfiguration::NodeConfiguration() {
    expectedNodes = {"INT_A", "INT_B", "INT_C"};
}

bool NodeConfiguration::Validate(std::string* outError) const {
    if (nodeId.empty()) {
        if (outError) *outError = "Node ID cannot be empty";
        return false;
    }

    if (role == NodeRole::UNKNOWN) {
        if (outError) *outError = "Node role must be explicitly COORDINATOR or TRAFFIC_NODE";
        return false;
    }

    if (ipAddress.empty()) {
        if (outError) *outError = "IP address cannot be empty";
        return false;
    }

    if (port == 0) {
        if (outError) *outError = "Port cannot be 0";
        return false;
    }

    if (role == NodeRole::TRAFFIC_NODE) {
        if (coordinatorAddress.empty()) {
            if (outError) *outError = "Coordinator address cannot be empty for traffic node";
            return false;
        }
        if (coordinatorPort == 0) {
            if (outError) *outError = "Coordinator port cannot be 0 for traffic node";
            return false;
        }
    }

    if (heartbeatIntervalMs < 50 || heartbeatIntervalMs > 60000) {
        if (outError) *outError = "Heartbeat interval out of bounds (50 ms - 60000 ms)";
        return false;
    }

    if (reconnectIntervalMs < 50 || reconnectIntervalMs > 60000) {
        if (outError) *outError = "Reconnect interval out of bounds (50 ms - 60000 ms)";
        return false;
    }

    // GPIO Pin Conflict Validation
    if (gpioEnabled) {
        std::set<uint32_t> usedPins;
        const auto pins = gpioMapping.GetAllAssignedPins();
        for (uint32_t pin : pins) {
            if (pin > 27) {
                if (outError) *outError = "GPIO pin number " + std::to_string(pin) + " exceeds BCM2711 Bank 0 range (0-27)";
                return false;
            }
            if (usedPins.find(pin) != usedPins.end()) {
                if (outError) *outError = "Duplicate GPIO pin assignment detected on Pin " + std::to_string(pin);
                return false;
            }
            usedPins.insert(pin);
        }
    }

    return true;
}

std::string NodeConfiguration::ToJson() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"nodeId\": \"" << nodeId << "\",\n";
    oss << "  \"role\": \"" << NodeRoleToString(role) << "\",\n";
    oss << "  \"intersectionName\": \"" << intersectionName << "\",\n";
    oss << "  \"ipAddress\": \"" << ipAddress << "\",\n";
    oss << "  \"port\": " << port << ",\n";
    oss << "  \"coordinatorAddress\": \"" << coordinatorAddress << "\",\n";
    oss << "  \"coordinatorPort\": " << coordinatorPort << ",\n";
    oss << "  \"heartbeatIntervalMs\": " << heartbeatIntervalMs << ",\n";
    oss << "  \"reconnectIntervalMs\": " << reconnectIntervalMs << ",\n";
    oss << "  \"connectTimeoutMs\": " << connectTimeoutMs << ",\n";
    oss << "  \"loggingLevel\": \"" << LogLevelToString(loggingLevel) << "\",\n";
    oss << "  \"gpioEnabled\": " << (gpioEnabled ? "true" : "false") << ",\n";
    oss << "  \"simulationEnabled\": " << (simulationEnabled ? "true" : "false") << ",\n";
    oss << "  \"logDirectory\": \"" << logDirectory << "\",\n";

    // Expected Nodes
    oss << "  \"expectedNodes\": [";
    for (size_t i = 0; i < expectedNodes.size(); ++i) {
        oss << "\"" << expectedNodes[i] << "\"";
        if (i + 1 < expectedNodes.size()) oss << ", ";
    }
    oss << "],\n";

    // GPIO Mapping
    oss << "  \"gpioMapping\": {\n";
    oss << "    \"redPin\": " << gpioMapping.redPin << ",\n";
    oss << "    \"yellowPin\": " << gpioMapping.yellowPin << ",\n";
    oss << "    \"greenPin\": " << gpioMapping.greenPin << ",\n";
    oss << "    \"nsGreenPin\": " << gpioMapping.nsGreenPin << ",\n";
    oss << "    \"ewGreenPin\": " << gpioMapping.ewGreenPin << ",\n";
    oss << "    \"pedWalkPin\": " << gpioMapping.pedWalkPin << ",\n";
    oss << "    \"pedDontWalkPin\": " << gpioMapping.pedDontWalkPin << ",\n";
    oss << "    \"preemptStrobePin\": " << gpioMapping.preemptStrobePin << ",\n";
    oss << "    \"irSensorNorthSouth\": " << gpioMapping.irSensorNorthSouth << ",\n";
    oss << "    \"irSensorEastWest\": " << gpioMapping.irSensorEastWest << ",\n";
    oss << "    \"emergencyOverrideButton\": " << gpioMapping.emergencyOverrideButton << ",\n";
    oss << "    \"buzzerOutput\": " << gpioMapping.buzzerOutput << "\n";
    oss << "  }\n";
    oss << "}\n";
    return oss.str();
}

NodeConfiguration NodeConfiguration::DefaultCoordinator(const std::string& id, uint16_t port) {
    NodeConfiguration config;
    config.nodeId = id;
    config.role = NodeRole::COORDINATOR;
    config.intersectionName = "CENTRAL_COORDINATOR";
    config.ipAddress = "127.0.0.1";
    config.port = port;
    config.coordinatorAddress = "127.0.0.1";
    config.coordinatorPort = port;
    config.heartbeatIntervalMs = 1000;
    config.loggingLevel = LogLevel::INFO_LEVEL;
    config.gpioEnabled = false;
    config.simulationEnabled = true;
    config.expectedNodes = {"INT_A", "INT_B", "INT_C"};
    return config;
}

NodeConfiguration NodeConfiguration::DefaultTrafficNode(const std::string& id,
                                                        const std::string& coordIp,
                                                        uint16_t coordPort) {
    NodeConfiguration config;
    config.nodeId = id;
    config.role = NodeRole::TRAFFIC_NODE;
    config.intersectionName = "INTERSECTION_" + id;
    config.ipAddress = "127.0.0.1";
    config.port = (id == "INT_A" ? 9001 : (id == "INT_B" ? 9002 : 9003));
    config.coordinatorAddress = coordIp;
    config.coordinatorPort = coordPort;
    config.heartbeatIntervalMs = 1000;
    config.reconnectIntervalMs = 2000;
    config.loggingLevel = LogLevel::INFO_LEVEL;
    config.gpioEnabled = true;
    config.simulationEnabled = true;
    config.expectedNodes.clear();
    return config;
}

} // namespace deployment
} // namespace aegis
