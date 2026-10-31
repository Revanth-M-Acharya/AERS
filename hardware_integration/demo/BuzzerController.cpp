/**
 * @file BuzzerController.cpp
 * @brief Implementation of non-blocking hardware buzzer controller.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "hardware_integration/demo/BuzzerController.hpp"
#include <vector>
#include <chrono>

namespace aegis::hardware_integration::demo {

namespace {
struct ToneStep {
    bool on;
    uint32_t durationMs;
};

std::vector<ToneStep> GetPatternSteps(BuzzerPattern pattern) {
    switch (pattern) {
        case BuzzerPattern::CONTINUOUS_TONE:
            return {{true, 5000}}; // Kept on indefinitely or in chunks
        case BuzzerPattern::THREE_SHORT_BEEPS:
            return {
                {true, 120}, {false, 100},
                {true, 120}, {false, 100},
                {true, 120}, {false, 200}
            };
        case BuzzerPattern::RAPID_BEEPS:
            return {
                {true, 60}, {false, 60},
                {true, 60}, {false, 60},
                {true, 60}, {false, 60},
                {true, 60}, {false, 60},
                {true, 60}, {false, 60},
                {true, 60}, {false, 150}
            };
        case BuzzerPattern::TWO_LONG_BEEPS:
            return {
                {true, 450}, {false, 200},
                {true, 450}, {false, 300}
            };
        case BuzzerPattern::ALTERNATING_WARNING:
            return {
                {true, 250}, {false, 100},
                {true, 150}, {false, 100}
            };
        case BuzzerPattern::OFF:
        default:
            return {{false, 100}};
    }
}
} // namespace

BuzzerController::BuzzerController(
    std::shared_ptr<SymbolicGpioHAL> hal,
    const std::string& symbolicSignal)
    : hal_(std::move(hal)),
      symbolicSignal_(symbolicSignal) {
}

BuzzerController::~BuzzerController() {
    Stop();
    if (workerThread_.joinable()) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            isRunning_ = false;
        }
        cv_.notify_all();
        workerThread_.join();
    }
    SetBuzzerState(false);
}

bool BuzzerController::Initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!hal_) {
        return false;
    }

    if (!hal_->IsSignalRegistered(symbolicSignal_)) {
        // Register default direct-drive active-high buzzer if not present
        hal_->RegisterSignal(
            symbolicSignal_,
            6, // BCM GPIO 6
            PinDirection::OUTPUT,
            ActivePolarity::ACTIVE_HIGH,
            "Coordinator Preemption Buzzer");
    }

    SetBuzzerState(false);
    isRunning_ = true;
    workerThread_ = std::thread(&BuzzerController::WorkerLoop, this);
    return true;
}

void BuzzerController::PlayPattern(BuzzerPattern pattern, uint32_t repeatCount) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (pattern == BuzzerPattern::OFF) {
        currentPattern_ = BuzzerPattern::OFF;
        isPlaying_ = false;
        SetBuzzerState(false);
        cv_.notify_all();
        return;
    }

    currentPattern_ = pattern;
    repeatCount_ = repeatCount;
    currentRepeat_ = 0;
    phaseIndex_ = 0;
    phaseElapsedMs_ = 0;
    isPlaying_ = true;
    cv_.notify_all();
}

void BuzzerController::Stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    currentPattern_ = BuzzerPattern::OFF;
    isPlaying_ = false;
    phaseIndex_ = 0;
    phaseElapsedMs_ = 0;
    SetBuzzerState(false);
    cv_.notify_all();
}

bool BuzzerController::IsPlaying() const {
    return isPlaying_;
}

BuzzerPattern BuzzerController::GetCurrentPattern() const {
    return currentPattern_;
}

uint64_t BuzzerController::GetToggleCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return toggleCount_;
}

void BuzzerController::SetBuzzerState(bool on) {
    if (pinState_ != on) {
        pinState_ = on;
        toggleCount_++;
        if (hal_) {
            hal_->SetSignal(symbolicSignal_, on);
        }
    }
}

void BuzzerController::StepSimulation(uint32_t elapsedMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!isPlaying_ || currentPattern_ == BuzzerPattern::OFF) {
        SetBuzzerState(false);
        return;
    }

    if (currentPattern_ == BuzzerPattern::CONTINUOUS_TONE) {
        SetBuzzerState(true);
        return;
    }

    const auto steps = GetPatternSteps(currentPattern_);
    if (steps.empty()) {
        SetBuzzerState(false);
        isPlaying_ = false;
        return;
    }

    phaseElapsedMs_ += elapsedMs;
    while (phaseIndex_ < steps.size() && phaseElapsedMs_ >= steps[phaseIndex_].durationMs) {
        phaseElapsedMs_ -= steps[phaseIndex_].durationMs;
        phaseIndex_++;
    }

    if (phaseIndex_ < steps.size()) {
        SetBuzzerState(steps[phaseIndex_].on);
    } else {
        // Finished one iteration of sequence
        currentRepeat_++;
        if (repeatCount_ == 0 || currentRepeat_ < repeatCount_) {
            phaseIndex_ = 0;
            phaseElapsedMs_ = 0;
            if (!steps.empty()) {
                SetBuzzerState(steps[0].on);
            }
        } else {
            // All repeats complete
            isPlaying_ = false;
            currentPattern_ = BuzzerPattern::OFF;
            SetBuzzerState(false);
        }
    }
}

void BuzzerController::WorkerLoop() {
    while (isRunning_) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!isPlaying_ || currentPattern_ == BuzzerPattern::OFF) {
            SetBuzzerState(false);
            cv_.wait(lock, [this]() {
                return !isRunning_ || (isPlaying_ && currentPattern_ != BuzzerPattern::OFF);
            });
            if (!isRunning_) break;
        }

        if (currentPattern_ == BuzzerPattern::CONTINUOUS_TONE) {
            SetBuzzerState(true);
            cv_.wait_for(lock, std::chrono::milliseconds(50), [this]() {
                return !isRunning_ || !isPlaying_ || currentPattern_ != BuzzerPattern::CONTINUOUS_TONE;
            });
            continue;
        }

        const auto steps = GetPatternSteps(currentPattern_);
        if (steps.empty()) {
            isPlaying_ = false;
            SetBuzzerState(false);
            continue;
        }

        size_t stepIdx = phaseIndex_;
        if (stepIdx >= steps.size()) {
            stepIdx = 0;
            phaseIndex_ = 0;
        }

        const auto currentStep = steps[stepIdx];
        SetBuzzerState(currentStep.on);
        auto waitDuration = std::chrono::milliseconds(currentStep.durationMs);

        BuzzerPattern activePattern = currentPattern_.load();
        bool changed = cv_.wait_for(lock, waitDuration, [this, activePattern]() {
            return !isRunning_ || !isPlaying_ || currentPattern_ != activePattern;
        });

        if (changed || !isRunning_ || !isPlaying_) {
            continue;
        }

        // Advance to next step
        phaseIndex_++;
        if (phaseIndex_ >= steps.size()) {
            currentRepeat_++;
            if (repeatCount_ == 0 || currentRepeat_ < repeatCount_) {
                phaseIndex_ = 0;
            } else {
                isPlaying_ = false;
                currentPattern_ = BuzzerPattern::OFF;
                SetBuzzerState(false);
            }
        }
    }
}

} // namespace aegis::hardware_integration::demo
