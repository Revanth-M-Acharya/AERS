/**
 * @file traffic_controller.cpp
 * @brief Implementation of TrafficController coordinating FSM, preemption, recovery, and GPIO.
 */

#include "aegis/edge/traffic_controller.hpp"
#include "aegis/logger/logger.hpp"

namespace aegis {
namespace edge {
namespace traffic {

TrafficController::TrafficController()
    : m_stateMachine(SignalState::RED),
      m_gpio(CreateSimulationGpio()),
      m_config(TrafficTimingConfig{}) {}

TrafficController::TrafficController(std::unique_ptr<IGpioInterface> gpio,
                                     const TrafficTimingConfig& config)
    : m_stateMachine(SignalState::RED),
      m_gpio(gpio ? std::move(gpio) : CreateSimulationGpio()),
      m_config(config) {}

TrafficController::~TrafficController() {
    Shutdown();
}

bool TrafficController::Initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isInitialized) {
        return true;
    }

    if (!m_gpio) {
        m_gpio = CreateSimulationGpio();
    }

    if (!m_gpio->Initialize()) {
        logger::Logger::Instance().Log(
            logger::LogLevel::ERR, "TRAFFIC_CTRL", "Failed to initialize GPIO interface.");
        return false;
    }

    m_stateMachine.Reset(SignalState::RED);
    m_stateElapsedMs = 0;
    ApplyGpioLampState(SignalState::RED);
    m_isInitialized = true;

    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "TRAFFIC_CTRL", "TrafficController initialized successfully.");
    return true;
}

bool TrafficController::Initialize(const std::string& intersectionId, uint32_t yellowClearanceMs) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_intersectionId = intersectionId;
        m_config.yellowDurationMs = yellowClearanceMs;
    }
    return Initialize();
}

bool TrafficController::Initialize(const TrafficTimingConfig& config) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_config = config;
    }
    return Initialize();
}

bool TrafficController::Start() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_isInitialized) {
        if (!m_gpio) {
            m_gpio = CreateSimulationGpio();
        }
        m_gpio->Initialize();
        m_stateMachine.Reset(SignalState::RED);
        m_isInitialized = true;
    }

    m_isRunning = true;
    m_stateElapsedMs = 0;
    ApplyGpioLampState(m_stateMachine.GetCurrentState());

    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "TRAFFIC_CTRL", "TrafficController started.");
    return true;
}

void TrafficController::Stop() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isRunning = false;
    if (m_gpio) {
        m_gpio->SetRed();
    }
    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "TRAFFIC_CTRL", "TrafficController stopped.");
}

void TrafficController::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isRunning = false;
    m_preemptionManager.CancelEmergency();
    m_recoveryManager.Reset();
    m_stateMachine.Reset(SignalState::UNKNOWN);

    if (m_gpio) {
        m_gpio->AllOff();
        m_gpio->Shutdown();
    }

    m_isInitialized = false;
    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "TRAFFIC_CTRL", "TrafficController shut down.");
}

void TrafficController::Tick(uint32_t deltaMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_isInitialized || !m_isRunning) {
        return;
    }

    m_stateElapsedMs += deltaMs;
    SignalState current = m_stateMachine.GetCurrentState();

    if (m_preemptionManager.IsEmergencyActive()) {
        // Emergency cycle: Current -> ALL_RED -> GREEN -> Hold Green
        if (current == SignalState::ALL_RED) {
            if (m_stateElapsedMs >= m_config.allRedClearanceMs) {
                if (m_stateMachine.TransitionTo(SignalState::GREEN)) {
                    m_stateElapsedMs = 0;
                    ApplyGpioLampState(SignalState::GREEN);
                }
            }
        } else if (current != SignalState::GREEN) {
            if (m_stateMachine.TransitionTo(SignalState::ALL_RED)) {
                m_stateElapsedMs = 0;
                ApplyGpioLampState(SignalState::ALL_RED);
            }
        }
        // When in GREEN during emergency, hold green indefinitely
        return;
    }

    if (m_recoveryManager.IsRecoveryActive()) {
        // Recovery cycle: Hold Green -> Recovery -> YELLOW -> RED
        if (current == SignalState::GREEN) {
            if (m_stateElapsedMs >= m_config.recoveryDurationMs) {
                m_recoveryManager.RecoveryFinished();
                if (m_stateMachine.TransitionTo(SignalState::YELLOW)) {
                    m_stateElapsedMs = 0;
                    ApplyGpioLampState(SignalState::YELLOW);
                }
            }
        } else if (current == SignalState::ALL_RED) {
            m_recoveryManager.RecoveryFinished();
            if (m_stateMachine.TransitionTo(SignalState::RED)) {
                m_stateElapsedMs = 0;
                ApplyGpioLampState(SignalState::RED);
            }
        }
        return;
    }

    // Normal cycle: RED -> GREEN -> YELLOW -> RED
    switch (current) {
        case SignalState::RED:
            if (m_stateElapsedMs >= m_config.redDurationMs) {
                if (m_stateMachine.TransitionTo(SignalState::GREEN)) {
                    m_stateElapsedMs = 0;
                    ApplyGpioLampState(SignalState::GREEN);
                }
            }
            break;

        case SignalState::GREEN:
            if (m_stateElapsedMs >= m_config.greenDurationMs) {
                if (m_stateMachine.TransitionTo(SignalState::YELLOW)) {
                    m_stateElapsedMs = 0;
                    ApplyGpioLampState(SignalState::YELLOW);
                }
            }
            break;

        case SignalState::YELLOW:
            if (m_stateElapsedMs >= m_config.yellowDurationMs) {
                if (m_stateMachine.TransitionTo(SignalState::RED)) {
                    m_stateElapsedMs = 0;
                    ApplyGpioLampState(SignalState::RED);
                }
            }
            break;

        case SignalState::ALL_RED:
            if (m_stateMachine.TransitionTo(SignalState::RED)) {
                m_stateElapsedMs = 0;
                ApplyGpioLampState(SignalState::RED);
            }
            break;

        default:
            break;
    }
}

bool TrafficController::RequestPreemption() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_isInitialized) {
        return false;
    }

    m_preemptionManager.StartEmergency();
    m_recoveryManager.Reset();

    // Emergency cycle: Current -> ALL_RED -> GREEN -> Hold Green
    if (m_stateMachine.GetCurrentState() != SignalState::ALL_RED) {
        if (m_stateMachine.TransitionTo(SignalState::ALL_RED)) {
            m_stateElapsedMs = 0;
            ApplyGpioLampState(SignalState::ALL_RED);
        }
    }
    return true;
}

bool TrafficController::ClearPreemption() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_isInitialized || !m_preemptionManager.IsEmergencyActive()) {
        return false;
    }

    m_preemptionManager.CancelEmergency();
    m_recoveryManager.StartRecovery();
    m_stateElapsedMs = 0;
    return true;
}

SignalState TrafficController::GetCurrentState() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stateMachine.GetCurrentState();
}

std::string TrafficController::GetCurrentStateString() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stateMachine.GetCurrentStateString();
}

bool TrafficController::IsPreemptionActive() const noexcept {
    return m_preemptionManager.IsEmergencyActive();
}

bool TrafficController::IsRecoveryActive() const noexcept {
    return m_recoveryManager.IsRecoveryActive();
}

void TrafficController::SetTimingConfig(const TrafficTimingConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
}

void TrafficController::ApplyGpioLampState(SignalState state) {
    if (!m_gpio) {
        return;
    }

    switch (state) {
        case SignalState::RED:
        case SignalState::ALL_RED:
            m_gpio->SetRed();
            break;
        case SignalState::YELLOW:
            m_gpio->SetYellow();
            break;
        case SignalState::GREEN:
            m_gpio->SetGreen();
            break;
        case SignalState::FLASHING_YELLOW:
            m_gpio->FlashYellow();
            break;
        case SignalState::FLASHING_RED:
            m_gpio->FlashRed();
            break;
        case SignalState::UNKNOWN:
        default:
            m_gpio->AllOff();
            break;
    }
}

} // namespace traffic
} // namespace edge
} // namespace aegis
