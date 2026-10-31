/**
 * @file LampController.cpp
 * @brief Implementation of LampController.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "traffic_node/lamp_controller/LampController.hpp"
#include "traffic_node/logger/TrafficLogger.hpp"
#include "common/utils/TimeUtils.hpp"

namespace aegis::traffic {

LampController::LampController(std::shared_ptr<RelayController> relayController)
    : relayController_(std::move(relayController)) {}

bool LampController::Initialize(uint32_t switchDelayMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    switchDelayMs_ = switchDelayMs;

    if (!relayController_ || !relayController_->IsInitialized()) {
        TrafficLogger::Instance().Error("LAMP_CTRL", "RelayController not initialized or null");
        return false;
    }

    initialized_.store(true);
    TrafficLogger::Instance().Info("LAMP_CTRL", "LampController initialized with relay switch delay: " +
                                  std::to_string(switchDelayMs_) + "ms");
    return true;
}

void LampController::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    initialized_.store(false);
    if (relayController_) {
        relayController_->TurnOffAll();
    }
}

void LampController::DisplayRed() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (relayController_) {
        relayController_->SetRed();
        if (switchDelayMs_ > 0) common::TimeUtils::SleepMs(switchDelayMs_);
    }
}

void LampController::DisplayYellow() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (relayController_) {
        relayController_->SetYellow();
        if (switchDelayMs_ > 0) common::TimeUtils::SleepMs(switchDelayMs_);
    }
}

void LampController::DisplayGreen() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (relayController_) {
        relayController_->SetGreen();
        if (switchDelayMs_ > 0) common::TimeUtils::SleepMs(switchDelayMs_);
    }
}

void LampController::DisplayAllRed() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (relayController_) {
        relayController_->SetAllRed();
        if (switchDelayMs_ > 0) common::TimeUtils::SleepMs(switchDelayMs_);
    }
}

void LampController::DisplayFlashingYellow() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (relayController_) {
        relayController_->FlashYellow();
        if (switchDelayMs_ > 0) common::TimeUtils::SleepMs(switchDelayMs_);
    }
}

void LampController::DisplayDark() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (relayController_) {
        relayController_->TurnOffAll();
    }
}

bool LampController::IsRedLit() const {
    return relayController_ ? relayController_->IsRedActive() : false;
}

bool LampController::IsYellowLit() const {
    return relayController_ ? relayController_->IsYellowActive() : false;
}

bool LampController::IsGreenLit() const {
    return relayController_ ? relayController_->IsGreenActive() : false;
}

bool LampController::IsAuxLit() const {
    return relayController_ ? relayController_->IsAuxActive() : false;
}

} // namespace aegis::traffic

