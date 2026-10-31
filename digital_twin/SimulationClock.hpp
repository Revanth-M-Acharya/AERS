/**
 * @file SimulationClock.hpp
 * @brief High-precision virtual timekeeper supporting variable playback speeds, pausing, and frame stepping.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <cstdint>
#include <mutex>
#include <string>

namespace aegis::digital_twin {

enum class ClockState {
    STOPPED,
    RUNNING,
    PAUSED
};

std::string ClockStateToString(ClockState state);

class SimulationClock {
public:
    SimulationClock();
    ~SimulationClock() = default;

    void Start();
    void Pause();
    void Resume();
    void Stop();
    void Reset();

    // Advance simulation by a fixed quantum (manual stepping / tick)
    void Step(double stepSeconds);

    // Speed multiplier (e.g. 1.0, 2.0, 5.0, 10.0, 20.0)
    void SetSpeedMultiplier(double multiplier);
    double GetSpeedMultiplier() const;

    // Time queries
    double GetElapsedSimSeconds() const;
    uint64_t GetElapsedSimMonoMs() const;
    double GetElapsedWallSeconds() const;
    ClockState GetState() const;
    bool IsRunning() const;
    bool IsPaused() const;

    // Update called from periodic tick loop
    void Update();

private:
    mutable std::mutex mutex_;
    ClockState state_{ClockState::STOPPED};
    double speedMultiplier_{1.0};
    double elapsedSimSeconds_{0.0};
    uint64_t lastWallTickMonoMs_{0};
    uint64_t startWallMonoMs_{0};
    uint64_t totalWallElapsedMs_{0};
};

} // namespace aegis::digital_twin

