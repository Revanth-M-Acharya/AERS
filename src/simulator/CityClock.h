/**
 * @file CityClock.h
 * @brief SMART AEGIS (AERS) Phase 19 - Deterministic City Simulation Clock.
 */

#ifndef AEGIS_SIMULATOR_CITY_CLOCK_H
#define AEGIS_SIMULATOR_CITY_CLOCK_H

#include <cstdint>
#include <string>
#include <mutex>

namespace aegis {
namespace simulator {

/**
 * @brief Simulation speed multiplier enumeration.
 */
enum class SimulationSpeedMultiplier : uint8_t {
    SPEED_1X = 1,
    SPEED_2X = 2,
    SPEED_5X = 5,
    SPEED_10X = 10
};

/**
 * @brief Thread-safe high-precision virtual clock governing the digital twin world.
 */
class CityClock {
public:
    explicit CityClock(double startHour = 8.0, double timeScale = 1.0);
    ~CityClock() = default;

    // Simulation Controls
    void SetTimeScale(double scale);
    double GetTimeScale() const;

    void SetSpeedMultiplier(SimulationSpeedMultiplier multiplier);
    SimulationSpeedMultiplier GetSpeedMultiplier() const;

    void Pause();
    void Resume();
    void TogglePause();
    bool IsPaused() const;

    void Reset(double startHour = 8.0);
    void StepSingleTick(double dtSec = 0.05);

    // Time Advance
    void Advance(double wallDtSec);

    // Getters
    double GetElapsedSimSeconds() const;
    uint64_t GetTickCount() const;
    double GetCurrentHourOfDay() const;
    std::string GetTimeString() const; // "HH:MM:SS"
    std::string GetIsoString() const;

private:
    mutable std::mutex m_mutex;
    double m_timeScale{1.0};
    bool m_isPaused{false};
    double m_elapsedSimSeconds{0.0};
    double m_currentHourOfDay{8.0};
    uint64_t m_tickCount{0};
};

} // namespace simulator
} // namespace aegis

#endif // AEGIS_SIMULATOR_CITY_CLOCK_H

