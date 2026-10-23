#include "aegis/edge/traffic_controller.hpp"

namespace aegis {
namespace edge {
namespace traffic {

TrafficController::TrafficController() = default;
TrafficController::~TrafficController() = default;

bool TrafficController::Initialize(const std::string& /*intersectionId*/, uint32_t /*yellowClearanceMs*/) {
    return m_gpio.Initialize();
}

bool TrafficController::Start() {
    // TODO: Start deterministic FSM loop on high-priority QNX thread
    return true;
}

void TrafficController::Stop() {
    FallbackToFailSafe();
}

bool TrafficController::HandlePreemptionRequest(const std::string& /*unitId*/, common::SignalPhase /*requestedPhase*/, uint32_t /*holdDurationSec*/) {
    // TODO: Enforce yellow clearance (3s), all-red (1s), then illuminate green corridor
    return true;
}

void TrafficController::ReleasePreemption() {
    // TODO: Restore normal civil phase sequence
}

void TrafficController::FallbackToFailSafe() {
    m_state = ControllerFsmState::FAILSAFE_FLASHING_AMBER;
    m_gpio.SetLampState(common::SignalPhase::FLASHING_AMBER, common::SignalPhase::FLASHING_AMBER);
}

ControllerFsmState TrafficController::GetCurrentState() const {
    return m_state;
}

} // namespace traffic
} // namespace edge
} // namespace aegis
