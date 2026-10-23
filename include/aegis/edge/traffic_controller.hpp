#ifndef AEGIS_EDGE_TRAFFIC_CONTROLLER_HPP
#define AEGIS_EDGE_TRAFFIC_CONTROLLER_HPP

#include <string>
#include <cstdint>
#include "aegis/common/types.hpp"
#include "aegis/edge/gpio_interface.hpp"

namespace aegis {
namespace edge {
namespace traffic {

enum class ControllerFsmState : uint8_t {
    NORMAL_CYCLE = 0,
    PREEMPTION_PREPARE,
    GREEN_CORRIDOR,
    RESTORE,
    FAILSAFE_FLASHING_AMBER
};

/**
 * @brief Deterministic Roadside Unit (RSU) traffic intersection controller.
 * TODO: Implement deterministic phase FSM with yellow and all-red clearance guarantees.
 */
class TrafficController {
public:
    TrafficController();
    ~TrafficController();

    bool Initialize(const std::string& intersectionId, uint32_t yellowClearanceMs);
    bool Start();
    void Stop();
    
    bool HandlePreemptionRequest(const std::string& unitId, common::SignalPhase requestedPhase, uint32_t holdDurationSec);
    void ReleasePreemption();
    void FallbackToFailSafe();

    ControllerFsmState GetCurrentState() const;

private:
    GpioInterface m_gpio;
    ControllerFsmState m_state{ControllerFsmState::NORMAL_CYCLE};
};

} // namespace traffic
} // namespace edge
} // namespace aegis

#endif // AEGIS_EDGE_TRAFFIC_CONTROLLER_HPP
