/**
 * @file EmergencySwitchDriver.cpp
 * @brief Implementation of debounced emergency switch driver.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "hardware_integration/demo/EmergencySwitchDriver.hpp"

namespace aegis::hardware_integration::demo {

EmergencySwitchDriver::EmergencySwitchDriver(
    std::shared_ptr<SymbolicGpioHAL> hal,
    EmergencySwitchConfig config)
    : hal_(std::move(hal)),
      config_(std::move(config)) {
}

bool EmergencySwitchDriver::Initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!hal_) {
        return false;
    }

    if (!hal_->IsSignalRegistered(config_.symbolicSignal)) {
        hal_->RegisterSignal(
            config_.symbolicSignal,
            config_.physicalPin,
            PinDirection::INPUT,
            config_.polarity,
            "Emergency Mission Trigger Switch");
    }

    // Read initial steady state
    isPressed_ = hal_->GetSignal(config_.symbolicSignal);
    rawState_ = isPressed_;
    return true;
}

void EmergencySwitchDriver::RegisterCallback(TriggerCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = std::move(callback);
}

bool EmergencySwitchDriver::Poll(uint64_t nowMonoMs) {
    TriggerCallback cbToCall = nullptr;
    bool triggered = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!hal_) return false;

        bool currentLogical = hal_->GetSignal(config_.symbolicSignal);

        if (currentLogical != rawState_) {
            rawState_ = currentLogical;
            lastStateChangeMs_ = nowMonoMs;
        }

        // Debounce validation
        if ((nowMonoMs - lastStateChangeMs_) >= config_.debounceMs) {
            if (rawState_ != isPressed_) {
                isPressed_ = rawState_;
                if (isPressed_) {
                    // Transition: Released -> Pressed
                    totalTriggers_++;
                    lastTriggerMs_ = nowMonoMs;
                    triggered = true;
                    cbToCall = callback_;
                }
            }
        }
    }

    if (triggered && cbToCall) {
        cbToCall(nowMonoMs);
    }

    return triggered;
}

void EmergencySwitchDriver::SimulatePress(uint64_t nowMonoMs) {
    TriggerCallback cbToCall = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        isPressed_ = true;
        rawState_ = true;
        totalTriggers_++;
        lastTriggerMs_ = nowMonoMs;
        lastStateChangeMs_ = nowMonoMs;
        cbToCall = callback_;
        if (hal_) {
            // Set signal to active logical state
            hal_->SetSignal(config_.symbolicSignal, true);
        }
    }

    if (cbToCall) {
        cbToCall(nowMonoMs);
    }
}

bool EmergencySwitchDriver::IsPressed() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return isPressed_;
}

uint64_t EmergencySwitchDriver::GetTotalTriggers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return totalTriggers_;
}

uint64_t EmergencySwitchDriver::GetLastTriggerTimestampMs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return lastTriggerMs_;
}

} // namespace aegis::hardware_integration::demo
