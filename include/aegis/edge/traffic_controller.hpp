/**
 * @file traffic_controller.hpp
 * @brief Roadside Unit (RSU) traffic intersection controller coordinating FSM, preemption, recovery, and GPIO.
 */

#ifndef AEGIS_EDGE_TRAFFIC_CONTROLLER_HPP
#define AEGIS_EDGE_TRAFFIC_CONTROLLER_HPP

#include <string>
#include <memory>
#include <cstdint>
#include <mutex>

#include "aegis/edge/gpio_interface.hpp"
#include "aegis/edge/signal_state_machine.hpp"
#include "aegis/edge/preemption_manager.hpp"
#include "aegis/edge/recovery_manager.hpp"

namespace aegis {
namespace edge {
namespace traffic {

/**
 * @brief Configurable timing intervals for traffic signal phases.
 * Eliminates all hardcoded magic numbers.
 */
struct TrafficTimingConfig {
    uint32_t redDurationMs{5000};
    uint32_t greenDurationMs{5000};
    uint32_t yellowDurationMs{3000};
    uint32_t allRedClearanceMs{2000};
    uint32_t emergencyHoldDurationMs{10000};
    uint32_t recoveryDurationMs{3000};
};

/**
 * @brief Deterministic traffic signal controller coordinating signal states, preemption, and GPIO.
 */
class TrafficController {
public:
    TrafficController();
    explicit TrafficController(std::unique_ptr<IGpioInterface> gpio,
                              const TrafficTimingConfig& config = TrafficTimingConfig{});
    ~TrafficController();

    // Lifecycle
    bool Initialize();
    bool Initialize(const std::string& intersectionId, uint32_t yellowClearanceMs);
    bool Initialize(const TrafficTimingConfig& config);
    bool Start();
    void Stop();
    void Shutdown();

    // State Progression
    void Tick(uint32_t deltaMs = 100);

    // Preemption & Fail-Safe Controls
    bool RequestPreemption();
    bool ClearPreemption();
    bool EnterFailSafe(SignalState failSafeState = SignalState::FLASHING_YELLOW);
    bool RestoreNormalCycle();

    // State Queries
    SignalState GetCurrentState() const;
    std::string GetCurrentStateString() const;
    bool IsPreemptionActive() const noexcept;
    bool IsRecoveryActive() const noexcept;
    bool IsFailSafeActive() const noexcept;

    // Subsystem Accessors (for inspection/testing)
    const SignalStateMachine& GetStateMachine() const noexcept { return m_stateMachine; }
    SignalStateMachine& GetStateMachine() noexcept { return m_stateMachine; }
    const PreemptionManager& GetPreemptionManager() const noexcept { return m_preemptionManager; }
    const RecoveryManager& GetRecoveryManager() const noexcept { return m_recoveryManager; }
    const IGpioInterface& GetGpio() const noexcept { return *m_gpio; }
    const TrafficTimingConfig& GetTimingConfig() const noexcept { return m_config; }
    void SetTimingConfig(const TrafficTimingConfig& config);

private:
    void ApplyGpioLampState(SignalState state);

    mutable std::mutex m_mutex;
    SignalStateMachine m_stateMachine;
    PreemptionManager m_preemptionManager;
    RecoveryManager m_recoveryManager;
    std::unique_ptr<IGpioInterface> m_gpio;

    TrafficTimingConfig m_config;
    uint32_t m_stateElapsedMs{0};
    bool m_isInitialized{false};
    bool m_isRunning{false};
    bool m_isFailSafeActive{false};
    std::string m_intersectionId{"INT-01"};
};

} // namespace traffic
} // namespace edge
} // namespace aegis

#endif // AEGIS_EDGE_TRAFFIC_CONTROLLER_HPP
