/**
 * @file SignalController.cpp
 * @brief Implementation of SignalController.
 * Contains ZERO GPIO code; delegates exclusively to FSM and LampController.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "traffic_node/signal_controller/SignalController.hpp"

namespace aegis::traffic {

SignalController::SignalController(std::shared_ptr<LampController> lampController)
    : fsm_(std::make_shared<TrafficSignalFSM>(std::move(lampController))) {}

SignalController::SignalController(std::shared_ptr<TrafficSignalFSM> fsm)
    : fsm_(std::move(fsm)) {}

bool SignalController::Initialize(const TrafficNodeConfig& config) {
    if (!fsm_) return false;
    return fsm_->Initialize(config);
}

void SignalController::Shutdown() {
    if (fsm_) {
        fsm_->Shutdown();
    }
}

bool SignalController::CommandGreen(const std::string& reason) {
    if (!fsm_) return false;
    return fsm_->CommandSafeState(SignalState::GREEN, reason);
}

bool SignalController::CommandYellow(const std::string& reason) {
    if (!fsm_) return false;
    return fsm_->CommandSafeState(SignalState::YELLOW, reason);
}

bool SignalController::CommandRed(const std::string& reason) {
    if (!fsm_) return false;
    return fsm_->CommandSafeState(SignalState::RED, reason);
}

bool SignalController::CommandAllRed(const std::string& reason) {
    if (!fsm_) return false;
    return fsm_->CommandSafeState(SignalState::ALL_RED, reason);
}

void SignalController::CommandEmergencyMode(const std::string& corridorId) {
    if (fsm_) {
        fsm_->EnterEmergencyMode(corridorId);
    }
}

void SignalController::CommandNormalMode(const std::string& reason) {
    if (fsm_) {
        fsm_->RestoreNormalMode(reason);
    }
}

SignalState SignalController::GetCurrentState() const {
    return fsm_ ? fsm_->GetCurrentState() : SignalState::ALL_RED;
}

OperationalMode SignalController::GetCurrentMode() const {
    return fsm_ ? fsm_->GetCurrentMode() : OperationalMode::FAILSAFE_MODE;
}

bool SignalController::IsInEmergencyMode() const {
    return fsm_ ? fsm_->IsInEmergencyMode() : false;
}

} // namespace aegis::traffic

