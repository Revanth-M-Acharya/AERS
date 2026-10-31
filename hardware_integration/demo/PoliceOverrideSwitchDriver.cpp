/**
 * @file PoliceOverrideSwitchDriver.cpp
 * @brief Implementation of debounced police override switch driver.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "hardware_integration/demo/PoliceOverrideSwitchDriver.hpp"

namespace aegis::hardware_integration::demo {

PoliceOverrideSwitchDriver::PoliceOverrideSwitchDriver(
    std::shared_ptr<SymbolicGpioHAL> hal,
    PoliceOverrideSwitchConfig config)
    : hal_(std::move(hal)),
      config_(std::move(config)) {
}

bool PoliceOverrideSwitchDriver::Initialize() {
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
            "Police Barricade / Road Closed Switch");
    }

    isPressed_ = hal_->GetSignal(config_.symbolicSignal);
    rawState_ = isPressed_;
    return true;
}

void PoliceOverrideSwitchDriver::RegisterCallback(PoliceReportCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = std::move(callback);
}

bool PoliceOverrideSwitchDriver::Poll(uint64_t nowMonoMs) {
    PoliceReportCallback cbToCall = nullptr;
    std::string intersectionId;
    std::string reason;
    bool triggered = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!hal_) return false;

        bool currentLogical = hal_->GetSignal(config_.symbolicSignal);

        if (currentLogical != rawState_) {
            rawState_ = currentLogical;
            lastStateChangeMs_ = nowMonoMs;
        }

        if ((nowMonoMs - lastStateChangeMs_) >= config_.debounceMs) {
            if (rawState_ != isPressed_) {
                isPressed_ = rawState_;
                if (isPressed_) {
                    totalReports_++;
                    lastReportMs_ = nowMonoMs;
                    triggered = true;
                    cbToCall = callback_;
                    intersectionId = config_.defaultTargetIntersection;
                    reason = config_.defaultReason;
                }
            }
        }
    }

    if (triggered && cbToCall) {
        cbToCall(intersectionId, reason, nowMonoMs);
    }

    return triggered;
}

void PoliceOverrideSwitchDriver::SimulatePress(uint64_t nowMonoMs, const std::string& customReason) {
    PoliceReportCallback cbToCall = nullptr;
    std::string intersectionId;
    std::string reason;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        isPressed_ = true;
        rawState_ = true;
        totalReports_++;
        lastReportMs_ = nowMonoMs;
        lastStateChangeMs_ = nowMonoMs;
        cbToCall = callback_;
        intersectionId = config_.defaultTargetIntersection;
        reason = customReason.empty() ? config_.defaultReason : customReason;

        if (hal_) {
            hal_->SetSignal(config_.symbolicSignal, true);
        }
    }

    if (cbToCall) {
        cbToCall(intersectionId, reason, nowMonoMs);
    }
}

bool PoliceOverrideSwitchDriver::IsPressed() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return isPressed_;
}

uint64_t PoliceOverrideSwitchDriver::GetTotalReports() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return totalReports_;
}

uint64_t PoliceOverrideSwitchDriver::GetLastReportTimestampMs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return lastReportMs_;
}

} // namespace aegis::hardware_integration::demo
