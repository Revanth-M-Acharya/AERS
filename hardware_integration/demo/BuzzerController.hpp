/**
 * @file BuzzerController.hpp
 * @brief Non-blocking hardware buzzer controller supporting 5 demonstration rhythm patterns.
 * Eliminates busy-waiting and runs asynchronously without blocking coordinator dispatching.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "hardware_integration/gpio/SymbolicGpioHAL.hpp"
#include "hardware_integration/demo/DemoHardwareSignals.hpp"

#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdint>

namespace aegis::hardware_integration::demo {

/**
 * @brief Standard acoustic preemption patterns mandated for SMART AEGIS demonstration.
 */
enum class BuzzerPattern {
    OFF,
    CONTINUOUS_TONE,     // Feature 1: Emergency Mission Created / Corridor Active
    THREE_SHORT_BEEPS,   // Feature 3: Intersection 2 Priority Conflict
    RAPID_BEEPS,         // Feature 4: Police Road-Closed / Replanning Event
    TWO_LONG_BEEPS,      // Feature 2: Mission Completed at Hospital
    ALTERNATING_WARNING  // Feature 8: Critical System / Sensor Fault Fallback
};

inline std::string BuzzerPatternToString(BuzzerPattern pattern) {
    switch (pattern) {
        case BuzzerPattern::OFF:                 return "OFF";
        case BuzzerPattern::CONTINUOUS_TONE:     return "CONTINUOUS_TONE";
        case BuzzerPattern::THREE_SHORT_BEEPS:   return "THREE_SHORT_BEEPS";
        case BuzzerPattern::RAPID_BEEPS:         return "RAPID_BEEPS";
        case BuzzerPattern::TWO_LONG_BEEPS:      return "TWO_LONG_BEEPS";
        case BuzzerPattern::ALTERNATING_WARNING: return "ALTERNATING_WARNING";
        default:                                 return "UNKNOWN";
    }
}

class BuzzerController {
public:
    explicit BuzzerController(
        std::shared_ptr<SymbolicGpioHAL> hal,
        const std::string& symbolicSignal = DemoSignals::COORDINATOR_BUZZER);
    ~BuzzerController();

    // Prevent copying
    BuzzerController(const BuzzerController&) = delete;
    BuzzerController& operator=(const BuzzerController&) = delete;

    /**
     * @brief Initialize buzzer controller and verify HAL pin registration.
     */
    bool Initialize();

    /**
     * @brief Start playing a pre-defined rhythmic pattern asynchronously.
     * @param pattern Pattern to play.
     * @param repeatCount Number of times to loop the pattern (0 = loop indefinitely until Stop()).
     */
    void PlayPattern(BuzzerPattern pattern, uint32_t repeatCount = 1);

    /**
     * @brief Immediately silence the buzzer and terminate current playback.
     */
    void Stop();

    /**
     * @brief Check whether the buzzer is currently sounding or executing a pattern.
     */
    bool IsPlaying() const;

    /**
     * @brief Get currently active pattern.
     */
    BuzzerPattern GetCurrentPattern() const;

    /**
     * @brief Total number of physical buzzer toggles performed since initialization.
     */
    uint64_t GetToggleCount() const;

    /**
     * @brief Step simulation synchronously for deterministic testing without sleeping real time.
     * @param elapsedMs Elapsed virtual milliseconds.
     */
    void StepSimulation(uint32_t elapsedMs);

private:
    void WorkerLoop();
    void SetBuzzerState(bool on);

    std::shared_ptr<SymbolicGpioHAL> hal_;
    std::string symbolicSignal_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::thread workerThread_;

    std::atomic<bool> isRunning_{false};
    std::atomic<bool> isPlaying_{false};
    std::atomic<BuzzerPattern> currentPattern_{BuzzerPattern::OFF};
    uint32_t repeatCount_{1};
    uint32_t currentRepeat_{0};

    // Internal pattern sequencer state (for deterministic simulation and worker)
    uint32_t phaseIndex_{0};
    uint32_t phaseElapsedMs_{0};
    uint64_t toggleCount_{0};
    bool pinState_{false};
};

} // namespace aegis::hardware_integration::demo
