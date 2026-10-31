/**
 * @file PoliceOverrideSwitchDriver.hpp
 * @brief Debounced physical police override switch driver.
 * Generates verified roadside incident report without seizing manual traffic control.
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

struct PoliceOverrideSwitchConfig {
    std::string symbolicSignal{DemoSignals::POLICE_OVERRIDE_SWITCH};
    int physicalPin{24}; // BCM GPIO 24
    ActivePolarity polarity{ActivePolarity::ACTIVE_LOW}; // Internal pull-up to 3.3V
    uint32_t debounceMs{50};
    std::string defaultTargetIntersection{"INT_2"};
    std::string defaultReason{"POLICE_BARRICADE_ROAD_CLOSED"};
};

class PoliceOverrideSwitchDriver {
public:
    using PoliceReportCallback = std::function<void(
        const std::string& intersectionId,
        const std::string& reason,
        uint64_t timestampMonoMs)>;

    explicit PoliceOverrideSwitchDriver(
        std::shared_ptr<SymbolicGpioHAL> hal,
        PoliceOverrideSwitchConfig config = {});
    ~PoliceOverrideSwitchDriver() = default;

    // Prevent copying
    PoliceOverrideSwitchDriver(const PoliceOverrideSwitchDriver&) = delete;
    PoliceOverrideSwitchDriver& operator=(const PoliceOverrideSwitchDriver&) = delete;

    /**
     * @brief Initialize switch GPIO signal in HAL.
     */
    bool Initialize();

    /**
     * @brief Register callback invoked when police switch is activated.
     */
    void RegisterCallback(PoliceReportCallback callback);

    /**
     * @brief Poll physical GPIO state with software debouncing.
     * @param nowMonoMs Monotonic timestamp in milliseconds.
     * @return true if a valid debounced press was detected on this poll.
     */
    bool Poll(uint64_t nowMonoMs);

    /**
     * @brief Simulate police report switch press programmatically.
     * @param nowMonoMs Monotonic timestamp in milliseconds.
     * @param customReason Optional override reason description.
     */
    void SimulatePress(uint64_t nowMonoMs, const std::string& customReason = "");

    /**
     * @brief Check whether the switch is currently held down.
     */
    bool IsPressed() const;

    /**
     * @brief Total number of police reports triggered since initialization.
     */
    uint64_t GetTotalReports() const;

    /**
     * @brief Timestamp of the most recent police report in monotonic milliseconds.
     */
    uint64_t GetLastReportTimestampMs() const;

    const std::string& GetTargetIntersection() const { return config_.defaultTargetIntersection; }
    const std::string& GetDefaultReason() const { return config_.defaultReason; }

private:
    std::shared_ptr<SymbolicGpioHAL> hal_;
    PoliceOverrideSwitchConfig config_;
    PoliceReportCallback callback_;

    mutable std::mutex mutex_;
    bool isPressed_{false};
    bool rawState_{false};
    uint64_t lastStateChangeMs_{0};
    uint64_t totalReports_{0};
    uint64_t lastReportMs_{0};
};

} // namespace aegis::hardware_integration::demo
