/**
 * @file IntersectionHardware.cpp
 * @brief SMART AEGIS (AERS) Phase 20 - Roadside Intersection Physical Hardware Implementation.
 */

#include "IntersectionHardware.h"
#include "aegis/hardware/GpioReservationManager.hpp"

namespace aegis {
namespace hardware {

IntersectionHardware::IntersectionHardware(std::string nodeId,
                                           std::shared_ptr<HardwareEventLogger> logger)
    : m_nodeId(std::move(nodeId))
    , m_logger(std::move(logger))
    , m_bsp(std::make_shared<bsp::RaspberryPi4BSP>())
{
    if (!m_logger) {
        m_logger = std::make_shared<HardwareEventLogger>();
    }
    m_relayController = std::make_shared<RelayController>(m_bsp);
    m_trafficLights = std::make_shared<TrafficLightController>(m_bsp, m_relayController);
}

IntersectionHardware::~IntersectionHardware() {
    Shutdown();
}

bool IntersectionHardware::Initialize(const DualSignalPins& pins,
                                      const std::vector<RelayChannelDef>& relays,
                                      const IrSensorParameters& irConfig,
                                      const TimingParameters& timings) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_pins = pins;
    m_irConfig = irConfig;
    m_timings = timings;

    // 1. BSP Initialization
    if (!m_bsp->IsBoardInitialized()) {
        if (!m_bsp->InitializeBoard()) {
            m_logger->Error("HARDWARE", "BSP Initialization failed for node " + m_nodeId + ". Aborting startup.");
            return false;
        }
    }

    // 2. Configure IR Sensor Pins as INPUT with Pull-Up
    m_approachIr.gpioPin = irConfig.approachSensorPin;
    m_approachIr.isHealthy = true;
    m_departureIr.gpioPin = irConfig.departureSensorPin;
    m_departureIr.isHealthy = true;

    if (!m_bsp->IsGPIOReserved(m_approachIr.gpioPin)) {
        if (!m_bsp->ReserveGPIO(m_approachIr.gpioPin, "IR_SENSOR_APPROACH")) {
            m_logger->Error("HARDWARE", "Failed to reserve approach IR sensor pin " + std::to_string(m_approachIr.gpioPin));
            Shutdown();
            return false;
        }
    }
    if (!GpioReservationManager::Instance().ReservePin(
            m_approachIr.gpioPin, "OpticalSensorFilter", "Approach IR Sensor", bsp::PinMode::INPUT)) {
        m_logger->Error("HARDWARE", "Reservation conflict on approach IR pin GPIO" + std::to_string(m_approachIr.gpioPin));
        Shutdown();
        return false;
    }
    if (!m_bsp->ConfigurePinMode(m_approachIr.gpioPin, bsp::PinMode::INPUT) ||
        !m_bsp->ConfigurePullResistor(m_approachIr.gpioPin, bsp::PullMode::PULL_UP)) {
        m_logger->Error("HARDWARE", "Failed to configure approach IR pin direction/resistor");
        Shutdown();
        return false;
    }

    if (!m_bsp->IsGPIOReserved(m_departureIr.gpioPin)) {
        if (!m_bsp->ReserveGPIO(m_departureIr.gpioPin, "IR_SENSOR_DEPARTURE")) {
            m_logger->Error("HARDWARE", "Failed to reserve departure IR sensor pin " + std::to_string(m_departureIr.gpioPin));
            Shutdown();
            return false;
        }
    }
    if (!GpioReservationManager::Instance().ReservePin(
            m_departureIr.gpioPin, "OpticalSensorFilter", "Departure IR Sensor", bsp::PinMode::INPUT)) {
        m_logger->Error("HARDWARE", "Reservation conflict on departure IR pin GPIO" + std::to_string(m_departureIr.gpioPin));
        Shutdown();
        return false;
    }
    if (!m_bsp->ConfigurePinMode(m_departureIr.gpioPin, bsp::PinMode::INPUT) ||
        !m_bsp->ConfigurePullResistor(m_departureIr.gpioPin, bsp::PullMode::PULL_UP)) {
        m_logger->Error("HARDWARE", "Failed to configure departure IR pin direction/resistor");
        Shutdown();
        return false;
    }

    // 3. Initialize Relays & Traffic Lights (Direct Active-HIGH GPIO LED drive)
    if (!m_relayController->Initialize(relays, false)) {
        m_logger->Error("HARDWARE", "Relay controller initialization failed. Aborting startup.");
        Shutdown();
        return false;
    }
    if (!m_trafficLights->Initialize(pins)) {
        m_logger->Error("HARDWARE", "Traffic lights initialization failed. Aborting startup.");
        Shutdown();
        return false;
    }

    // 4. Hardware Readiness Verification
    if (!m_relayController->IsInitialized()) {
        m_logger->Error("HARDWARE", "Hardware readiness verification failed after initialization.");
        Shutdown();
        return false;
    }

    m_approachFilter.SetDebounceMs(m_irConfig.debounceMs);
    m_approachFilter.SetStuckTimeoutMs(m_irConfig.stuckTimeoutSec * 1000);
    m_departureFilter.SetDebounceMs(m_irConfig.debounceMs);
    m_departureFilter.SetStuckTimeoutMs(m_irConfig.stuckTimeoutSec * 1000);

    m_phaseTimer = m_timings.normalGreenSec;
    m_initialized = true;

    m_logger->Info("HARDWARE", "Intersection " + m_nodeId + " physical hardware initialized successfully.");
    return true;
}

void IntersectionHardware::Shutdown() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!m_initialized) {
        return;
    }

    // Safe state: Set signals to ALL_RED clearance
    if (m_trafficLights) {
        m_trafficLights->SetPhase(DualPhase::ALL_RED_CLEARANCE);
        m_trafficLights->Shutdown();
    }

    // De-energize all relays
    if (m_relayController) {
        m_relayController->DeenergizeAll();
        m_relayController->Shutdown();
    }

    // Release GPIO reservations
    if (m_bsp) {
        m_bsp->ReleaseGPIO(m_approachIr.gpioPin);
        m_bsp->ReleaseGPIO(m_departureIr.gpioPin);
    }
    GpioReservationManager::Instance().ReleasePin(m_approachIr.gpioPin, "OpticalSensorFilter");
    GpioReservationManager::Instance().ReleasePin(m_departureIr.gpioPin, "OpticalSensorFilter");

    // Unmap hardware memory & cleanup BSP
    if (m_bsp) {
        m_bsp->Cleanup();
    }

    m_initialized = false;
    m_logger->Info("HARDWARE", "Intersection " + m_nodeId + " shut down cleanly with hardware memory unmapped.");
}

void IntersectionHardware::Update(double dtSec) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!m_initialized) {
        return;
    }

    UpdateIrSensors(dtSec);

    if (m_inFailSafe) {
        m_flashTimer += dtSec;
        if (m_flashTimer >= 0.5) { // 1 Hz flashing
            m_flashTimer = 0.0;
            m_flashState = !m_flashState;
            if (m_flashState) {
                m_trafficLights->SetSignals(LightColor::YELLOW, LightColor::OFF);
            } else {
                m_trafficLights->SetSignals(LightColor::OFF, LightColor::OFF);
            }
        }
        return;
    }

    if (m_isPreempted) {
        m_preemptionElapsedSec += dtSec;
        if (m_preemptionElapsedSec >= m_timings.preemptionMaxHoldSec) {
            ReleasePreemption();
        }
        return;
    }

    AdvanceNormalCycle(dtSec);
}

void IntersectionHardware::AdvanceNormalCycle(double dtSec) {
    m_phaseTimer -= dtSec;
    if (m_phaseTimer > 0.0) {
        return;
    }

    DualPhase current = m_trafficLights->GetCurrentPhase();
    switch (current) {
        case DualPhase::ROAD_A_GREEN_ROAD_B_RED:
            m_trafficLights->SetPhase(DualPhase::ROAD_A_YELLOW_ROAD_B_RED);
            m_phaseTimer = m_timings.clearanceYellowSec;
            m_logger->Info("SIGNAL", m_nodeId + ": Road A Yellow Clearance");
            break;

        case DualPhase::ROAD_A_YELLOW_ROAD_B_RED:
            m_trafficLights->SetPhase(DualPhase::ALL_RED_CLEARANCE);
            m_phaseTimer = m_timings.allRedHoldSec;
            m_logger->Info("SIGNAL", m_nodeId + ": All-Red Clearance");
            break;

        case DualPhase::ALL_RED_CLEARANCE:
            // Switch to Road B Green
            m_trafficLights->SetPhase(DualPhase::ROAD_A_RED_ROAD_B_GREEN);
            m_phaseTimer = m_timings.normalGreenSec;
            m_logger->Info("SIGNAL", m_nodeId + ": Road B Green Phase");
            break;

        case DualPhase::ROAD_A_RED_ROAD_B_GREEN:
            m_trafficLights->SetPhase(DualPhase::ROAD_A_RED_ROAD_B_YELLOW);
            m_phaseTimer = m_timings.clearanceYellowSec;
            m_logger->Info("SIGNAL", m_nodeId + ": Road B Yellow Clearance");
            break;

        case DualPhase::ROAD_A_RED_ROAD_B_YELLOW:
            m_trafficLights->SetPhase(DualPhase::ALL_RED_CLEARANCE);
            m_phaseTimer = m_timings.allRedHoldSec;
            m_logger->Info("SIGNAL", m_nodeId + ": All-Red Clearance");
            break;

        default:
            m_trafficLights->SetPhase(DualPhase::ROAD_A_GREEN_ROAD_B_RED);
            m_phaseTimer = m_timings.normalGreenSec;
            m_logger->Info("SIGNAL", m_nodeId + ": Road A Green Phase");
            break;
    }
}

void IntersectionHardware::TriggerPreemption(const std::string& corridorId,
                                            const std::string& roadDirection) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_isPreempted = true;
    m_activeCorridorId = corridorId;
    m_preemptDirection = roadDirection;
    m_preemptionElapsedSec = 0.0;

    if (roadDirection == "ROAD_B") {
        m_trafficLights->SetPhase(DualPhase::PREEMPTION_ROAD_B);
    } else {
        m_trafficLights->SetPhase(DualPhase::PREEMPTION_ROAD_A);
    }

    m_trafficLights->SetPreemptionStrobe(true);
    m_trafficLights->SetBuzzer(true);
    m_logger->Critical("PREEMPTION", m_nodeId + ": Emergency Preemption Activated for " + corridorId + " on " + roadDirection);
}

void IntersectionHardware::ReleasePreemption() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!m_isPreempted) {
        return;
    }

    m_isPreempted = false;
    m_activeCorridorId.clear();
    m_preemptionElapsedSec = 0.0;

    m_trafficLights->SetPreemptionStrobe(false);
    m_trafficLights->SetBuzzer(false);

    // Enter All-Red clearance before returning to normal cycle
    m_trafficLights->SetPhase(DualPhase::ALL_RED_CLEARANCE);
    m_phaseTimer = m_timings.allRedHoldSec;
    m_logger->Info("PREEMPTION", m_nodeId + ": Preemption Released. Entering All-Red Clearance.");
}

bool IntersectionHardware::IsPreempted() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_isPreempted;
}

std::string IntersectionHardware::GetActiveCorridorId() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_activeCorridorId;
}

void IntersectionHardware::TriggerFailSafe(const std::string& reason) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_inFailSafe = true;
    m_failsafeReason = reason;
    m_trafficLights->SetPhase(DualPhase::FAILSAFE_FLASHING_YELLOW);
    m_logger->Error("FAILSAFE", m_nodeId + " triggered fail-safe mode: " + reason);
}

void IntersectionHardware::RecoverFromFailSafe() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!m_inFailSafe) {
        return;
    }

    m_inFailSafe = false;
    m_failsafeReason.clear();
    m_trafficLights->ClearFault();
    m_trafficLights->SetPhase(DualPhase::ALL_RED_CLEARANCE);
    m_phaseTimer = m_timings.allRedHoldSec;
    m_logger->Info("FAILSAFE", m_nodeId + " recovered from fail-safe mode.");
}

bool IntersectionHardware::IsInFailSafe() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_inFailSafe;
}

std::string IntersectionHardware::GetFailSafeReason() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_failsafeReason;
}

void IntersectionHardware::ForceGreen(const std::string& roadDirection) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (roadDirection == "ROAD_B") {
        m_trafficLights->SetPhase(DualPhase::ROAD_A_RED_ROAD_B_GREEN);
    } else {
        m_trafficLights->SetPhase(DualPhase::ROAD_A_GREEN_ROAD_B_RED);
    }
    m_phaseTimer = 60.0;
    m_logger->Warn("OVERRIDE", m_nodeId + ": Manual Force Green on " + roadDirection);
}

void IntersectionHardware::ForceRed() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_trafficLights->SetPhase(DualPhase::ALL_RED_CLEARANCE);
    m_phaseTimer = 60.0;
    m_logger->Warn("OVERRIDE", m_nodeId + ": Manual Force All-Red");
}

void IntersectionHardware::RestoreNormalCycle() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_isPreempted = false;
    m_inFailSafe = false;
    m_trafficLights->SetPhase(DualPhase::ROAD_A_GREEN_ROAD_B_RED);
    m_phaseTimer = m_timings.normalGreenSec;
    m_logger->Info("SIGNAL", m_nodeId + ": Normal signal cycle restored.");
}

void IntersectionHardware::UpdateIrSensors(double dtSec) {
    // Check Approach Sensor
    if (m_approachPulseTimer > 0.0) {
        m_approachPulseTimer -= dtSec;
        if (m_approachPulseTimer <= 0.0) {
            m_approachIr.beamBroken = false;
        }
    }

    // Check Departure Sensor
    if (m_departurePulseTimer > 0.0) {
        m_departurePulseTimer -= dtSec;
        if (m_departurePulseTimer <= 0.0) {
            m_departureIr.beamBroken = false;
        }
    }

    // Stuck beam check
    if (m_approachIr.beamBroken) {
        m_approachIr.continuousObstructionSec += dtSec;
        if (m_approachIr.continuousObstructionSec > static_cast<double>(m_irConfig.stuckTimeoutSec)) {
            m_approachIr.isStuckFault = true;
            m_approachIr.isHealthy = false;
        }
    } else {
        m_approachIr.continuousObstructionSec = 0.0;
        m_approachIr.isStuckFault = false;
        m_approachIr.isHealthy = true;
    }
}

bool IntersectionHardware::ReadApproachIrSensor(bool& detected) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    detected = m_approachIr.beamBroken;
    return true;
}

bool IntersectionHardware::ReadDepartureIrSensor(bool& detected) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    detected = m_departureIr.beamBroken;
    return true;
}

void IntersectionHardware::InjectApproachDetection() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_approachIr.beamBroken = true;
    m_approachIr.detectionCount++;
    m_approachPulseTimer = 1.5; // 1.5 sec pulse
    m_logger->Info("IR_SENSOR", m_nodeId + ": Optical IR vehicle arrival detected.");
}

void IntersectionHardware::InjectDepartureDetection() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_departureIr.beamBroken = true;
    m_departureIr.detectionCount++;
    m_departurePulseTimer = 1.5;
    m_logger->Info("IR_SENSOR", m_nodeId + ": Optical IR vehicle departure detected.");

    // If preemption is active, vehicle departure clears preemption!
    if (m_isPreempted) {
        m_isPreempted = false;
        m_activeCorridorId.clear();
        m_trafficLights->SetPreemptionStrobe(false);
        m_trafficLights->SetBuzzer(false);
        m_trafficLights->SetPhase(DualPhase::ALL_RED_CLEARANCE);
        m_phaseTimer = m_timings.allRedHoldSec;
        m_logger->Info("PREEMPTION", m_nodeId + ": Departure IR triggered corridor clearance and cycle restoration.");
    }
}

IrSensorStatus IntersectionHardware::GetApproachSensorStatus() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_approachIr;
}

IrSensorStatus IntersectionHardware::GetDepartureSensorStatus() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_departureIr;
}

bool IntersectionHardware::ProcessIrSample(bool rawApproachLevel, bool rawDepartureLevel, uint32_t sampleDtMs) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    bool approachTriggered = m_approachFilter.ProcessSample(rawApproachLevel, sampleDtMs);
    bool departureTriggered = m_departureFilter.ProcessSample(rawDepartureLevel, sampleDtMs);

    if (approachTriggered && m_approachFilter.IsBeamBroken()) {
        InjectApproachDetection();
    }
    if (departureTriggered && m_departureFilter.IsBeamBroken()) {
        InjectDepartureDetection();
    }
    return approachTriggered || departureTriggered;
}

OpticalFilterStats IntersectionHardware::GetApproachFilterStats() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_approachFilter.GetStatistics();
}

OpticalFilterStats IntersectionHardware::GetDepartureFilterStats() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_departureFilter.GetStatistics();
}

} // namespace hardware
} // namespace aegis
