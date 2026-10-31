/**
 * @file EmergencySwitchDriver.hpp
 * @brief Debounced physical emergency push-button driver located near HOME.
 * Triggers emergency mission intake, hospital assignment, and green corridor dispatch.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "hardware_integration/gpio/SymbolicGpioHAL.hpp"
#include "hardware_integration/demo/DemoHardwareSignals.hpp"

#include <string>
#include <memory>
#include <mutex>
#include <functional>
#include <cstdint>

namespace aegis::hardware_integration::demo {

struct EmergencySwitchConfig {
    std::string symbolicSignal{DemoSignals::EMERGENCY_SWITCH};
    int physicalPin{23}; // BCM GPIO 23
    ActivePolarity polarity{ActivePolarity::ACTIVE_LOW}; // Internal pull-up to 3.3V, switch connects to GND
    uint32_t debounceMs{50}; // Contact bounce filter
};

class EmergencySwitchDriver {
public:
    using TriggerCallback = std::function<void(uint64_t timestampMonoMs)>;

    explicit EmergencySwitchDriver(
        std::shared_ptr<SymbolicGpioHAL> hal,
        EmergencySwitchConfig config = {});
    ~EmergencySwitchDriver() = default;

    // Prevent copying
    EmergencySwitchDriver(const EmergencySwitchDriver&) = delete;
    EmergencySwitchDriver& operator=(const EmergencySwitchDriver&) = delete;

    /**
     * @brief Initialize switch GPIO signal in HAL.
     */
    bool Initialize();

    /**
     * @brief Register callback invoked when switch transitions from released to pressed.
     */
    void RegisterCallback(TriggerCallback callback);

    /**
     * @brief Poll physical GPIO state with software debouncing.
     * @param nowMonoMs Monotonic timestamp in milliseconds.
     * @return true if a valid debounced press was detected on this poll.
     */
    bool Poll(uint64_t nowMonoMs);

    /**
     * @brief Simulate switch press programmatically (for automated testing / demonstration).
     * @param nowMonoMs Monotonic timestamp in milliseconds.
     */
    void SimulatePress(uint64_t nowMonoMs);

    /**
     * @brief Check whether the switch is currently held down.
     */
    bool IsPressed() const;

    /**
     * @brief Total number of valid presses detected since initialization.
     */
    uint64_t GetTotalTriggers() const;

    /**
     * @brief Timestamp of the most recent press in monotonic milliseconds.
     */
    uint64_t GetLastTriggerTimestampMs() const;

private:
    std::shared_ptr<SymbolicGpioHAL> hal_;
    EmergencySwitchConfig config_;
    TriggerCallback callback_;

    mutable std::mutex mutex_;
    bool isPressed_{false};
    bool rawState_{false};
    uint64_t lastStateChangeMs_{0};
    uint64_t totalTriggers_{0};
    uint64_t lastTriggerMs_{0};
};

} // namespace aegis::hardware_integration::demo
