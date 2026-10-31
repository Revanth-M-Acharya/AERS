/**
 * @file TrafficSignalFSM.cpp
 * @brief Implementation of deterministic safety-critical traffic signal FSM.
 * Contains ZERO GPIO code; delegates exclusively to LampController.
 * Enforces legal state transitions without generating autonomous cycles.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "traffic_node/signal_controller/TrafficSignalFSM.hpp"
#include "traffic_node/logger/TrafficLogger.hpp"
#include "common/utils/ThreadUtils.hpp"
#include "common/utils/TimeUtils.hpp"

namespace aegis::traffic {

TrafficSignalFSM::TrafficSignalFSM(std::shared_ptr<LampController> lampController)
    : lampController_(std::move(lampController)) {}

TrafficSignalFSM::TrafficSignalFSM(std::shared_ptr<RelayController> relayController)
    : lampController_(std::make_shared<LampController>(std::move(relayController))) {}

TrafficSignalFSM::~TrafficSignalFSM() {
    Shutdown();
}

bool TrafficSignalFSM::Initialize(const TrafficNodeConfig& config) {
    std::lock_guard<std::mutex> lock(fsmMutex_);
    config_ = config;

    if (!lampController_) {
        TrafficLogger::Instance().Error("FSM", "Cannot initialize FSM: LampController is null");
        return false;
    }

    lampController_->Initialize(config_.relaySwitchDelayMs);

    // Boot Requirement: Always initialize in safe stop state (RED)
    currentState_ = SignalState::RED;
    currentMode_ = OperationalMode::NORMAL_MODE;
    ApplyLampHardware(currentState_);

    isRunning_.store(true);
    fsmThread_ = std::thread([this]() {
        common::ThreadUtils::SetCurrentThreadName("TrafficFsmLp");
        while (isRunning_.load()) {
            std::unique_lock<std::mutex> lk(fsmMutex_);
            fsmCv_.wait_for(lk, std::chrono::milliseconds(500), [this]() {
                return !isRunning_.load();
            });
            if (petCallback_) {
                petCallback_();
            }
        }
    });

    TrafficLogger::Instance().Info("FSM", "Traffic Signal FSM initialized in safe default (ALL_RED).");
    return true;
}

void TrafficSignalFSM::Shutdown() {
    if (!isRunning_.exchange(false)) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(fsmMutex_);
        fsmCv_.notify_all();
    }

    if (fsmThread_.joinable()) {
        fsmThread_.join();
    }

    if (lampController_) {
        lampController_->Shutdown();
    }

    TrafficLogger::Instance().Info("FSM", "Traffic Signal FSM shut down cleanly.");
}

SignalState TrafficSignalFSM::GetCurrentState() const {
    std::lock_guard<std::mutex> lock(fsmMutex_);
    return currentState_;
}

OperationalMode TrafficSignalFSM::GetCurrentMode() const {
    std::lock_guard<std::mutex> lock(fsmMutex_);
    return currentMode_;
}

bool TrafficSignalFSM::IsInEmergencyMode() const {
    std::lock_guard<std::mutex> lock(fsmMutex_);
    return (currentMode_ == OperationalMode::EMERGENCY_MODE);
}

bool TrafficSignalFSM::IsTransitionLegal(SignalState from, SignalState to) {
    if (from == to) {
        return true; // Idempotent
    }

    switch (from) {
        case SignalState::GREEN:
            // CRITICAL SAFETY RULE: Never transition directly from GREEN to RED or ALL_RED!
            return (to == SignalState::YELLOW);

        case SignalState::YELLOW:
            // Yellow clears to RED or ALL_RED
            return (to == SignalState::RED || to == SignalState::ALL_RED);

        case SignalState::RED:
            return (to == SignalState::RED_YELLOW || to == SignalState::ALL_RED || to == SignalState::FLASHING_YELLOW);

        case SignalState::RED_YELLOW:
            return (to == SignalState::GREEN || to == SignalState::ALL_RED);

        case SignalState::ALL_RED:
            return (to == SignalState::RED || to == SignalState::GREEN ||
                    to == SignalState::RED_YELLOW || to == SignalState::FLASHING_YELLOW);

        case SignalState::FLASHING_YELLOW:
            return (to == SignalState::ALL_RED || to == SignalState::RED);

        default:
            return false;
    }
}

bool TrafficSignalFSM::RequestTransition(SignalState targetState, const std::string& reason) {
    std::lock_guard<std::mutex> lock(fsmMutex_);
    if (currentState_ == targetState) {
        return true;
    }

    if (!IsTransitionLegal(currentState_, targetState)) {
        TrafficLogger::Instance().LogIllegalTransition(SignalStateToString(currentState_),
                                                       SignalStateToString(targetState));
        return false;
    }

    SignalState previous = currentState_;
    currentState_ = targetState;
    ApplyLampHardware(currentState_);

    TrafficLogger::Instance().LogFsmTransition(SignalStateToString(previous),
                                               SignalStateToString(currentState_),
                                               reason);
    return true;
}

bool TrafficSignalFSM::CommandSafeState(SignalState targetState, const std::string& reason) {
    std::unique_lock<std::mutex> lock(fsmMutex_);

    if (currentState_ == targetState) {
        return true;
    }

    // If currently GREEN and requested state is RED or ALL_RED, transition safely through YELLOW
    if (currentState_ == SignalState::GREEN && (targetState == SignalState::RED || targetState == SignalState::ALL_RED)) {
        SignalState prev = currentState_;
        currentState_ = SignalState::YELLOW;
        ApplyLampHardware(currentState_);
        TrafficLogger::Instance().LogFsmTransition(SignalStateToString(prev), "YELLOW", "Safety clearance before " + SignalStateToString(targetState));

        // Unlock during clearance duration
        lock.unlock();
        common::TimeUtils::SleepMs(config_.yellowDurationMs > 0 ? config_.yellowDurationMs : 1000);
        lock.lock();

        prev = currentState_;
        currentState_ = targetState;
        ApplyLampHardware(currentState_);
        TrafficLogger::Instance().LogFsmTransition(SignalStateToString(prev), SignalStateToString(currentState_), reason);
        return true;
    }

    // If currently RED and requested state is GREEN, transition safely through RED_YELLOW
    if (currentState_ == SignalState::RED && targetState == SignalState::GREEN) {
        SignalState prev = currentState_;
        currentState_ = SignalState::RED_YELLOW;
        ApplyLampHardware(currentState_);
        TrafficLogger::Instance().LogFsmTransition(SignalStateToString(prev), "RED_YELLOW", "Clearance sequence before GREEN");

        lock.unlock();
        common::TimeUtils::SleepMs(config_.allRedDurationMs > 0 ? std::min<uint32_t>(config_.allRedDurationMs, 500) : 100);
        lock.lock();

        prev = currentState_;
        currentState_ = SignalState::GREEN;
        ApplyLampHardware(currentState_);
        TrafficLogger::Instance().LogFsmTransition(SignalStateToString(prev), "GREEN", reason);
        return true;
    }

    // Direct transition attempt if legally permissible
    if (IsTransitionLegal(currentState_, targetState)) {
        SignalState prev = currentState_;
        currentState_ = targetState;
        ApplyLampHardware(currentState_);
        TrafficLogger::Instance().LogFsmTransition(SignalStateToString(prev), SignalStateToString(currentState_), reason);
        return true;
    }

    // Illegal direct transition rejected
    TrafficLogger::Instance().LogIllegalTransition(SignalStateToString(currentState_), SignalStateToString(targetState));
    return false;
}

void TrafficSignalFSM::EnterEmergencyMode(const std::string& corridorId) {
    TrafficLogger::Instance().Warn("FSM", "[EMERGENCY_PREEMPTION] Entering EMERGENCY_MODE for Corridor: " + corridorId);

    {
        std::lock_guard<std::mutex> lock(fsmMutex_);
        currentMode_ = OperationalMode::EMERGENCY_MODE;
    }

    // Clear active conflicts safely
    CommandSafeState(SignalState::ALL_RED, "Emergency preemption clearance");

    // Establish Emergency Green for ambulance corridor
    RequestTransition(SignalState::GREEN, "Corridor Preemption Active: " + corridorId);
}

void TrafficSignalFSM::RestoreNormalMode(const std::string& reason) {
    TrafficLogger::Instance().Info("FSM", "[NORMAL_RESTORE] Restoring NORMAL_MODE. Reason: " + reason);

    // Clear green safely to all-red before restoring
    CommandSafeState(SignalState::ALL_RED, "Emergency corridor release clearance");

    {
        std::lock_guard<std::mutex> lock(fsmMutex_);
        currentMode_ = OperationalMode::NORMAL_MODE;
        fsmCv_.notify_all();
    }
}

void TrafficSignalFSM::ApplyLampHardware(SignalState state) {
    if (!lampController_) return;

    switch (state) {
        case SignalState::RED:
            lampController_->DisplayRed();
            break;
        case SignalState::YELLOW:
            lampController_->DisplayYellow();
            break;
        case SignalState::GREEN:
            lampController_->DisplayGreen();
            break;
        case SignalState::ALL_RED:
            lampController_->DisplayAllRed();
            break;
        case SignalState::FLASHING_YELLOW:
            lampController_->DisplayFlashingYellow();
            break;
        case SignalState::RED_YELLOW:
            lampController_->DisplayRed();
            break;
    }
}

} // namespace aegis::traffic
