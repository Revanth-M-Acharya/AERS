#include "traffic_controller.hpp"

namespace aegis {
namespace traffic {

TrafficController::TrafficController() = default;
TrafficController::~TrafficController() = default;

bool TrafficController::Start() {
    return m_gpio.Initialize();
}

void TrafficController::Stop() {
    FallbackToFailSafe();
}

void TrafficController::HandlePreemptionRequest(const std::string& /*unitId*/, uint32_t /*holdSeconds*/) {
    // Placeholder: Enforce yellow clearance, then transition to green corridor
}

void TrafficController::FallbackToFailSafe() {
    m_mode = ControllerMode::FAILSAFE_FLASHING_AMBER;
    m_gpio.SetLampState(common::SignalPhase::FLASHING_AMBER, common::SignalPhase::FLASHING_AMBER);
}

} // namespace traffic
} // namespace aegis
