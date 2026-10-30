/**
 * @file TrafficLightController.cpp
 * @brief SMART AEGIS (AERS) Phase 20 - Dual-Module Complementary Traffic Light Controller Implementation.
 */

#include "TrafficLightController.h"

namespace aegis {
namespace hardware {

std::string LightColorToString(LightColor color) {
    switch (color) {
        case LightColor::RED:    return "RED";
        case LightColor::YELLOW: return "YELLOW";
        case LightColor::GREEN:  return "GREEN";
        case LightColor::OFF:    return "OFF";
    }
    return "OFF";
}

std::string DualPhaseToString(DualPhase phase) {
    switch (phase) {
        case DualPhase::ROAD_A_GREEN_ROAD_B_RED:
            return "ROAD_A_GREEN_ROAD_B_RED";
        case DualPhase::ROAD_A_YELLOW_ROAD_B_RED:
            return "ROAD_A_YELLOW_ROAD_B_RED";
        case DualPhase::ALL_RED_CLEARANCE:
            return "ALL_RED_CLEARANCE";
        case DualPhase::ROAD_A_RED_ROAD_B_GREEN:
            return "ROAD_A_RED_ROAD_B_GREEN";
        case DualPhase::ROAD_A_RED_ROAD_B_YELLOW:
            return "ROAD_A_RED_ROAD_B_YELLOW";
        case DualPhase::PREEMPTION_ROAD_A:
            return "PREEMPTION_ROAD_A";
        case DualPhase::PREEMPTION_ROAD_B:
            return "PREEMPTION_ROAD_B";
        case DualPhase::FAILSAFE_FLASHING_YELLOW:
            return "FAILSAFE_FLASHING_YELLOW";
    }
    return "ALL_RED_CLEARANCE";
}

TrafficLightController::TrafficLightController(std::shared_ptr<bsp::RaspberryPi4BSP> bsp,
                                               std::shared_ptr<RelayController> relayController)
    : m_bsp(std::move(bsp))
    , m_relayController(std::move(relayController))
{
    if (!m_bsp) {
        m_bsp = std::make_shared<bsp::RaspberryPi4BSP>();
    }
}

TrafficLightController::~TrafficLightController() {
    Shutdown();
}

bool TrafficLightController::Initialize(const DualSignalPins& pins) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pins = pins;

    if (m_bsp) {
        if (!m_bsp->IsBoardInitialized()) {
            m_bsp->InitializeBoard();
        }

        // Reserve and configure all signal pins as OUTPUT
        for (uint32_t pin : m_pins.GetAllPins()) {
            if (!m_bsp->IsGPIOReserved(pin)) {
                m_bsp->ReserveGPIO(pin, "TRAFFIC_LIGHT_CONTROLLER");
            }
            m_bsp->ConfigurePinMode(pin, bsp::PinMode::OUTPUT);
            m_bsp->WriteGPIO(pin, bsp::PinState::LOW);
        }
    }

    m_initialized = true;

    // Set initial safe complementary phase (Road A Red, Road B Green)
    m_currentPhase = DualPhase::ROAD_A_RED_ROAD_B_GREEN;
    m_roadAColor = LightColor::RED;
    m_roadBColor = LightColor::GREEN;
    ApplyHardwareOutputs();

    return true;
}

void TrafficLightController::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) {
        return;
    }

    // Turn off all lamps
    m_roadAColor = LightColor::OFF;
    m_roadBColor = LightColor::OFF;
    ApplyHardwareOutputs();

    if (m_bsp) {
        for (uint32_t pin : m_pins.GetAllPins()) {
            m_bsp->WriteGPIO(pin, bsp::PinState::LOW);
            m_bsp->ReleaseGPIO(pin);
        }
    }

    m_initialized = false;
}

bool TrafficLightController::EnforceMutualExclusion(LightColor roadA, LightColor roadB) {
    // Critical safety invariant: Road A and Road B can NEVER be GREEN simultaneously
    if (roadA == LightColor::GREEN && roadB == LightColor::GREEN) {
        m_conflictDetected = true;
        m_hardwareFault = true;
        m_roadAColor = LightColor::RED;
        m_roadBColor = LightColor::RED;
        m_currentPhase = DualPhase::ALL_RED_CLEARANCE;
        ApplyHardwareOutputs();
        return false;
    }
    return true;
}

bool TrafficLightController::SetPhase(DualPhase phase) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentPhase = phase;

    switch (phase) {
        case DualPhase::ROAD_A_GREEN_ROAD_B_RED:
            m_roadAColor = LightColor::GREEN;
            m_roadBColor = LightColor::RED;
            m_preemptStrobe = false;
            break;

        case DualPhase::ROAD_A_YELLOW_ROAD_B_RED:
            m_roadAColor = LightColor::YELLOW;
            m_roadBColor = LightColor::RED;
            m_preemptStrobe = false;
            break;

        case DualPhase::ALL_RED_CLEARANCE:
            m_roadAColor = LightColor::RED;
            m_roadBColor = LightColor::RED;
            m_preemptStrobe = false;
            break;

        case DualPhase::ROAD_A_RED_ROAD_B_GREEN:
            m_roadAColor = LightColor::RED;
            m_roadBColor = LightColor::GREEN;
            m_preemptStrobe = false;
            break;

        case DualPhase::ROAD_A_RED_ROAD_B_YELLOW:
            m_roadAColor = LightColor::RED;
            m_roadBColor = LightColor::YELLOW;
            m_preemptStrobe = false;
            break;

        case DualPhase::PREEMPTION_ROAD_A:
            m_roadAColor = LightColor::GREEN;
            m_roadBColor = LightColor::RED;
            m_preemptStrobe = true;
            break;

        case DualPhase::PREEMPTION_ROAD_B:
            m_roadAColor = LightColor::RED;
            m_roadBColor = LightColor::GREEN;
            m_preemptStrobe = true;
            break;

        case DualPhase::FAILSAFE_FLASHING_YELLOW:
            m_roadAColor = LightColor::YELLOW;
            m_roadBColor = LightColor::RED;
            m_preemptStrobe = false;
            break;
    }

    if (!EnforceMutualExclusion(m_roadAColor, m_roadBColor)) {
        return false;
    }

    ApplyHardwareOutputs();
    return true;
}

DualPhase TrafficLightController::GetCurrentPhase() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentPhase;
}

bool TrafficLightController::SetSignals(LightColor roadA, LightColor roadB, bool bypassInterlockForTest) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!bypassInterlockForTest) {
        if (!EnforceMutualExclusion(roadA, roadB)) {
            return false;
        }
    } else {
        // Explicitly testing hardware conflict detection
        if (roadA == LightColor::GREEN && roadB == LightColor::GREEN) {
            m_conflictDetected = true;
        }
    }

    m_roadAColor = roadA;
    m_roadBColor = roadB;
    ApplyHardwareOutputs();
    return true;
}

LightColor TrafficLightController::GetRoadAColor() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_roadAColor;
}

LightColor TrafficLightController::GetRoadBColor() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_roadBColor;
}

bool TrafficLightController::IsConflictingGreenDetected() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_conflictDetected;
}

bool TrafficLightController::HasHardwareFault() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_hardwareFault;
}

void TrafficLightController::ClearFault() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_conflictDetected = false;
    m_hardwareFault = false;
}

bool TrafficLightController::SetPedestrianSignal(bool walk) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pedWalk = walk;
    if (m_bsp) {
        m_bsp->WriteGPIO(m_pins.pedWalkPin, walk ? bsp::PinState::HIGH : bsp::PinState::LOW);
        m_bsp->WriteGPIO(m_pins.pedDontWalkPin, walk ? bsp::PinState::LOW : bsp::PinState::HIGH);
    }
    return true;
}

bool TrafficLightController::SetPreemptionStrobe(bool active) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_preemptStrobe = active;
    if (m_bsp) {
        m_bsp->WriteGPIO(m_pins.strobePin, active ? bsp::PinState::HIGH : bsp::PinState::LOW);
    }
    return true;
}

bool TrafficLightController::SetBuzzer(bool active) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_buzzerActive = active;
    if (m_bsp) {
        m_bsp->WriteGPIO(m_pins.buzzerPin, active ? bsp::PinState::HIGH : bsp::PinState::LOW);
    }
    return true;
}

void TrafficLightController::ApplyHardwareOutputs() {
    if (!m_bsp) {
        return;
    }

    // Road A (North-South)
    m_bsp->WriteGPIO(m_pins.roadARedPin,
                     (m_roadAColor == LightColor::RED) ? bsp::PinState::HIGH : bsp::PinState::LOW);
    m_bsp->WriteGPIO(m_pins.roadAYellowPin,
                     (m_roadAColor == LightColor::YELLOW) ? bsp::PinState::HIGH : bsp::PinState::LOW);
    m_bsp->WriteGPIO(m_pins.roadAGreenPin,
                     (m_roadAColor == LightColor::GREEN) ? bsp::PinState::HIGH : bsp::PinState::LOW);

    // Road B (East-West)
    m_bsp->WriteGPIO(m_pins.roadBRedPin,
                     (m_roadBColor == LightColor::RED) ? bsp::PinState::HIGH : bsp::PinState::LOW);
    m_bsp->WriteGPIO(m_pins.roadBYellowPin,
                     (m_roadBColor == LightColor::YELLOW) ? bsp::PinState::HIGH : bsp::PinState::LOW);
    m_bsp->WriteGPIO(m_pins.roadBGreenPin,
                     (m_roadBColor == LightColor::GREEN) ? bsp::PinState::HIGH : bsp::PinState::LOW);

    // If Relay Controller is attached, mirror outputs to relay channels
    if (m_relayController && m_relayController->IsInitialized()) {
        m_relayController->SetRelay(1, m_roadAColor == LightColor::RED);
        m_relayController->SetRelay(2, m_roadAColor == LightColor::GREEN);
        m_relayController->SetRelay(3, m_roadBColor == LightColor::RED);
        m_relayController->SetRelay(4, m_roadBColor == LightColor::GREEN);
    }
}

} // namespace hardware
} // namespace aegis

