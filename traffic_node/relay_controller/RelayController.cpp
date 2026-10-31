/**
 * @file RelayController.cpp
 * @brief Implementation of 4-Channel Traffic Signal Relay Controller HAL.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "traffic_node/relay_controller/RelayController.hpp"
#include "traffic_node/gpio/MockGpioDriver.hpp"
#include "traffic_node/logger/TrafficLogger.hpp"

namespace aegis::traffic {

RelayController::RelayController()
    : driver_(std::make_shared<MockGpioDriver>()) {}

RelayController::RelayController(std::shared_ptr<IGpioDriver> gpioDriver)
    : driver_(gpioDriver ? std::move(gpioDriver) : std::make_shared<MockGpioDriver>()) {}

RelayController::~RelayController() {
    Shutdown();
}

void RelayController::SetGpioDriver(std::shared_ptr<IGpioDriver> driver) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (driver) {
        driver_ = std::move(driver);
    }
}

bool RelayController::Initialize(int redPin, int yellowPin, int greenPin, int auxPin, bool activeLow) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pinRed_ = redPin;
    pinYellow_ = yellowPin;
    pinGreen_ = greenPin;
    pinAux_ = auxPin;
    activeLow_ = activeLow;

    if (!driver_->Initialize()) {
        TrafficLogger::Instance().Error("RELAY_HAL", "Failed to initialize underlying GPIO driver");
        return false;
    }

    driver_->ConfigureOutput(pinRed_);
    driver_->ConfigureOutput(pinYellow_);
    driver_->ConfigureOutput(pinGreen_);
    driver_->ConfigureOutput(pinAux_);

    initialized_.store(true);

    // Initial safe state: Set ALL RED for safety clearance
    ApplyRelayStates(true, false, false, false);
    TrafficLogger::Instance().Info("RELAY_HAL", "Relay Controller initialized. Safe default (RED) energized.");
    return true;
}

void RelayController::Shutdown() {
    if (!initialized_.exchange(false)) {
        return;
    }
    TurnOffAll();
    driver_->Shutdown();
    TrafficLogger::Instance().Info("RELAY_HAL", "Relay Controller shut down cleanly.");
}

void RelayController::SetChannel(int channel, bool active) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    switch (channel) {
        case 1: ApplyRelayStates(active, yellowActive_.load(), greenActive_.load(), auxActive_.load()); break;
        case 2: ApplyRelayStates(redActive_.load(), active, greenActive_.load(), auxActive_.load()); break;
        case 3: ApplyRelayStates(redActive_.load(), yellowActive_.load(), active, auxActive_.load()); break;
        case 4: ApplyRelayStates(redActive_.load(), yellowActive_.load(), greenActive_.load(), active); break;
        default: break;
    }
}

void RelayController::SetAllChannels(bool ch1, bool ch2, bool ch3, bool ch4) {
    ApplyRelayStates(ch1, ch2, ch3, ch4);
}

bool RelayController::IsChannelActive(int channel) const {
    switch (channel) {
        case 1: return redActive_.load();
        case 2: return yellowActive_.load();
        case 3: return greenActive_.load();
        case 4: return auxActive_.load();
        default: return false;
    }
}

void RelayController::SetRed() {
    ApplyRelayStates(true, false, false, false);
}

void RelayController::SetYellow() {
    ApplyRelayStates(false, true, false, false);
}

void RelayController::SetGreen() {
    ApplyRelayStates(false, false, true, false);
}

void RelayController::SetAllRed() {
    ApplyRelayStates(true, false, false, true);
}

void RelayController::FlashYellow() {
    bool currentYellow = yellowActive_.load();
    ApplyRelayStates(false, !currentYellow, false, false);
}

void RelayController::TurnOffAll() {
    ApplyRelayStates(false, false, false, false);
}

void RelayController::ApplyRelayStates(bool red, bool yellow, bool green, bool aux) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!initialized_.load()) {
        return;
    }

    auto toLevel = [this](bool active) {
        return activeLow_ ? !active : active;
    };

    driver_->WritePin(pinRed_, toLevel(red));
    driver_->WritePin(pinYellow_, toLevel(yellow));
    driver_->WritePin(pinGreen_, toLevel(green));
    driver_->WritePin(pinAux_, toLevel(aux));

    redActive_.store(red);
    yellowActive_.store(yellow);
    greenActive_.store(green);
    auxActive_.store(aux);

    TrafficLogger::Instance().LogRelayChange(red, yellow, green, aux);
}

} // namespace aegis::traffic
