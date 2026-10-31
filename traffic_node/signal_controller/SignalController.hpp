/**
 * @file SignalController.hpp
 * @brief High-level intersection signal coordinator and execution controller.
 * Contains ZERO GPIO code; delegates exclusively to FSM and LampController.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "traffic_node/signal_controller/TrafficSignalFSM.hpp"
#include "traffic_node/lamp_controller/LampController.hpp"
#include "traffic_node/config/TrafficNodeConfig.hpp"
#include <memory>

namespace aegis::traffic {

class SignalController {
public:
    explicit SignalController(std::shared_ptr<LampController> lampController);
    explicit SignalController(std::shared_ptr<TrafficSignalFSM> fsm);
    ~SignalController() = default;

    // Prevent copying
    SignalController(const SignalController&) = delete;
    SignalController& operator=(const SignalController&) = delete;

    bool Initialize(const TrafficNodeConfig& config);
    void Shutdown();

    // High-Level Signal Commands
    bool CommandGreen(const std::string& reason = "Coordinator GREEN");
    bool CommandYellow(const std::string& reason = "Coordinator YELLOW");
    bool CommandRed(const std::string& reason = "Coordinator RED");
    bool CommandAllRed(const std::string& reason = "Coordinator ALL_RED");
    void CommandEmergencyMode(const std::string& corridorId);
    void CommandNormalMode(const std::string& reason = "Corridor Cleared");

    // Queries
    SignalState GetCurrentState() const;
    OperationalMode GetCurrentMode() const;
    bool IsInEmergencyMode() const;

    std::shared_ptr<TrafficSignalFSM> GetFsm() const { return fsm_; }
    void SetPetCallback(std::function<void()> cb) {
        if (fsm_) fsm_->SetPetCallback(std::move(cb));
    }

private:
    std::shared_ptr<TrafficSignalFSM> fsm_;
};

} // namespace aegis::traffic
