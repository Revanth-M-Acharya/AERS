/**
 * @file TrafficSignalFSM.hpp
 * @brief Deterministic safety-critical Finite State Machine for Traffic Signal Node.
 * Contains ZERO GPIO code; interacts exclusively with LampController.
 * Enforces legal state transitions without generating autonomous cycles.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "traffic_node/signal_controller/TrafficState.hpp"
#include "traffic_node/lamp_controller/LampController.hpp"
#include "traffic_node/config/TrafficNodeConfig.hpp"
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>

namespace aegis::traffic {

class TrafficSignalFSM {
public:
    explicit TrafficSignalFSM(std::shared_ptr<LampController> lampController);
    // Backward-compatible constructor accepting RelayController directly
    explicit TrafficSignalFSM(std::shared_ptr<RelayController> relayController);
    ~TrafficSignalFSM();

    // Prevent copying
    TrafficSignalFSM(const TrafficSignalFSM&) = delete;
    TrafficSignalFSM& operator=(const TrafficSignalFSM&) = delete;

    /**
     * @brief Initialize FSM and place lamps in initial safe state (ALL RED).
     */
    bool Initialize(const TrafficNodeConfig& config);

    /**
     * @brief Stop FSM execution and worker thread cleanly.
     */
    void Shutdown();

    // State & Mode Queries
    SignalState GetCurrentState() const;
    OperationalMode GetCurrentMode() const;
    bool IsInEmergencyMode() const;

    /**
     * @brief Direct state transition. Validates transition safety matrix.
     * Rejects illegal transitions (e.g. GREEN -> RED) with a safety violation event.
     * @param targetState Requested target lamp state.
     * @param reason Human-readable rationale for transition.
     * @return true if legal and transitioned; false if illegal transition rejected.
     */
    bool RequestTransition(SignalState targetState, const std::string& reason = "Manual Request");

    /**
     * @brief Safe multi-step command: if currently GREEN, automatically routes through YELLOW -> ALL_RED -> targetState.
     */
    bool CommandSafeState(SignalState targetState, const std::string& reason = "Safe Clearance");

    /**
     * @brief Transition to Emergency Mode for Ambulance Green Corridor preemption.
     * Safely clears active conflict traffic before locking emergency green corridor.
     */
    void EnterEmergencyMode(const std::string& corridorId = "AMBULANCE_PREEMPTION");

    /**
     * @brief Restore Normal Mode upon corridor completion.
     */
    void RestoreNormalMode(const std::string& reason = "Corridor Released");

    /**
     * @brief Validate whether a state transition is legally permissible under safety rules.
     */
    static bool IsTransitionLegal(SignalState from, SignalState to);

    std::shared_ptr<LampController> GetLampController() const { return lampController_; }

    using FsmPetCallback = std::function<void()>;
    void SetPetCallback(FsmPetCallback cb) {
        std::lock_guard<std::mutex> lk(fsmMutex_);
        petCallback_ = std::move(cb);
    }

private:
    void ApplyLampHardware(SignalState state);

    std::shared_ptr<LampController> lampController_;
    TrafficNodeConfig config_;
    FsmPetCallback petCallback_;

    mutable std::mutex fsmMutex_;
    std::condition_variable fsmCv_;

    SignalState currentState_{SignalState::ALL_RED};
    OperationalMode currentMode_{OperationalMode::NORMAL_MODE};

    std::atomic<bool> isRunning_{false};
    std::thread fsmThread_;
};

} // namespace aegis::traffic
