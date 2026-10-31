/**
 * @file SymbolicGpioHAL.cpp
 * @brief Implementation of SymbolicGpioHAL.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "hardware_integration/gpio/SymbolicGpioHAL.hpp"
#include "common/utils/TimeUtils.hpp"
#include "traffic_node/gpio/MockGpioDriver.hpp"

namespace aegis::hardware_integration {

SymbolicGpioHAL::SymbolicGpioHAL(std::shared_ptr<traffic::IGpioDriver> driver)
    : driver_(std::move(driver)) {
    if (!driver_) {
        driver_ = std::make_shared<traffic::MockGpioDriver>();
    }
}

SymbolicGpioHAL::~SymbolicGpioHAL() {
    Shutdown();
}

void SymbolicGpioHAL::SetDriver(std::shared_ptr<traffic::IGpioDriver> driver) {
    std::lock_guard<std::mutex> lock(mutex_);
    driver_ = std::move(driver);
}

std::shared_ptr<traffic::IGpioDriver> SymbolicGpioHAL::GetDriver() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return driver_;
}

bool SymbolicGpioHAL::Initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!driver_) {
        return false;
    }

    if (!driver_->Initialize()) {
        return false;
    }

    // Configure all registered pins
    for (auto& [name, sig] : signalsByName_) {
        if (sig.direction == PinDirection::OUTPUT) {
            driver_->ConfigureOutput(sig.physicalPin);
            // Default to inactive state
            bool rawLevel = (sig.polarity == ActivePolarity::ACTIVE_LOW);
            driver_->WritePin(sig.physicalPin, rawLevel);
            sig.currentLogicalState = false;
        } else {
            driver_->ConfigureInput(sig.physicalPin);
            // Simulate internal pull-up resistor for active-low inputs (idle = 3.3V / HIGH)
            if (sig.polarity == ActivePolarity::ACTIVE_LOW) {
                driver_->WritePin(sig.physicalPin, true);
            }
        }
    }

    isInitialized_ = true;
    return true;
}

void SymbolicGpioHAL::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!isInitialized_ || !driver_) {
        return;
    }

    // De-assert all output pins to safe inactive state
    for (auto& [name, sig] : signalsByName_) {
        if (sig.direction == PinDirection::OUTPUT) {
            bool rawLevel = (sig.polarity == ActivePolarity::ACTIVE_LOW);
            driver_->WritePin(sig.physicalPin, rawLevel);
            sig.currentLogicalState = false;
        }
    }

    driver_->Shutdown();
    isInitialized_ = false;
}

bool SymbolicGpioHAL::RegisterSignal(const std::string& symbolicName,
                                    int physicalPin,
                                    PinDirection direction,
                                    ActivePolarity polarity,
                                    const std::string& description) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (symbolicName.empty() || physicalPin < 0 || physicalPin > 27) {
        return false;
    }

    // Prevent duplicate symbolic names
    if (signalsByName_.find(symbolicName) != signalsByName_.end()) {
        return false;
    }

    // Prevent pin collision
    if (signalsByPin_.find(physicalPin) != signalsByPin_.end()) {
        return false;
    }

    SignalDefinition def;
    def.symbolicName = symbolicName;
    def.physicalPin = physicalPin;
    def.direction = direction;
    def.polarity = polarity;
    def.description = description;
    def.currentLogicalState = false;
    def.lastToggleTimestampMs = common::TimeUtils::GetMonotonicTimeMs();

    if (isInitialized_ && driver_) {
        if (direction == PinDirection::OUTPUT) {
            driver_->ConfigureOutput(physicalPin);
            bool rawLevel = (polarity == ActivePolarity::ACTIVE_LOW);
            driver_->WritePin(physicalPin, rawLevel);
        } else {
            driver_->ConfigureInput(physicalPin);
            // Simulate internal pull-up resistor for active-low inputs (idle = 3.3V / HIGH)
            if (polarity == ActivePolarity::ACTIVE_LOW) {
                driver_->WritePin(physicalPin, true);
            }
        }
    }

    signalsByName_[symbolicName] = def;
    signalsByPin_[physicalPin] = symbolicName;
    return true;
}

bool SymbolicGpioHAL::IsSignalRegistered(const std::string& symbolicName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return signalsByName_.find(symbolicName) != signalsByName_.end();
}

int SymbolicGpioHAL::GetPhysicalPin(const std::string& symbolicName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = signalsByName_.find(symbolicName);
    if (it != signalsByName_.end()) {
        return it->second.physicalPin;
    }
    return -1;
}

std::optional<SignalDefinition> SymbolicGpioHAL::GetSignalDefinition(const std::string& symbolicName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = signalsByName_.find(symbolicName);
    if (it != signalsByName_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool SymbolicGpioHAL::SetSignal(const std::string& symbolicName, bool active) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!driver_) {
        return false;
    }

    auto it = signalsByName_.find(symbolicName);
    if (it == signalsByName_.end()) {
        return false;
    }

    SignalDefinition& sig = it->second;
    bool rawLevel = (sig.polarity == ActivePolarity::ACTIVE_HIGH) ? active : !active;

    if (driver_->WritePin(sig.physicalPin, rawLevel)) {
        sig.currentLogicalState = active;
        sig.lastToggleTimestampMs = common::TimeUtils::GetMonotonicTimeMs();
        return true;
    }
    return false;
}

bool SymbolicGpioHAL::GetSignal(const std::string& symbolicName) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!driver_) {
        return false;
    }

    auto it = signalsByName_.find(symbolicName);
    if (it == signalsByName_.end()) {
        return false;
    }

    SignalDefinition& sig = it->second;
    bool rawLevel = driver_->ReadPin(sig.physicalPin);
    bool logicalActive = (sig.polarity == ActivePolarity::ACTIVE_HIGH) ? rawLevel : !rawLevel;
    sig.currentLogicalState = logicalActive;
    return logicalActive;
}

bool SymbolicGpioHAL::ToggleSignal(const std::string& symbolicName) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!driver_) {
        return false;
    }

    auto it = signalsByName_.find(symbolicName);
    if (it == signalsByName_.end()) {
        return false;
    }

    SignalDefinition& sig = it->second;
    bool newActive = !sig.currentLogicalState;
    bool rawLevel = (sig.polarity == ActivePolarity::ACTIVE_HIGH) ? newActive : !newActive;

    if (driver_->WritePin(sig.physicalPin, rawLevel)) {
        sig.currentLogicalState = newActive;
        sig.lastToggleTimestampMs = common::TimeUtils::GetMonotonicTimeMs();
        return true;
    }
    return false;
}

bool SymbolicGpioHAL::ReadRawPin(int physicalPin) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!driver_) return false;
    return driver_->ReadPin(physicalPin);
}

bool SymbolicGpioHAL::WriteRawPin(int physicalPin, bool high) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!driver_) return false;
    return driver_->WritePin(physicalPin, high);
}

std::vector<SignalDefinition> SymbolicGpioHAL::GetAllSignals() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<SignalDefinition> result;
    result.reserve(signalsByName_.size());
    for (const auto& [name, sig] : signalsByName_) {
        result.push_back(sig);
    }
    return result;
}

void SymbolicGpioHAL::ClearSignals() {
    std::lock_guard<std::mutex> lock(mutex_);
    signalsByName_.clear();
    signalsByPin_.clear();
}

} // namespace aegis::hardware_integration
