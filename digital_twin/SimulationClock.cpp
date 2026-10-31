/**
 * @file SimulationClock.cpp
 * @brief Implementation of SimulationClock virtual timekeeper.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "digital_twin/SimulationClock.hpp"
#include "common/utils/TimeUtils.hpp"
#include <algorithm>

namespace aegis::digital_twin {

std::string ClockStateToString(ClockState state) {
    switch (state) {
        case ClockState::STOPPED: return "STOPPED";
        case ClockState::RUNNING: return "RUNNING";
        case ClockState::PAUSED:  return "PAUSED";
    }
    return "UNKNOWN";
}

SimulationClock::SimulationClock() {
    Reset();
}

void SimulationClock::Start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == ClockState::STOPPED) {
        startWallMonoMs_ = common::TimeUtils::GetMonotonicTimeMs();
        lastWallTickMonoMs_ = startWallMonoMs_;
    } else if (state_ == ClockState::PAUSED) {
        lastWallTickMonoMs_ = common::TimeUtils::GetMonotonicTimeMs();
    }
    state_ = ClockState::RUNNING;
}

void SimulationClock::Pause() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == ClockState::RUNNING) {
        uint64_t now = common::TimeUtils::GetMonotonicTimeMs();
        if (now > lastWallTickMonoMs_) {
            uint64_t deltaMs = now - lastWallTickMonoMs_;
            elapsedSimSeconds_ += (static_cast<double>(deltaMs) / 1000.0) * speedMultiplier_;
            totalWallElapsedMs_ += deltaMs;
        }
        lastWallTickMonoMs_ = now;
        state_ = ClockState::PAUSED;
    }
}

void SimulationClock::Resume() {
    Start();
}

void SimulationClock::Stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    state_ = ClockState::STOPPED;
}

void SimulationClock::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    state_ = ClockState::STOPPED;
    speedMultiplier_ = 1.0;
    elapsedSimSeconds_ = 0.0;
    startWallMonoMs_ = 0;
    lastWallTickMonoMs_ = 0;
    totalWallElapsedMs_ = 0;
}

void SimulationClock::Step(double stepSeconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    elapsedSimSeconds_ += std::max(0.0, stepSeconds);
}

void SimulationClock::SetSpeedMultiplier(double multiplier) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Flush accrued time at old speed before changing multiplier
    if (state_ == ClockState::RUNNING) {
        uint64_t now = common::TimeUtils::GetMonotonicTimeMs();
        if (now > lastWallTickMonoMs_) {
            uint64_t deltaMs = now - lastWallTickMonoMs_;
            elapsedSimSeconds_ += (static_cast<double>(deltaMs) / 1000.0) * speedMultiplier_;
            totalWallElapsedMs_ += deltaMs;
            lastWallTickMonoMs_ = now;
        }
    }
    speedMultiplier_ = std::clamp(multiplier, 0.1, 50.0);
}

double SimulationClock::GetSpeedMultiplier() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return speedMultiplier_;
}

double SimulationClock::GetElapsedSimSeconds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == ClockState::RUNNING) {
        uint64_t now = common::TimeUtils::GetMonotonicTimeMs();
        if (now > lastWallTickMonoMs_) {
            uint64_t deltaMs = now - lastWallTickMonoMs_;
            return elapsedSimSeconds_ + (static_cast<double>(deltaMs) / 1000.0) * speedMultiplier_;
        }
    }
    return elapsedSimSeconds_;
}

uint64_t SimulationClock::GetElapsedSimMonoMs() const {
    return static_cast<uint64_t>(GetElapsedSimSeconds() * 1000.0);
}

double SimulationClock::GetElapsedWallSeconds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == ClockState::RUNNING && lastWallTickMonoMs_ > 0) {
        uint64_t now = common::TimeUtils::GetMonotonicTimeMs();
        uint64_t addMs = (now > lastWallTickMonoMs_) ? (now - lastWallTickMonoMs_) : 0;
        return static_cast<double>(totalWallElapsedMs_ + addMs) / 1000.0;
    }
    return static_cast<double>(totalWallElapsedMs_) / 1000.0;
}

ClockState SimulationClock::GetState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

bool SimulationClock::IsRunning() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_ == ClockState::RUNNING;
}

bool SimulationClock::IsPaused() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_ == ClockState::PAUSED;
}

void SimulationClock::Update() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == ClockState::RUNNING) {
        uint64_t now = common::TimeUtils::GetMonotonicTimeMs();
        if (now > lastWallTickMonoMs_) {
            uint64_t deltaMs = now - lastWallTickMonoMs_;
            elapsedSimSeconds_ += (static_cast<double>(deltaMs) / 1000.0) * speedMultiplier_;
            totalWallElapsedMs_ += deltaMs;
            lastWallTickMonoMs_ = now;
        }
    }
}

} // namespace aegis::digital_twin

