/**
 * @file SimulationGpioController.cpp
 * @brief Implementation of in-memory SimulationGpioController.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "gpio/SimulationGpioController.hpp"
#include <iostream>
#include <thread>

namespace aegis::gpio {

SimulationGpioController::SimulationGpioController(int intersectionId)
    : intersectionId_(intersectionId)
    , hasRed_(intersectionId != 3) // Node 3 has NO Red LED due to hardware shortage
    , hasBlue_(true) {
}

SimulationGpioController::~SimulationGpioController() {
    shutdown();
}

bool SimulationGpioController::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    initialized_.store(true);
    blueState_.store(false);
    redState_.store(false);
    std::cout << "[SIM_GPIO] Node " << intersectionId_
              << " initialized. Capabilities: Blue=YES, Red="
              << (hasRed_ ? "YES (GPIO5)" : "NO (Hardware Shortage)")
              << std::endl;
    return true;
}

void SimulationGpioController::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_.load()) return;
    blueState_.store(false);
    redState_.store(false);
    initialized_.store(false);
    std::cout << "[SIM_GPIO] Node " << intersectionId_ << " shutdown complete. All LEDs OFF." << std::endl;
}

bool SimulationGpioController::setBlue(bool on) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_.load()) return false;
    blueState_.store(on);
    writeCountBlue_++;
    std::cout << "[SIM_GPIO] Node " << intersectionId_ << " | BLUE LED (GPIO6) -> "
              << (on ? "ON [CORRIDOR ACTIVE]" : "OFF") << std::endl;
    return true;
}

bool SimulationGpioController::setRed(bool on) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_.load()) return false;

    if (!hasRed_) {
        // Intersection 3 has no Red LED due to hardware shortage; safe no-op
        std::cout << "[SIM_GPIO] Node " << intersectionId_
                  << " | RED LED -> IGNORED (No Red LED connected on Node 3)" << std::endl;
        return false;
    }

    redState_.store(on);
    writeCountRed_++;
    std::cout << "[SIM_GPIO] Node " << intersectionId_ << " | RED LED (GPIO5) -> "
              << (on ? "ON [HOLD]" : "OFF") << std::endl;
    return true;
}

bool SimulationGpioController::getBlue() const {
    return blueState_.load();
}

bool SimulationGpioController::getRed() const {
    if (!hasRed_) return false;
    return redState_.load();
}

bool SimulationGpioController::flashBlue(uint32_t durationMs, uint32_t count) {
    if (!initialized_.load()) return false;
    for (uint32_t i = 0; i < count; ++i) {
        setBlue(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
        setBlue(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    }
    return true;
}

bool SimulationGpioController::flashRed(uint32_t durationMs, uint32_t count) {
    if (!hasRed_ || !initialized_.load()) return false;
    for (uint32_t i = 0; i < count; ++i) {
        setRed(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
        setRed(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    }
    return true;
}

void SimulationGpioController::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    blueState_.store(false);
    redState_.store(false);
    std::cout << "[SIM_GPIO] Node " << intersectionId_ << " | RESET: All LEDs turned OFF." << std::endl;
}

} // namespace aegis::gpio
