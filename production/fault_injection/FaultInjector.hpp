/**
 * @file FaultInjector.hpp
 * @brief Chaos engineering and fault injection engine for SMART AEGIS.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <cstdint>

namespace aegis::production {

enum class FaultType {
    PACKET_LOSS,
    PACKET_DELAY,
    NODE_FAILURE,
    CAMERA_FAILURE,
    COORDINATOR_RESTART,
    HOSPITAL_OFFLINE,
    TRAFFIC_NODE_OFFLINE,
    COMMUNICATION_INTERRUPTION,
    CPU_OVERLOAD,
    MEMORY_PRESSURE
};

inline std::string FaultTypeToString(FaultType type) {
    switch (type) {
        case FaultType::PACKET_LOSS:                return "PACKET_LOSS";
        case FaultType::PACKET_DELAY:               return "PACKET_DELAY";
        case FaultType::NODE_FAILURE:               return "NODE_FAILURE";
        case FaultType::CAMERA_FAILURE:             return "CAMERA_FAILURE";
        case FaultType::COORDINATOR_RESTART:        return "COORDINATOR_RESTART";
        case FaultType::HOSPITAL_OFFLINE:           return "HOSPITAL_OFFLINE";
        case FaultType::TRAFFIC_NODE_OFFLINE:       return "TRAFFIC_NODE_OFFLINE";
        case FaultType::COMMUNICATION_INTERRUPTION: return "COMMUNICATION_INTERRUPTION";
        case FaultType::CPU_OVERLOAD:               return "CPU_OVERLOAD";
        case FaultType::MEMORY_PRESSURE:            return "MEMORY_PRESSURE";
        default:                                    return "UNKNOWN";
    }
}

struct FaultDescriptor {
    std::string faultId;
    FaultType type;
    std::string targetComponentId;
    double probability{1.0};
    uint32_t delayMs{0};
    bool isActive{true};
    uint64_t triggerCount{0};
};

class FaultInjector {
public:
    FaultInjector();
    ~FaultInjector() = default;

    /**
     * @brief Arm a new fault injection rule.
     */
    void ArmFault(const std::string& faultId, FaultType type, const std::string& target = "", double probability = 1.0, uint32_t delayMs = 0);

    /**
     * @brief Disarm / remove a fault rule.
     */
    void DisarmFault(const std::string& faultId);
    void DisarmAll();

    /**
     * @brief Evaluate whether a packet or event should be dropped.
     */
    bool ShouldDropPacket(const std::string& target = "");

    /**
     * @brief Get injection delay for a packet/event in ms.
     */
    uint32_t GetInjectedDelayMs(const std::string& target = "");

    /**
     * @brief Check whether a specific node is under an active offline fault.
     */
    bool IsNodeFaulted(const std::string& nodeId) const;

    /**
     * @brief Simulate synthetic CPU overload.
     */
    void SimulateCpuOverload(uint32_t durationMs = 10);

    /**
     * @brief Simulate synthetic memory pressure.
     */
    void SimulateMemoryPressure(size_t pressureBytes = 1024 * 1024);

    size_t GetActiveFaultCount() const;
    uint64_t GetTotalTriggeredCount() const;

    std::string ToJson() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, FaultDescriptor> faults_;
    uint64_t totalTriggeredCount_{0};
    std::vector<uint8_t> pressureBuffer_;
};

} // namespace aegis::production

