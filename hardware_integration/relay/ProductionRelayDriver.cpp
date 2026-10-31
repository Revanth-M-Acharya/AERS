/**
 * @file ProductionRelayDriver.cpp
 * @brief Implementation of ProductionRelayDriver.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "hardware_integration/relay/ProductionRelayDriver.hpp"
#include "common/utils/TimeUtils.hpp"
#include <thread>
#include <chrono>

namespace aegis::hardware_integration {

ProductionRelayDriver::ProductionRelayDriver(std::shared_ptr<SymbolicGpioHAL> hal)
    : hal_(std::move(hal)) {
}

ProductionRelayDriver::~ProductionRelayDriver() {
    Shutdown();
}

bool ProductionRelayDriver::Initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!hal_) {
        return false;
    }

    // Default pin mapping if not previously registered
    if (!hal_->IsSignalRegistered(StandardSignals::ROAD_A_RED)) {
        hal_->RegisterSignal(StandardSignals::ROAD_A_RED, 17, PinDirection::OUTPUT, ActivePolarity::ACTIVE_LOW, "Road A Red Lamp (Relay Ch 1)");
    }
    if (!hal_->IsSignalRegistered(StandardSignals::ROAD_A_YELLOW)) {
        hal_->RegisterSignal(StandardSignals::ROAD_A_YELLOW, 27, PinDirection::OUTPUT, ActivePolarity::ACTIVE_LOW, "Road A Yellow Lamp (Relay Ch 2)");
    }
    if (!hal_->IsSignalRegistered(StandardSignals::ROAD_A_GREEN)) {
        hal_->RegisterSignal(StandardSignals::ROAD_A_GREEN, 22, PinDirection::OUTPUT, ActivePolarity::ACTIVE_LOW, "Road A Green Lamp (Relay Ch 3)");
    }
    if (!hal_->IsSignalRegistered(StandardSignals::ROAD_B_RED)) {
        hal_->RegisterSignal(StandardSignals::ROAD_B_RED, 23, PinDirection::OUTPUT, ActivePolarity::ACTIVE_HIGH, "Road B Red Lamp (Direct/Relay)");
    }
    if (!hal_->IsSignalRegistered(StandardSignals::ROAD_B_YELLOW)) {
        hal_->RegisterSignal(StandardSignals::ROAD_B_YELLOW, 24, PinDirection::OUTPUT, ActivePolarity::ACTIVE_HIGH, "Road B Yellow Lamp (Direct/Relay)");
    }
    if (!hal_->IsSignalRegistered(StandardSignals::ROAD_B_GREEN)) {
        hal_->RegisterSignal(StandardSignals::ROAD_B_GREEN, 25, PinDirection::OUTPUT, ActivePolarity::ACTIVE_LOW, "Road B Green Lamp (Relay Ch 4)");
    }
    if (!hal_->IsSignalRegistered(StandardSignals::EMERGENCY_STROBE)) {
        hal_->RegisterSignal(StandardSignals::EMERGENCY_STROBE, 26, PinDirection::OUTPUT, ActivePolarity::ACTIVE_HIGH, "Optical Emergency Strobe LED");
    }
    if (!hal_->IsSignalRegistered(StandardSignals::ACOUSTIC_BUZZER)) {
        hal_->RegisterSignal(StandardSignals::ACOUSTIC_BUZZER, 6, PinDirection::OUTPUT, ActivePolarity::ACTIVE_HIGH, "Acoustic Warning Buzzer");
    }

    // Setup channel statuses
    auto signals = hal_->GetAllSignals();
    for (const auto& sig : signals) {
        RelayChannelStatus st;
        st.channelName = sig.description;
        st.symbolicSignal = sig.symbolicName;
        st.physicalPin = sig.physicalPin;
        st.isEnergized = false;
        st.switchCount = 0;
        channelStatus_[sig.symbolicName] = st;
    }

    // Default to safe ALL_RED state
    currentMode_ = RelayOperationalMode::ALL_RED;
    modeEnterTimeMonoMs_ = common::TimeUtils::GetMonotonicTimeMs();

    hal_->SetSignal(StandardSignals::ROAD_A_RED, true);
    hal_->SetSignal(StandardSignals::ROAD_A_YELLOW, false);
    hal_->SetSignal(StandardSignals::ROAD_A_GREEN, false);
    hal_->SetSignal(StandardSignals::ROAD_B_RED, true);
    hal_->SetSignal(StandardSignals::ROAD_B_YELLOW, false);
    hal_->SetSignal(StandardSignals::ROAD_B_GREEN, false);
    hal_->SetSignal(StandardSignals::EMERGENCY_STROBE, false);
    hal_->SetSignal(StandardSignals::ACOUSTIC_BUZZER, false);

    return true;
}

void ProductionRelayDriver::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!hal_) return;

    // Fail-safe de-energization
    hal_->SetSignal(StandardSignals::ROAD_A_RED, false);
    hal_->SetSignal(StandardSignals::ROAD_A_YELLOW, false);
    hal_->SetSignal(StandardSignals::ROAD_A_GREEN, false);
    hal_->SetSignal(StandardSignals::ROAD_B_RED, false);
    hal_->SetSignal(StandardSignals::ROAD_B_YELLOW, false);
    hal_->SetSignal(StandardSignals::ROAD_B_GREEN, false);
    hal_->SetSignal(StandardSignals::EMERGENCY_STROBE, false);
    hal_->SetSignal(StandardSignals::ACOUSTIC_BUZZER, false);

    currentMode_ = RelayOperationalMode::STANDBY;
}

bool ProductionRelayDriver::AssertSafetyInterlocks(bool reqRoadAGreen, bool reqRoadBGreen) {
    if (reqRoadAGreen && reqRoadBGreen) {
        // Critical safety violation: Conflicting green requested!
        interlockTripped_ = true;
        currentMode_ = RelayOperationalMode::FAULT_INTERLOCK_TRIPPED;

        // Force ALL_RED immediately
        hal_->SetSignal(StandardSignals::ROAD_A_GREEN, false);
        hal_->SetSignal(StandardSignals::ROAD_B_GREEN, false);
        hal_->SetSignal(StandardSignals::ROAD_A_YELLOW, false);
        hal_->SetSignal(StandardSignals::ROAD_B_YELLOW, false);
        hal_->SetSignal(StandardSignals::ROAD_A_RED, true);
        hal_->SetSignal(StandardSignals::ROAD_B_RED, true);
        hal_->SetSignal(StandardSignals::EMERGENCY_STROBE, true); // Strobe alarm
        return false;
    }
    return true;
}

void ProductionRelayDriver::RecordSwitch(const std::string& signalName, bool state) {
    auto it = channelStatus_.find(signalName);
    if (it != channelStatus_.end()) {
        if (it->second.isEnergized != state) {
            it->second.isEnergized = state;
            it->second.switchCount++;
            it->second.lastSwitchMonoMs = common::TimeUtils::GetMonotonicTimeMs();
        }
    }
}

bool ProductionRelayDriver::SetAllRed() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!hal_ || interlockTripped_) return false;

    hal_->SetSignal(StandardSignals::ROAD_A_GREEN, false);
    hal_->SetSignal(StandardSignals::ROAD_B_GREEN, false);
    hal_->SetSignal(StandardSignals::ROAD_A_YELLOW, false);
    hal_->SetSignal(StandardSignals::ROAD_B_YELLOW, false);
    hal_->SetSignal(StandardSignals::ROAD_A_RED, true);
    hal_->SetSignal(StandardSignals::ROAD_B_RED, true);
    hal_->SetSignal(StandardSignals::EMERGENCY_STROBE, false);
    hal_->SetSignal(StandardSignals::ACOUSTIC_BUZZER, false);

    RecordSwitch(StandardSignals::ROAD_A_RED, true);
    RecordSwitch(StandardSignals::ROAD_B_RED, true);
    RecordSwitch(StandardSignals::ROAD_A_GREEN, false);
    RecordSwitch(StandardSignals::ROAD_B_GREEN, false);

    currentMode_ = RelayOperationalMode::ALL_RED;
    modeEnterTimeMonoMs_ = common::TimeUtils::GetMonotonicTimeMs();
    return true;
}

bool ProductionRelayDriver::SetNormalPhase(TrafficApproach greenApproach, bool withYellowTransition) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!hal_ || interlockTripped_) return false;

    bool reqA = (greenApproach == TrafficApproach::ROAD_A);
    bool reqB = (greenApproach == TrafficApproach::ROAD_B);

    if (!AssertSafetyInterlocks(reqA, reqB)) {
        return false;
    }

    if (withYellowTransition && (currentMode_ == RelayOperationalMode::NORMAL)) {
        // Yellow clearance transition
        if (activeApproach_ == TrafficApproach::ROAD_A && greenApproach == TrafficApproach::ROAD_B) {
            hal_->SetSignal(StandardSignals::ROAD_A_GREEN, false);
            hal_->SetSignal(StandardSignals::ROAD_A_YELLOW, true);
            RecordSwitch(StandardSignals::ROAD_A_YELLOW, true);
        } else if (activeApproach_ == TrafficApproach::ROAD_B && greenApproach == TrafficApproach::ROAD_A) {
            hal_->SetSignal(StandardSignals::ROAD_B_GREEN, false);
            hal_->SetSignal(StandardSignals::ROAD_B_YELLOW, true);
            RecordSwitch(StandardSignals::ROAD_B_YELLOW, true);
        }
    }

    if (reqA) {
        hal_->SetSignal(StandardSignals::ROAD_B_GREEN, false);
        hal_->SetSignal(StandardSignals::ROAD_B_YELLOW, false);
        hal_->SetSignal(StandardSignals::ROAD_B_RED, true);

        hal_->SetSignal(StandardSignals::ROAD_A_RED, false);
        hal_->SetSignal(StandardSignals::ROAD_A_YELLOW, false);
        hal_->SetSignal(StandardSignals::ROAD_A_GREEN, true);

        RecordSwitch(StandardSignals::ROAD_A_GREEN, true);
        RecordSwitch(StandardSignals::ROAD_B_RED, true);
    } else {
        hal_->SetSignal(StandardSignals::ROAD_A_GREEN, false);
        hal_->SetSignal(StandardSignals::ROAD_A_YELLOW, false);
        hal_->SetSignal(StandardSignals::ROAD_A_RED, true);

        hal_->SetSignal(StandardSignals::ROAD_B_RED, false);
        hal_->SetSignal(StandardSignals::ROAD_B_YELLOW, false);
        hal_->SetSignal(StandardSignals::ROAD_B_GREEN, true);

        RecordSwitch(StandardSignals::ROAD_B_GREEN, true);
        RecordSwitch(StandardSignals::ROAD_A_RED, true);
    }

    hal_->SetSignal(StandardSignals::EMERGENCY_STROBE, false);
    hal_->SetSignal(StandardSignals::ACOUSTIC_BUZZER, false);

    currentMode_ = RelayOperationalMode::NORMAL;
    activeApproach_ = greenApproach;
    modeEnterTimeMonoMs_ = common::TimeUtils::GetMonotonicTimeMs();
    return true;
}

bool ProductionRelayDriver::SetFlashingYellow(bool lampState) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!hal_ || interlockTripped_) return false;

    hal_->SetSignal(StandardSignals::ROAD_A_RED, false);
    hal_->SetSignal(StandardSignals::ROAD_A_GREEN, false);
    hal_->SetSignal(StandardSignals::ROAD_B_RED, false);
    hal_->SetSignal(StandardSignals::ROAD_B_GREEN, false);

    hal_->SetSignal(StandardSignals::ROAD_A_YELLOW, lampState);
    hal_->SetSignal(StandardSignals::ROAD_B_YELLOW, lampState);

    RecordSwitch(StandardSignals::ROAD_A_YELLOW, lampState);
    RecordSwitch(StandardSignals::ROAD_B_YELLOW, lampState);

    currentMode_ = RelayOperationalMode::FLASHING_YELLOW;
    modeEnterTimeMonoMs_ = common::TimeUtils::GetMonotonicTimeMs();
    return true;
}

bool ProductionRelayDriver::SetEmergencyPreemption(TrafficApproach priorityApproach, bool enableAudioVisualAlerts) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!hal_ || interlockTripped_) return false;

    bool reqA = (priorityApproach == TrafficApproach::ROAD_A);
    bool reqB = (priorityApproach == TrafficApproach::ROAD_B);

    if (!AssertSafetyInterlocks(reqA, reqB)) {
        return false;
    }

    if (reqA) {
        // Road B locked in RED
        hal_->SetSignal(StandardSignals::ROAD_B_GREEN, false);
        hal_->SetSignal(StandardSignals::ROAD_B_YELLOW, false);
        hal_->SetSignal(StandardSignals::ROAD_B_RED, true);

        // Road A preempted to GREEN
        hal_->SetSignal(StandardSignals::ROAD_A_RED, false);
        hal_->SetSignal(StandardSignals::ROAD_A_YELLOW, false);
        hal_->SetSignal(StandardSignals::ROAD_A_GREEN, true);
    } else {
        // Road A locked in RED
        hal_->SetSignal(StandardSignals::ROAD_A_GREEN, false);
        hal_->SetSignal(StandardSignals::ROAD_A_YELLOW, false);
        hal_->SetSignal(StandardSignals::ROAD_A_RED, true);

        // Road B preempted to GREEN
        hal_->SetSignal(StandardSignals::ROAD_B_RED, false);
        hal_->SetSignal(StandardSignals::ROAD_B_YELLOW, false);
        hal_->SetSignal(StandardSignals::ROAD_B_GREEN, true);
    }

    if (enableAudioVisualAlerts) {
        hal_->SetSignal(StandardSignals::EMERGENCY_STROBE, true);
        hal_->SetSignal(StandardSignals::ACOUSTIC_BUZZER, true);
    }

    currentMode_ = RelayOperationalMode::EMERGENCY_PREEMPTION;
    activeApproach_ = priorityApproach;
    modeEnterTimeMonoMs_ = common::TimeUtils::GetMonotonicTimeMs();
    return true;
}

RelaySelfTestResult ProductionRelayDriver::ExecuteSelfTest(uint32_t channelDwellMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    RelaySelfTestResult result;
    uint64_t startMono = common::TimeUtils::GetMonotonicTimeMs();

    if (!hal_) {
        result.diagnostics.push_back("FAIL: Null HAL driver");
        return result;
    }

    currentMode_ = RelayOperationalMode::SELF_TEST;
    result.diagnostics.push_back("Starting relay hardware self-test...");

    std::vector<std::string> testSequence = {
        StandardSignals::ROAD_A_RED,
        StandardSignals::ROAD_A_YELLOW,
        StandardSignals::ROAD_A_GREEN,
        StandardSignals::ROAD_B_RED,
        StandardSignals::ROAD_B_YELLOW,
        StandardSignals::ROAD_B_GREEN,
        StandardSignals::EMERGENCY_STROBE,
        StandardSignals::ACOUSTIC_BUZZER
    };

    result.channelsTested = static_cast<uint32_t>(testSequence.size());

    // Turn all off initially
    for (const auto& sig : testSequence) {
        hal_->SetSignal(sig, false);
    }

    // Step each channel individually
    for (const auto& sig : testSequence) {
        if (!hal_->IsSignalRegistered(sig)) {
            result.diagnostics.push_back("SKIP: " + sig + " not registered");
            continue;
        }

        hal_->SetSignal(sig, true);
        if (channelDwellMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(channelDwellMs));
        }
        bool readBack = hal_->GetSignal(sig);

        if (readBack) {
            result.channelsPassed++;
            result.diagnostics.push_back("PASS: " + sig + " energized and read back OK");
        } else {
            result.diagnostics.push_back("FAIL: " + sig + " readback mismatch");
        }

        hal_->SetSignal(sig, false);
    }

    // Restore safe ALL_RED
    hal_->SetSignal(StandardSignals::ROAD_A_RED, true);
    hal_->SetSignal(StandardSignals::ROAD_B_RED, true);
    currentMode_ = RelayOperationalMode::ALL_RED;

    result.executionTimeMs = common::TimeUtils::GetMonotonicTimeMs() - startMono;
    result.passed = (result.channelsPassed == result.channelsTested);
    result.diagnostics.push_back(result.passed ? "SUCCESS: All relay channels passed self-test"
                                               : "WARNING: One or more channels failed self-test");
    return result;
}

RelayOperationalMode ProductionRelayDriver::GetCurrentMode() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return currentMode_;
}

TrafficApproach ProductionRelayDriver::GetCurrentGreenApproach() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return activeApproach_;
}

bool ProductionRelayDriver::IsEmergencyPreemptionActive() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return (currentMode_ == RelayOperationalMode::EMERGENCY_PREEMPTION);
}

bool ProductionRelayDriver::IsConflictInterlockTripped() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return interlockTripped_;
}

std::vector<RelayChannelStatus> ProductionRelayDriver::GetChannelStatuses() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RelayChannelStatus> res;
    res.reserve(channelStatus_.size());
    for (const auto& [name, st] : channelStatus_) {
        res.push_back(st);
    }
    return res;
}

bool ProductionRelayDriver::ResetInterlock() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!interlockTripped_) return true;

    interlockTripped_ = false;
    currentMode_ = RelayOperationalMode::ALL_RED;

    if (hal_) {
        hal_->SetSignal(StandardSignals::ROAD_A_RED, true);
        hal_->SetSignal(StandardSignals::ROAD_B_RED, true);
        hal_->SetSignal(StandardSignals::ROAD_A_GREEN, false);
        hal_->SetSignal(StandardSignals::ROAD_B_GREEN, false);
        hal_->SetSignal(StandardSignals::EMERGENCY_STROBE, false);
    }
    return true;
}

} // namespace aegis::hardware_integration
