/**
 * @file NodeConfiguration.h
 * @brief SMART AEGIS (AERS) Phase 16D - Deployment Node Configuration Model.
 *
 * Defines the complete deployment configuration schema for Raspberry Pi 4B nodes
 * running QNX Neutrino RTOS (Coordinator & Traffic Signal Intersections).
 */

#ifndef AEGIS_DEPLOYMENT_NODE_CONFIGURATION_H
#define AEGIS_DEPLOYMENT_NODE_CONFIGURATION_H

#include <string>
#include <vector>
#include <cstdint>

namespace aegis {
namespace deployment {

/**
 * @brief Functional deployment role of a SMART AEGIS node.
 */
enum class NodeRole {
    COORDINATOR,
    TRAFFIC_NODE,
    UNKNOWN
};

/**
 * @brief Convert NodeRole enum to string.
 */
inline std::string NodeRoleToString(NodeRole role) {
    switch (role) {
        case NodeRole::COORDINATOR:   return "COORDINATOR";
        case NodeRole::TRAFFIC_NODE:  return "TRAFFIC_NODE";
        default:                      return "UNKNOWN";
    }
}

/**
 * @brief Parse string to NodeRole enum.
 */
inline NodeRole StringToNodeRole(const std::string& str) {
    if (str == "COORDINATOR" || str == "Coordinator" || str == "coordinator") {
        return NodeRole::COORDINATOR;
    }
    if (str == "TRAFFIC_NODE" || str == "TrafficNode" || str == "traffic_node" ||
        str == "INTERSECTION" || str == "Intersection" || str == "intersection") {
        return NodeRole::TRAFFIC_NODE;
    }
    return NodeRole::UNKNOWN;
}

/**
 * @brief Node logging verbosity level.
 */
enum class LogLevel {
    DEBUG_LEVEL,
    INFO_LEVEL,
    WARN_LEVEL,
    ERROR_LEVEL,
    FATAL_LEVEL
};

/**
 * @brief Convert LogLevel enum to string.
 */
inline std::string LogLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG_LEVEL: return "DEBUG";
        case LogLevel::INFO_LEVEL:  return "INFO";
        case LogLevel::WARN_LEVEL:  return "WARN";
        case LogLevel::ERROR_LEVEL: return "ERROR";
        case LogLevel::FATAL_LEVEL: return "FATAL";
        default:                    return "INFO";
    }
}

/**
 * @brief Parse string to LogLevel enum.
 */
inline LogLevel StringToLogLevel(const std::string& str) {
    if (str == "DEBUG" || str == "debug") return LogLevel::DEBUG_LEVEL;
    if (str == "WARN"  || str == "warn"  || str == "WARNING" || str == "warning") return LogLevel::WARN_LEVEL;
    if (str == "ERROR" || str == "error") return LogLevel::ERROR_LEVEL;
    if (str == "FATAL" || str == "fatal") return LogLevel::FATAL_LEVEL;
    return LogLevel::INFO_LEVEL;
}

/**
 * @brief Broadcom BCM2711 GPIO pin mapping for roadside traffic signal intersection.
 */
struct GPIOPinMapping {
    uint32_t redPin{17};                  ///< Traffic Signal Red LED (Pin 17)
    uint32_t yellowPin{27};               ///< Traffic Signal Yellow LED (Pin 27)
    uint32_t greenPin{22};                ///< Traffic Signal Green LED (Pin 22)
    uint32_t nsGreenPin{23};              ///< North-South Green Phase Indicator (Pin 23)
    uint32_t ewGreenPin{24};              ///< East-West Green Phase Indicator (Pin 24)
    uint32_t pedWalkPin{25};              ///< Pedestrian Walk Signal (Pin 25)
    uint32_t pedDontWalkPin{5};           ///< Pedestrian Don't Walk Signal (Pin 5)
    uint32_t preemptStrobePin{26};        ///< Emergency Preemption Strobe Indicator (Pin 26)
    uint32_t irSensorNorthSouth{12};      ///< IR Vehicle Detection Sensor North-South (Pin 12)
    uint32_t irSensorEastWest{13};        ///< IR Vehicle Detection Sensor East-West (Pin 13)
    uint32_t emergencyOverrideButton{16}; ///< Emergency Manual Override Pushbutton (Pin 16)
    uint32_t buzzerOutput{6};             ///< Future Acoustic Warning Buzzer (Pin 6)

    std::vector<uint32_t> GetAllAssignedPins() const {
        return {redPin, yellowPin, greenPin, nsGreenPin, ewGreenPin,
                pedWalkPin, pedDontWalkPin, preemptStrobePin,
                irSensorNorthSouth, irSensorEastWest, emergencyOverrideButton, buzzerOutput};
    }
};

/**
 * @brief Unified configuration record for a SMART AEGIS node.
 */
class NodeConfiguration {
public:
    NodeConfiguration();
    ~NodeConfiguration() = default;

    // Node identity & roles
    std::string nodeId{"COORDINATOR"};
    NodeRole role{NodeRole::COORDINATOR};
    std::string intersectionName{""};

    // Network configuration
    std::string ipAddress{"127.0.0.1"};
    uint16_t port{9000};
    std::string coordinatorAddress{"127.0.0.1"};
    uint16_t coordinatorPort{9000};

    // Operational timing & retries
    uint32_t heartbeatIntervalMs{1000};
    uint32_t reconnectIntervalMs{2000};
    uint32_t connectTimeoutMs{3000};

    // Logging & system flags
    LogLevel loggingLevel{LogLevel::INFO_LEVEL};
    bool gpioEnabled{true};
    bool simulationEnabled{false};
    std::string logDirectory{"logs"};

    // Hardware GPIO mapping
    GPIOPinMapping gpioMapping{};

    // Coordinator cluster supervision expectations
    std::vector<std::string> expectedNodes{};

    // Helper queries
    bool IsCoordinator() const noexcept { return role == NodeRole::COORDINATOR; }
    bool IsTrafficNode() const noexcept { return role == NodeRole::TRAFFIC_NODE; }

    /**
     * @brief Validate configuration parameters against safety boundaries.
     * @param outError Optional string to receive validation failure details.
     * @return true if configuration is fully valid; false otherwise.
     */
    bool Validate(std::string* outError = nullptr) const;

    /**
     * @brief Serialize configuration into JSON string representation.
     */
    std::string ToJson() const;

    /**
     * @brief Factory for default Central Coordinator configuration.
     */
    static NodeConfiguration DefaultCoordinator(const std::string& id = "COORDINATOR", uint16_t port = 9000);

    /**
     * @brief Factory for default Roadside Traffic Intersection Node configuration.
     */
    static NodeConfiguration DefaultTrafficNode(const std::string& id = "INT_A",
                                               const std::string& coordIp = "127.0.0.1",
                                               uint16_t coordPort = 9000);
};

} // namespace deployment
} // namespace aegis

#endif // AEGIS_DEPLOYMENT_NODE_CONFIGURATION_H
