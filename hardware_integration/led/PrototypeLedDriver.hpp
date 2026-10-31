/**
 * @file PrototypeLedDriver.hpp
 * @brief Direct GPIO LED Driver for SMART AEGIS Prototype Hardware Architecture.
 * 
 * Hardware Prototype Specifications:
 * - Coordinator (RPi 1): Zero LEDs. Pure TCP/IP communication engine.
 * - Intersection 1 (RPi 2): Red LED (BCM GPIO 5, Pin 29) + Blue LED (BCM GPIO 6, Pin 31).
 * - Intersection 2 (RPi 3): Red LED (BCM GPIO 5, Pin 29) + Blue LED (BCM GPIO 6, Pin 31).
 * - Intersection 3 (RPi 4): Blue LED only (BCM GPIO 6, Pin 31). Zero Red LED due to hardware shortage.
 * 
 * LED Operational Behaviour:
 * - Intersections 1 & 2:
 *     Normal: Blue OFF, Red OFF.
 *     Emergency Path Granted: Blue ON, Red OFF.
 *     Emergency Path Closed: Blue OFF.
 * - Intersection 3:
 *     Normal: Blue OFF.
 *     Emergency Path Granted: Blue ON.
 *     Finished: Blue OFF.
 * 
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "hardware_integration/gpio/SymbolicGpioHAL.hpp"
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace aegis::hardware_integration {

enum class LedOperationalMode {
    STANDBY,
    NORMAL,
    ALL_RED,
    FLASHING_YELLOW,
    EMERGENCY_PREEMPTION,
    EMERGENCY_PATH_GRANTED,
    EMERGENCY_PATH_CLOSED,
    SELF_TEST,
    FAULT_INTERLOCK_TRIPPED
};

// Backwards compatibility alias
using RelayOperationalMode = LedOperationalMode;

enum class TrafficApproach {
    ROAD_A, // Primary corridor approach
    ROAD_B  // Secondary / cross approach
};

struct LedChannelStatus {
    std::string channelName;
    std::string symbolicSignal;
    int physicalPin{-1};
    bool isEnergized{false};
    uint64_t lastSwitchMonoMs{0};
    uint32_t switchCount{0};
};

using RelayChannelStatus = LedChannelStatus;

struct LedSelfTestResult {
    bool passed{false};
    uint32_t channelsTested{0};
    uint32_t channelsPassed{0};
    uint64_t executionTimeMs{0};
    std::vector<std::string> diagnostics;
};

using RelaySelfTestResult = LedSelfTestResult;

class PrototypeLedDriver {
public:
    explicit PrototypeLedDriver(std::shared_ptr<SymbolicGpioHAL> hal, const std::string& nodeId = "INT_1");
    ~PrototypeLedDriver();

    // Prevent copying
    PrototypeLedDriver(const PrototypeLedDriver&) = delete;
    PrototypeLedDriver& operator=(const PrototypeLedDriver&) = delete;

    /**
     * @brief Initialize prototype LED hardware lines on designated GPIOs.
     * Intersection 1: GPIO5 (Red) + GPIO6 (Blue)
     * Intersection 2: GPIO5 (Red) + GPIO6 (Blue)
     * Intersection 3: GPIO6 (Blue only)
     * Coordinator: Zero LEDs
     */
    bool Initialize();

    /**
     * @brief Safe shutdown: de-asserts all active LED lines.
     */
    void Shutdown();

    // =========================================================================
    // Core Prototype LED Control Methods
    // =========================================================================

    /**
     * @brief Grant emergency corridor path: Blue LED ON, Red LED OFF.
     */
    bool GrantEmergencyPath();

    /**
     * @brief Close emergency corridor path: Blue LED OFF.
     */
    bool CloseEmergencyPath();

    /**
     * @brief Set Red LED directly (BCM GPIO 5, Pin 29). No-op on Intersection 3.
     */
    bool SetRedLed(bool on);

    /**
     * @brief Set Blue LED directly (BCM GPIO 6, Pin 31).
     */
    bool SetBlueLed(bool on);

    /**
     * @brief Extinguish all LEDs on this node.
     */
    bool SetAllOff();

    // =========================================================================
    // Backwards Compatible Interface Methods
    // =========================================================================
    bool SetAllRed();
    bool SetNormalPhase(TrafficApproach greenApproach = TrafficApproach::ROAD_A, bool withYellowTransition = false);
    bool SetFlashingYellow(bool lampState);
    bool SetEmergencyPreemption(TrafficApproach priorityApproach = TrafficApproach::ROAD_A, bool enableAudioVisualAlerts = true);

    LedOperationalMode GetCurrentMode() const;
    TrafficApproach GetCurrentGreenApproach() const { return TrafficApproach::ROAD_A; }
    bool IsEmergencyPreemptionActive() const;
    bool IsConflictInterlockTripped() const { return false; }
    bool ResetInterlock() { return true; }

    bool HasRedLed() const { return hasRedLed_; }
    bool HasBlueLed() const { return hasBlueLed_; }
    int GetRedPin() const { return pinRed_; }
    int GetBluePin() const { return pinBlue_; }

    const std::string& GetNodeId() const { return nodeId_; }
    void SetNodeId(const std::string& nodeId);

    std::vector<LedChannelStatus> GetChannelStatuses() const;
    LedSelfTestResult ExecuteSelfTest(uint32_t channelDwellMs = 50);

private:
    void ConfigureNodeCapabilities();

    std::shared_ptr<SymbolicGpioHAL> hal_;
    std::string nodeId_;
    bool hasRedLed_{true};
    bool hasBlueLed_{true};
    int pinRed_{5};   // BCM GPIO 5 (Pin 29)
    int pinBlue_{6};  // BCM GPIO 6 (Pin 31)

    bool redState_{false};
    bool blueState_{false};
    LedOperationalMode mode_{LedOperationalMode::STANDBY};
    mutable std::mutex mutex_;
};

} // namespace aegis::hardware_integration
