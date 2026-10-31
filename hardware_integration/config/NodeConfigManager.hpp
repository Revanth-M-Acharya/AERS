/**
 * @file NodeConfigManager.hpp
 * @brief Node Configuration Management for Raspberry Pi 4 Cluster (Roles, Ports, Heartbeat, Symbolic GPIO).
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "hardware_integration/gpio/SymbolicGpioHAL.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <mutex>

namespace aegis::hardware_integration {

enum class NodeRole {
    COORDINATOR,
    TRAFFIC_NODE
};

inline std::string NodeRoleToString(NodeRole role) {
    return (role == NodeRole::COORDINATOR) ? "COORDINATOR" : "TRAFFIC_NODE";
}

struct SymbolicPinAssignment {
    std::string symbolicName;
    int physicalPin{-1};
    PinDirection direction{PinDirection::OUTPUT};
    ActivePolarity polarity{ActivePolarity::ACTIVE_HIGH};
    std::string description;
};

struct ClusterNodeProfile {
    std::string nodeId{"INT_A"};
    NodeRole role{NodeRole::TRAFFIC_NODE};
    std::string ipAddress{"192.168.1.101"};
    uint16_t listenPort{9001};
    std::string coordinatorIp{"192.168.1.100"};
    uint16_t coordinatorPort{9000};

    uint64_t heartbeatIntervalMs{1000};
    uint64_t connectTimeoutMs{3000};
    uint64_t ackTimeoutMs{3000};
    uint64_t watchdogTimeoutMs{5000};

    std::unordered_map<std::string, SymbolicPinAssignment> pinAssignments;
};

class NodeConfigManager {
public:
    NodeConfigManager();
    ~NodeConfigManager() = default;

    /**
     * @brief Load configuration from file (key=value or JSON).
     */
    bool LoadFromFile(const std::string& filePath);

    /**
     * @brief Parse key=value string content.
     */
    bool ParseString(const std::string& configData);

    /**
     * @brief Validate configuration parameters, ranges, and pin collision absence.
     */
    bool Validate(std::vector<std::string>& outErrors) const;

    /**
     * @brief Apply configured symbolic pin mappings to a SymbolicGpioHAL instance.
     */
    bool ApplyToHAL(SymbolicGpioHAL& hal) const;

    // Getters / Setters
    ClusterNodeProfile GetProfile() const;
    void SetProfile(const ClusterNodeProfile& profile);

    void SetNodeId(const std::string& id);
    void SetRole(NodeRole role);
    void SetIpAddress(const std::string& ip);
    void SetPort(uint16_t port);
    void SetCoordinatorEndpoint(const std::string& ip, uint16_t port);
    void AssignPin(const SymbolicPinAssignment& pin);
    void ClearPinAssignments();

    // Factory Defaults
    static ClusterNodeProfile CreateDefaultCoordinatorProfile();
    static ClusterNodeProfile CreateDefaultTrafficNodeProfile(const std::string& nodeId,
                                                             const std::string& ip,
                                                             uint16_t port);

private:
    mutable std::mutex mutex_;
    ClusterNodeProfile profile_;
};

} // namespace aegis::hardware_integration
