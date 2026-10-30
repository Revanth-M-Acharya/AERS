/**
 * @file DeploymentGpioDriver.cpp
 * @brief SMART AEGIS (AERS) Phase 16D - Low-Level BCM2711 GPIO Hardware Driver Adapter Implementation.
 */

#include "DeploymentGpioDriver.h"
#include <iostream>
#include <sstream>

namespace aegis {
namespace deployment {

DeploymentGpioDriver::DeploymentGpioDriver() = default;

DeploymentGpioDriver::~DeploymentGpioDriver() {
    if (m_initialized) {
        Cleanup();
    }
}

bool DeploymentGpioDriver::Initialize(const GPIOPinMapping& mapping, bool simulation) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mapping = mapping;
    m_simulation = simulation;

    if (!m_bsp.IsBoardInitialized()) {
        if (!m_bsp.InitializeBoard(bsp::BoardRevision::RPI4_B_REV1_4)) {
            return false;
        }
    }

    // Configure Output Pins
    const std::vector<uint32_t> outputPins = {
        m_mapping.redPin, m_mapping.yellowPin, m_mapping.greenPin,
        m_mapping.nsGreenPin, m_mapping.ewGreenPin,
        m_mapping.pedWalkPin, m_mapping.pedDontWalkPin,
        m_mapping.preemptStrobePin, m_mapping.buzzerOutput
    };

    for (uint32_t pin : outputPins) {
        if (!m_bsp.IsGPIOReserved(pin)) {
            m_bsp.ReserveGPIO(pin, "DEPLOYMENT_GPIO");
        }
        m_bsp.ConfigurePinMode(pin, bsp::PinMode::OUTPUT);
        m_bsp.WriteGPIO(pin, bsp::PinState::LOW);
    }

    // Configure Input Pins
    const std::vector<uint32_t> inputPins = {
        m_mapping.irSensorNorthSouth,
        m_mapping.irSensorEastWest,
        m_mapping.emergencyOverrideButton
    };

    for (uint32_t pin : inputPins) {
        if (!m_bsp.IsGPIOReserved(pin)) {
            m_bsp.ReserveGPIO(pin, "DEPLOYMENT_GPIO");
        }
        m_bsp.ConfigurePinMode(pin, bsp::PinMode::INPUT);
        m_bsp.ConfigurePullResistor(pin, bsp::PullMode::PULL_DOWN);
    }

    // Set Initial Safe Signal State: Red lamp HIGH, Pedestrian Don't Walk HIGH
    m_bsp.WriteGPIO(m_mapping.redPin, bsp::PinState::HIGH);
    m_bsp.WriteGPIO(m_mapping.pedDontWalkPin, bsp::PinState::HIGH);

    m_initialized = true;
    return true;
}

bool DeploymentGpioDriver::Read(uint32_t pin, bool& outState) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return false;

    bsp::PinState state = bsp::PinState::LOW;
    if (!m_bsp.ReadGPIO(pin, state)) {
        return false;
    }
    outState = (state == bsp::PinState::HIGH);
    return true;
}

bool DeploymentGpioDriver::Write(uint32_t pin, bool state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return false;

    return m_bsp.WriteGPIO(pin, state ? bsp::PinState::HIGH : bsp::PinState::LOW);
}

bool DeploymentGpioDriver::Toggle(uint32_t pin) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return false;

    return m_bsp.ToggleGPIO(pin);
}

bool DeploymentGpioDriver::Cleanup() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return true;

    const auto allPins = m_mapping.GetAllAssignedPins();
    for (uint32_t pin : allPins) {
        m_bsp.WriteGPIO(pin, bsp::PinState::LOW);
        m_bsp.ConfigurePinMode(pin, bsp::PinMode::INPUT);
        m_bsp.ReleaseGPIO(pin);
    }

    m_initialized = false;
    return true;
}

bool DeploymentGpioDriver::SetTrafficSignalState(const std::string& state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return false;

    // Reset primary vehicular lamps
    m_bsp.WriteGPIO(m_mapping.redPin, bsp::PinState::LOW);
    m_bsp.WriteGPIO(m_mapping.yellowPin, bsp::PinState::LOW);
    m_bsp.WriteGPIO(m_mapping.greenPin, bsp::PinState::LOW);

    if (state == "RED" || state == "ALL_RED") {
        m_bsp.WriteGPIO(m_mapping.redPin, bsp::PinState::HIGH);
        m_bsp.WriteGPIO(m_mapping.nsGreenPin, bsp::PinState::LOW);
        m_bsp.WriteGPIO(m_mapping.ewGreenPin, bsp::PinState::LOW);
    } else if (state == "YELLOW") {
        m_bsp.WriteGPIO(m_mapping.yellowPin, bsp::PinState::HIGH);
    } else if (state == "GREEN") {
        m_bsp.WriteGPIO(m_mapping.greenPin, bsp::PinState::HIGH);
        m_bsp.WriteGPIO(m_mapping.nsGreenPin, bsp::PinState::HIGH);
    } else if (state == "FLASHING_YELLOW") {
        m_bsp.WriteGPIO(m_mapping.yellowPin, bsp::PinState::HIGH);
    } else {
        // Safe default: Red
        m_bsp.WriteGPIO(m_mapping.redPin, bsp::PinState::HIGH);
    }

    return true;
}

bool DeploymentGpioDriver::SetPedestrianSignal(bool walk) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return false;

    m_bsp.WriteGPIO(m_mapping.pedWalkPin, walk ? bsp::PinState::HIGH : bsp::PinState::LOW);
    m_bsp.WriteGPIO(m_mapping.pedDontWalkPin, walk ? bsp::PinState::LOW : bsp::PinState::HIGH);
    return true;
}

bool DeploymentGpioDriver::SetBuzzer(bool active) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return false;

    return m_bsp.WriteGPIO(m_mapping.buzzerOutput, active ? bsp::PinState::HIGH : bsp::PinState::LOW);
}

bool DeploymentGpioDriver::SetPreemptionStrobe(bool active) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return false;

    return m_bsp.WriteGPIO(m_mapping.preemptStrobePin, active ? bsp::PinState::HIGH : bsp::PinState::LOW);
}

bool DeploymentGpioDriver::ReadIRSensorNorthSouth(bool& vehicleDetected) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return false;

    if (m_simulation) {
        vehicleDetected = m_simulatedIrNS;
        return true;
    }
    bsp::PinState s = bsp::PinState::LOW;
    bool ok = m_bsp.ReadGPIO(m_mapping.irSensorNorthSouth, s);
    vehicleDetected = (s == bsp::PinState::HIGH);
    return ok;
}

bool DeploymentGpioDriver::ReadIRSensorEastWest(bool& vehicleDetected) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return false;

    if (m_simulation) {
        vehicleDetected = m_simulatedIrEW;
        return true;
    }
    bsp::PinState s = bsp::PinState::LOW;
    bool ok = m_bsp.ReadGPIO(m_mapping.irSensorEastWest, s);
    vehicleDetected = (s == bsp::PinState::HIGH);
    return ok;
}

bool DeploymentGpioDriver::ReadEmergencyOverrideButton(bool& buttonPressed) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return false;

    if (m_simulation) {
        buttonPressed = m_simulatedOverride;
        return true;
    }
    bsp::PinState s = bsp::PinState::LOW;
    bool ok = m_bsp.ReadGPIO(m_mapping.emergencyOverrideButton, s);
    buttonPressed = (s == bsp::PinState::HIGH);
    return ok;
}

std::string DeploymentGpioDriver::GetStatusString() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::ostringstream oss;
    bsp::PinState red = bsp::PinState::LOW;
    bsp::PinState yellow = bsp::PinState::LOW;
    bsp::PinState green = bsp::PinState::LOW;
    bsp::PinState walk = bsp::PinState::LOW;
    bsp::PinState strobe = bsp::PinState::LOW;

    m_bsp.ReadGPIO(m_mapping.redPin, red);
    m_bsp.ReadGPIO(m_mapping.yellowPin, yellow);
    m_bsp.ReadGPIO(m_mapping.greenPin, green);
    m_bsp.ReadGPIO(m_mapping.pedWalkPin, walk);
    m_bsp.ReadGPIO(m_mapping.preemptStrobePin, strobe);

    oss << "RED=" << (red == bsp::PinState::HIGH ? "ON" : "OFF")
        << " YELLOW=" << (yellow == bsp::PinState::HIGH ? "ON" : "OFF")
        << " GREEN=" << (green == bsp::PinState::HIGH ? "ON" : "OFF")
        << " PED_WALK=" << (walk == bsp::PinState::HIGH ? "ON" : "OFF")
        << " STROBE=" << (strobe == bsp::PinState::HIGH ? "ON" : "OFF");
    return oss.str();
}

} // namespace deployment
} // namespace aegis
