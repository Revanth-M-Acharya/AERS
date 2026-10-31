/**
 * @file ProductionRelayDriver.hpp
 * @brief Production-grade Relay Driver for 4-Channel Traffic Signal Actuation on Raspberry Pi 4.
 * Supports Normal Operation, All-Red, Flashing Yellow, Emergency Preemption, and Self-Test.
 * Guarantees zero conflicting green indications via hardware/software interlocks.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "hardware_integration/gpio/SymbolicGpioHAL.hpp"
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace aegis::hardware_integration {

enum class RelayOperationalMode {
    STANDBY,
    NORMAL,
    ALL_RED,
    FLASHING_YELLOW,
    EMERGENCY_PREEMPTION,
    SELF_TEST,
    FAULT_INTERLOCK_TRIPPED
};

enum class TrafficApproach {
    ROAD_A, // Primary arterial
    ROAD_B  // Secondary / cross street
};

struct RelayChannelStatus {
    std::string channelName;
    std::string symbolicSignal;
    int physicalPin{-1};
    bool isEnergized{false};
    uint64_t lastSwitchMonoMs{0};
    uint32_t switchCount{0};
};

struct RelaySelfTestResult {
    bool passed{false};
    uint32_t channelsTested{0};
    uint32_t channelsPassed{0};
    uint64_t executionTimeMs{0};
    std::vector<std::string> diagnostics;
};

class ProductionRelayDriver {
public:
    explicit ProductionRelayDriver(std::shared_ptr<SymbolicGpioHAL> hal);
    ~ProductionRelayDriver();

    // Prevent copying
    ProductionRelayDriver(const ProductionRelayDriver&) = delete;
    ProductionRelayDriver& operator=(const ProductionRelayDriver&) = delete;

    /**
     * @brief Initialize relay driver and register standard signals if not already present.
     */
    bool Initialize();

    /**
     * @brief Safe shutdown: de-assert all relay channels.
     */
    void Shutdown();

    // Operational Modes
    bool SetAllRed();
    bool SetNormalPhase(TrafficApproach greenApproach, bool withYellowTransition = false);
    bool SetFlashingYellow(bool lampState);
    bool SetEmergencyPreemption(TrafficApproach priorityApproach, bool enableAudioVisualAlerts = true);

    /**
     * @brief Execute automated hardware self-test cycling through all channels.
     */
    RelaySelfTestResult ExecuteSelfTest(uint32_t channelDwellMs = 100);

    // Queries
    RelayOperationalMode GetCurrentMode() const;
    TrafficApproach GetCurrentGreenApproach() const;
    bool IsEmergencyPreemptionActive() const;
    bool IsConflictInterlockTripped() const;
    std::vector<RelayChannelStatus> GetChannelStatuses() const;

    /**
     * @brief Clear interlock trip after safety audit.
     */
    bool ResetInterlock();

private:
    bool AssertSafetyInterlocks(bool reqRoadAGreen, bool reqRoadBGreen);
    void RecordSwitch(const std::string& signalName, bool state);

    std::shared_ptr<SymbolicGpioHAL> hal_;
    mutable std::mutex mutex_;
    RelayOperationalMode currentMode_{RelayOperationalMode::STANDBY};
    TrafficApproach activeApproach_{TrafficApproach::ROAD_A};
    bool interlockTripped_{false};
    uint64_t modeEnterTimeMonoMs_{0};

    std::unordered_map<std::string, RelayChannelStatus> channelStatus_;
};

} // namespace aegis::hardware_integration
