/**
 * @file MockGpioDriver.cpp
 * @brief Implementation of in-memory mock GPIO driver.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "traffic_node/gpio/MockGpioDriver.hpp"

namespace aegis::traffic {

MockGpioDriver::MockGpioDriver() = default;

bool MockGpioDriver::Initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    initialized_.store(true);
    return true;
}

void MockGpioDriver::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    initialized_.store(false);
    pinLevels_.clear();
    isOutput_.clear();
    writeCounts_.clear();
}

bool MockGpioDriver::ConfigureOutput(int pin) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_.load()) return false;
    isOutput_[pin] = true;
    if (pinLevels_.find(pin) == pinLevels_.end()) {
        pinLevels_[pin] = false;
    }
    return true;
}

bool MockGpioDriver::ConfigureInput(int pin) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_.load()) return false;
    isOutput_[pin] = false;
    return true;
}

bool MockGpioDriver::WritePin(int pin, bool high) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_.load()) return false;
    pinLevels_[pin] = high;
    writeCounts_[pin]++;
    return true;
}

bool MockGpioDriver::ReadPin(int pin) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = pinLevels_.find(pin);
    if (it != pinLevels_.end()) {
        return it->second;
    }
    return false;
}

bool MockGpioDriver::GetPinLevel(int pin) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = pinLevels_.find(pin);
    if (it != pinLevels_.end()) {
        return it->second;
    }
    return false;
}

uint32_t MockGpioDriver::GetWriteCount(int pin) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = writeCounts_.find(pin);
    if (it != writeCounts_.end()) {
        return it->second;
    }
    return 0;
}

bool MockGpioDriver::IsConfiguredOutput(int pin) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = isOutput_.find(pin);
    if (it != isOutput_.end()) {
        return it->second;
    }
    return false;
}

void MockGpioDriver::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    pinLevels_.clear();
    isOutput_.clear();
    writeCounts_.clear();
}

} // namespace aegis::traffic

