/**
 * @file PrototypeLedDriver.cpp
 * @brief Implementation of direct GPIO LED Driver for SMART AEGIS Prototype Hardware.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "hardware_integration/led/PrototypeLedDriver.hpp"
#include "common/utils/TimeUtils.hpp"
#include <thread>
#include <chrono>

namespace aegis::hardware_integration {

PrototypeLedDriver::PrototypeLedDriver(std::shared_ptr<SymbolicGpioHAL> hal, const std::string& nodeId)
    : hal_(std::move(hal)),
      nodeId_(nodeId) {
    ConfigureNodeCapabilities();
}

PrototypeLedDriver::~PrototypeLedDriver() {
    Shutdown();
}

void PrototypeLedDriver::SetNodeId(const std::string& nodeId) {
    std::lock_guard<std::mutex> lock(mutex_);
    nodeId_ = nodeId;
    ConfigureNodeCapabilities();
}

void PrototypeLedDriver::ConfigureNodeCapabilities() {
    if (nodeId_ == "INT_3" || nodeId_ == "INT3" || nodeId_ == "RPI_INT3" || nodeId_ == "NODE_C") {
        // Intersection 3: Due to hardware shortage, only ONE LED (Blue LED on BCM GPIO 6).
        hasRedLed_ = false;
        hasBlueLed_ = true;
        pinRed_ = -1;
        pinBlue_ = 6;
    } else if (nodeId_ == "RPI_COORD" || nodeId_ == "COORD" || nodeId_ == "COORDINATOR") {
        // Coordinator: Zero LEDs connected.
        hasRedLed_ = false;
        hasBlueLed_ = false;
        pinRed_ = -1;
        pinBlue_ = -1;
    } else {
        // Intersection 1 & Intersection 2: Red LED (GPIO 5) + Blue LED (GPIO 6).
        hasRedLed_ = true;
        hasBlueLed_ = true;
        pinRed_ = 5;
        pinBlue_ = 6;
    }
}

bool PrototypeLedDriver::Initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    ConfigureNodeCapabilities();

    if (!hal_) {
        return false;
    }

    if (hasBlueLed_ && !hal_->IsSignalRegistered(StandardSignals::LED_BLUE)) {
        hal_->RegisterSignal(StandardSignals::LED_BLUE, pinBlue_, PinDirection::OUTPUT, ActivePolarity::ACTIVE_HIGH, "Blue Corridor LED");
    }

    if (hasRedLed_ && !hal_->IsSignalRegistered(StandardSignals::LED_RED)) {
        hal_->RegisterSignal(StandardSignals::LED_RED, pinRed_, PinDirection::OUTPUT, ActivePolarity::ACTIVE_HIGH, "Red Stop LED");
    }

    // Register compatibility signals if not registered, avoiding pin collisions
    if (!hal_->IsSignalRegistered(StandardSignals::ROAD_A_GREEN)) {
        hal_->RegisterSignal(StandardSignals::ROAD_A_GREEN, 22, PinDirection::OUTPUT, ActivePolarity::ACTIVE_HIGH, "Road A Green");
    }
    if (!hal_->IsSignalRegistered(StandardSignals::ROAD_A_RED)) {
        hal_->RegisterSignal(StandardSignals::ROAD_A_RED, 17, PinDirection::OUTPUT, ActivePolarity::ACTIVE_HIGH, "Road A Red");
    }
    if (!hal_->IsSignalRegistered(StandardSignals::ROAD_B_RED)) {
        hal_->RegisterSignal(StandardSignals::ROAD_B_RED, 23, PinDirection::OUTPUT, ActivePolarity::ACTIVE_HIGH, "Road B Red");
    }
    if (!hal_->IsSignalRegistered(StandardSignals::ROAD_B_GREEN)) {
        hal_->RegisterSignal(StandardSignals::ROAD_B_GREEN, 25, PinDirection::OUTPUT, ActivePolarity::ACTIVE_HIGH, "Road B Green");
    }
    if (!hal_->IsSignalRegistered(StandardSignals::ROAD_A_YELLOW)) {
        hal_->RegisterSignal(StandardSignals::ROAD_A_YELLOW, 27, PinDirection::OUTPUT, ActivePolarity::ACTIVE_HIGH, "Road A Yellow");
    }
    if (!hal_->IsSignalRegistered(StandardSignals::ROAD_B_YELLOW)) {
        hal_->RegisterSignal(StandardSignals::ROAD_B_YELLOW, 24, PinDirection::OUTPUT, ActivePolarity::ACTIVE_HIGH, "Road B Yellow");
    }
    if (!hal_->IsSignalRegistered(StandardSignals::EMERGENCY_STROBE)) {
        hal_->RegisterSignal(StandardSignals::EMERGENCY_STROBE, 26, PinDirection::OUTPUT, ActivePolarity::ACTIVE_HIGH, "Emergency Strobe");
    }
    if (!hal_->IsSignalRegistered(StandardSignals::ACOUSTIC_BUZZER)) {
        hal_->RegisterSignal(StandardSignals::ACOUSTIC_BUZZER, 16, PinDirection::OUTPUT, ActivePolarity::ACTIVE_HIGH, "Acoustic Buzzer");
    }

    // Default safe initial state: ALL_RED
    if (hasBlueLed_) {
        hal_->SetSignal(StandardSignals::LED_BLUE, false);
        blueState_ = false;
    }
    if (hasRedLed_) {
        hal_->SetSignal(StandardSignals::LED_RED, true);
        redState_ = true;
    }
    hal_->SetSignal(StandardSignals::ROAD_A_GREEN, false);
    hal_->SetSignal(StandardSignals::ROAD_B_GREEN, false);
    hal_->SetSignal(StandardSignals::ROAD_A_RED, true);
    hal_->SetSignal(StandardSignals::ROAD_B_RED, true);

    mode_ = LedOperationalMode::ALL_RED;
    return true;
}

void PrototypeLedDriver::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    SetAllOff();
    mode_ = LedOperationalMode::STANDBY;
}

bool PrototypeLedDriver::GrantEmergencyPath() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!hal_) return false;

    // Blue ON, Red OFF
    if (hasBlueLed_) {
        hal_->SetSignal(StandardSignals::LED_BLUE, true);
        hal_->SetSignal(StandardSignals::ROAD_A_GREEN, true);
        blueState_ = true;
    }
    if (hasRedLed_) {
        hal_->SetSignal(StandardSignals::LED_RED, false);
        hal_->SetSignal(StandardSignals::ROAD_A_RED, false);
        redState_ = false;
    }

    mode_ = LedOperationalMode::EMERGENCY_PATH_GRANTED;
    return true;
}

bool PrototypeLedDriver::CloseEmergencyPath() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!hal_) return false;

    // Blue OFF
    if (hasBlueLed_) {
        hal_->SetSignal(StandardSignals::LED_BLUE, false);
        hal_->SetSignal(StandardSignals::ROAD_A_GREEN, false);
        blueState_ = false;
    }

    mode_ = LedOperationalMode::EMERGENCY_PATH_CLOSED;
    return true;
}

bool PrototypeLedDriver::SetAllOff() {
    if (!hal_) return false;

    if (hasBlueLed_) {
        hal_->SetSignal(StandardSignals::LED_BLUE, false);
        hal_->SetSignal(StandardSignals::ROAD_A_GREEN, false);
        blueState_ = false;
    }
    if (hasRedLed_) {
        hal_->SetSignal(StandardSignals::LED_RED, false);
        hal_->SetSignal(StandardSignals::ROAD_A_RED, false);
        redState_ = false;
    }
    return true;
}

bool PrototypeLedDriver::SetRedLed(bool on) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!hasRedLed_) {
        // Intersection 3 has no Red LED due to hardware shortage; safe no-op
        return true;
    }
    if (!hal_) return false;

    redState_ = on;
    hal_->SetSignal(StandardSignals::ROAD_A_RED, on);
    return hal_->SetSignal(StandardSignals::LED_RED, on);
}

bool PrototypeLedDriver::SetBlueLed(bool on) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!hasBlueLed_) return true;
    if (!hal_) return false;

    blueState_ = on;
    hal_->SetSignal(StandardSignals::ROAD_A_GREEN, on);
    return hal_->SetSignal(StandardSignals::LED_BLUE, on);
}

bool PrototypeLedDriver::SetAllRed() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!hal_) return false;

    // When all-red commanded: Blue OFF
    if (hasBlueLed_) {
        hal_->SetSignal(StandardSignals::LED_BLUE, false);
        hal_->SetSignal(StandardSignals::ROAD_A_GREEN, false);
        blueState_ = false;
    }
    hal_->SetSignal(StandardSignals::ROAD_A_RED, true);
    hal_->SetSignal(StandardSignals::ROAD_B_RED, true);
    hal_->SetSignal(StandardSignals::ROAD_A_GREEN, false);
    hal_->SetSignal(StandardSignals::ROAD_B_GREEN, false);

    if (hasRedLed_) {
        hal_->SetSignal(StandardSignals::LED_RED, true);
        redState_ = true;
    }
    mode_ = LedOperationalMode::ALL_RED;
    return true;
}

bool PrototypeLedDriver::SetNormalPhase(TrafficApproach greenApproach, bool withYellowTransition) {
    (void)withYellowTransition;
    std::lock_guard<std::mutex> lock(mutex_);
    if (!hal_) return false;

    if (greenApproach == TrafficApproach::ROAD_A) {
        hal_->SetSignal(StandardSignals::ROAD_A_GREEN, true);
        hal_->SetSignal(StandardSignals::ROAD_B_RED, true);
        hal_->SetSignal(StandardSignals::ROAD_B_GREEN, false);
        hal_->SetSignal(StandardSignals::ROAD_A_RED, false);
        if (hasBlueLed_) {
            hal_->SetSignal(StandardSignals::LED_BLUE, false);
            blueState_ = false;
        }
        if (hasRedLed_) {
            hal_->SetSignal(StandardSignals::LED_RED, false);
            redState_ = false;
        }
    } else {
        hal_->SetSignal(StandardSignals::ROAD_B_GREEN, true);
        hal_->SetSignal(StandardSignals::ROAD_A_RED, true);
        hal_->SetSignal(StandardSignals::ROAD_A_GREEN, false);
        hal_->SetSignal(StandardSignals::ROAD_B_RED, false);
        if (hasBlueLed_) {
            hal_->SetSignal(StandardSignals::LED_BLUE, false);
            blueState_ = false;
        }
        if (hasRedLed_) {
            hal_->SetSignal(StandardSignals::LED_RED, true);
            redState_ = true;
        }
    }
    mode_ = LedOperationalMode::NORMAL;
    return true;
}

bool PrototypeLedDriver::SetFlashingYellow(bool lampState) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (hal_) {
        hal_->SetSignal(StandardSignals::ROAD_A_YELLOW, lampState);
        hal_->SetSignal(StandardSignals::ROAD_B_YELLOW, lampState);
    }
    mode_ = LedOperationalMode::FLASHING_YELLOW;
    return true;
}

bool PrototypeLedDriver::SetEmergencyPreemption(TrafficApproach priorityApproach, bool enableAudioVisualAlerts) {
    (void)priorityApproach;
    (void)enableAudioVisualAlerts;
    std::lock_guard<std::mutex> lock(mutex_);
    if (!hal_) return false;

    // Blue ON, Red OFF
    if (hasBlueLed_) {
        hal_->SetSignal(StandardSignals::LED_BLUE, true);
        hal_->SetSignal(StandardSignals::ROAD_A_GREEN, true);
        blueState_ = true;
    }
    if (hasRedLed_) {
        hal_->SetSignal(StandardSignals::LED_RED, false);
        hal_->SetSignal(StandardSignals::ROAD_A_RED, false);
        hal_->SetSignal(StandardSignals::ROAD_B_RED, true);
        redState_ = false;
    }
    hal_->SetSignal(StandardSignals::EMERGENCY_STROBE, true);
    hal_->SetSignal(StandardSignals::ACOUSTIC_BUZZER, true);

    mode_ = LedOperationalMode::EMERGENCY_PREEMPTION;
    return true;
}

LedOperationalMode PrototypeLedDriver::GetCurrentMode() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return mode_;
}

bool PrototypeLedDriver::IsEmergencyPreemptionActive() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return blueState_;
}

std::vector<LedChannelStatus> PrototypeLedDriver::GetChannelStatuses() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<LedChannelStatus> statuses;

    if (hasBlueLed_) {
        LedChannelStatus blueSt;
        blueSt.channelName = "LED_BLUE";
        blueSt.symbolicSignal = StandardSignals::LED_BLUE;
        blueSt.physicalPin = pinBlue_;
        blueSt.isEnergized = blueState_;
        blueSt.lastSwitchMonoMs = common::TimeUtils::GetMonotonicTimeMs();
        blueSt.switchCount = 1;
        statuses.push_back(blueSt);
    }

    if (hasRedLed_) {
        LedChannelStatus redSt;
        redSt.channelName = "LED_RED";
        redSt.symbolicSignal = StandardSignals::LED_RED;
        redSt.physicalPin = pinRed_;
        redSt.isEnergized = redState_;
        redSt.lastSwitchMonoMs = common::TimeUtils::GetMonotonicTimeMs();
        redSt.switchCount = 1;
        statuses.push_back(redSt);
    }

    return statuses;
}

LedSelfTestResult PrototypeLedDriver::ExecuteSelfTest(uint32_t channelDwellMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t start = common::TimeUtils::GetMonotonicTimeMs();
    LedSelfTestResult res;
    res.passed = true;

    if (!hal_) {
        res.passed = false;
        res.diagnostics.push_back("Null GPIO HAL");
        return res;
    }

    // Test Blue LED (GPIO 6)
    if (hasBlueLed_) {
        res.channelsTested++;
        hal_->SetSignal(StandardSignals::LED_BLUE, true);
        std::this_thread::sleep_for(std::chrono::milliseconds(channelDwellMs));
        bool onOk = hal_->GetSignal(StandardSignals::LED_BLUE);
        hal_->SetSignal(StandardSignals::LED_BLUE, false);
        bool offOk = !hal_->GetSignal(StandardSignals::LED_BLUE);

        if (onOk && offOk) {
            res.channelsPassed++;
            res.diagnostics.push_back("Blue LED (GPIO 6 / Pin 31): OK");
        } else {
            res.passed = false;
            res.diagnostics.push_back("Blue LED (GPIO 6 / Pin 31): FAILED");
        }
    }

    // Test Red LED (GPIO 5) if present
    if (hasRedLed_) {
        res.channelsTested++;
        hal_->SetSignal(StandardSignals::LED_RED, true);
        std::this_thread::sleep_for(std::chrono::milliseconds(channelDwellMs));
        bool onOk = hal_->GetSignal(StandardSignals::LED_RED);
        hal_->SetSignal(StandardSignals::LED_RED, false);
        bool offOk = !hal_->GetSignal(StandardSignals::LED_RED);

        if (onOk && offOk) {
            res.channelsPassed++;
            res.diagnostics.push_back("Red LED (GPIO 5 / Pin 29): OK");
        } else {
            res.passed = false;
            res.diagnostics.push_back("Red LED (GPIO 5 / Pin 29): FAILED");
        }
    } else {
        res.diagnostics.push_back("Red LED (GPIO 5): Skipped (Not connected on Node " + nodeId_ + ")");
    }

    // Restore safe ALL_RED
    if (hasBlueLed_) {
        hal_->SetSignal(StandardSignals::LED_BLUE, false);
        blueState_ = false;
    }
    if (hasRedLed_) {
        hal_->SetSignal(StandardSignals::LED_RED, true);
        redState_ = true;
    }
    hal_->SetSignal(StandardSignals::ROAD_A_RED, true);
    hal_->SetSignal(StandardSignals::ROAD_B_RED, true);
    mode_ = LedOperationalMode::ALL_RED;

    res.executionTimeMs = common::TimeUtils::GetMonotonicTimeMs() - start;
    return res;
}

} // namespace aegis::hardware_integration
