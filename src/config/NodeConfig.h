/**
 * @file NodeConfig.h
 * @brief Node deployment configuration model and loader for Raspberry Pi deployment.
 *
 * Supports configuration loading for:
 * - Raspberry Pi 1: Central Coordinator (QNX RTOS)
 * - Raspberry Pi 2: Intersection Node A (INT_A)
 * - Raspberry Pi 3: Intersection Node B (INT_B)
 * - Raspberry Pi 4: Intersection Node C (INT_C)
 */

#ifndef AEGIS_CONFIG_NODE_CONFIG_H
#define AEGIS_CONFIG_NODE_CONFIG_H

#include <string>
#include <cstdint>
#include <vector>
#include <optional>
#include <iostream>

namespace aegis {
namespace config {

/**
 * @brief Functional role of an AERS deployment node.
 */
enum class NodeRole {
    COORDINATOR,
    INTERSECTION,
    UNKNOWN
};

/**
 * @brief Convert NodeRole enum to string representation.
 */
inline std::string NodeRoleToString(NodeRole role) {
    switch (role) {
        case NodeRole::COORDINATOR:  return "Coordinator";
        case NodeRole::INTERSECTION: return "Intersection";
        default:                     return "Unknown";
    }
}

/**
 * @brief Parse string representation to NodeRole enum.
 */
inline NodeRole StringToNodeRole(const std::string& roleStr) {
    if (roleStr == "Coordinator" || roleStr == "COORDINATOR" || roleStr == "coordinator") {
        return NodeRole::COORDINATOR;
    }
    if (roleStr == "Intersection" || roleStr == "INTERSECTION" || roleStr == "intersection") {
        return NodeRole::INTERSECTION;
    }
    return NodeRole::UNKNOWN;
}

/**
 * @brief Hardware deployment configuration parameters for a Raspberry Pi node.
 */
struct NodeConfig {
    std::string nodeId{""};                 ///< Unique identifier (e.g., "COORDINATOR", "INT_A", "INT_B", "INT_C")
    NodeRole role{NodeRole::UNKNOWN};       ///< Role: Coordinator or Intersection
    std::string ipAddress{"127.0.0.1"};     ///< Local Wi-Fi/Ethernet IP address of this Raspberry Pi
    uint16_t port{9000};                    ///< Local listening port (Coordinator bind port or node port)
    std::string coordinatorIp{"127.0.0.1"}; ///< IPv4 address of Raspberry Pi 1 (Central Coordinator)
    uint16_t coordinatorPort{9000};         ///< TCP listening port of Central Coordinator
    uint32_t heartbeatIntervalMs{1000};     ///< Keepalive heartbeat beacon interval in milliseconds

    // Operational timeouts & options
    uint32_t connectTimeoutMs{3000};        ///< Initial TCP connection timeout in ms
    uint32_t ackTimeoutMs{3000};            ///< Timeout waiting for registration ACK from Coordinator
    bool autoReconnect{true};               ///< Automatically attempt reconnect on socket drop
    std::string logFilePath{""};            ///< Optional path for logging output

    /**
     * @brief Validate that essential configuration properties are properly populated.
     */
    bool IsValid() const noexcept {
        if (nodeId.empty()) return false;
        if (role == NodeRole::UNKNOWN) return false;
        if (ipAddress.empty()) return false;
        if (coordinatorIp.empty()) return false;
        if (coordinatorPort == 0) return false;
        if (heartbeatIntervalMs == 0) return false;
        return true;
    }
};

/**
 * @brief Production JSON loader and serializer for NodeConfig.
 */
class NodeConfigLoader {
public:
    /**
     * @brief Load configuration from a JSON file on the local filesystem.
     * @param filePath Absolute or relative path to the .json configuration file.
     * @param outConfig Destination struct to populate.
     * @return true if successfully loaded and valid; false otherwise.
     */
    static bool LoadFromFile(const std::string& filePath, NodeConfig& outConfig);

    /**
     * @brief Parse configuration from a raw JSON string buffer.
     * @param jsonContent String buffer containing the JSON document.
     * @param outConfig Destination struct to populate.
     * @return true if successfully parsed and valid; false otherwise.
     */
    static bool LoadFromString(const std::string& jsonContent, NodeConfig& outConfig);

    /**
     * @brief Save configuration struct to a JSON file.
     * @param filePath Target file path.
     * @param config Configuration to serialize.
     * @return true if successfully written; false otherwise.
     */
    static bool SaveToFile(const std::string& filePath, const NodeConfig& config);

    /**
     * @brief Serialize configuration to a formatted JSON string.
     * @param config NodeConfig instance.
     * @return Formatted JSON string.
     */
    static std::string Serialize(const NodeConfig& config);
};

} // namespace config
} // namespace aegis

#endif // AEGIS_CONFIG_NODE_CONFIG_H
