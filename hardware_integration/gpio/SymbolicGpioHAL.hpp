/**
 * @file SymbolicGpioHAL.hpp
 * @brief GPIO Hardware Abstraction Layer with symbolic signal names for Raspberry Pi 4.
 * Eliminates all hardcoded GPIO numbers from driver and control logic.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "traffic_node/gpio/IGpioDriver.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <optional>

namespace aegis::hardware_integration {

/**
 * @brief Standard symbolic signal identifiers across SMART AEGIS roadside hardware.
 */
struct StandardSignals {
    static constexpr const char* LED_RED = "LED_RED";           // BCM 5, Pin 29 (Intersections 1 & 2)
    static constexpr const char* LED_BLUE = "LED_BLUE";         // BCM 6, Pin 31 (Intersections 1, 2 & 3)
    static constexpr const char* GPIO_RED = "GPIO_RED";         // BCM 5, Pin 29
    static constexpr const char* GPIO_BLUE = "GPIO_BLUE";       // BCM 6, Pin 31
    static constexpr const char* ROAD_A_RED = "ROAD_A_RED";     // Compatible alias for Red LED
    static constexpr const char* ROAD_A_YELLOW = "ROAD_A_YELLOW";
    static constexpr const char* ROAD_A_GREEN = "ROAD_A_GREEN"; // Compatible alias for Blue LED (Corridor active)
    static constexpr const char* ROAD_B_RED = "ROAD_B_RED";
    static constexpr const char* ROAD_B_YELLOW = "ROAD_B_YELLOW";
    static constexpr const char* ROAD_B_GREEN = "ROAD_B_GREEN";
    static constexpr const char* PEDESTRIAN_WALK = "PEDESTRIAN_WALK";
    static constexpr const char* EMERGENCY_STROBE = "EMERGENCY_STROBE";
    static constexpr const char* ACOUSTIC_BUZZER = "ACOUSTIC_BUZZER";
    static constexpr const char* APPROACH_IR_SENSOR = "APPROACH_IR_SENSOR";
    static constexpr const char* DEPARTURE_IR_SENSOR = "DEPARTURE_IR_SENSOR";
};

enum class PinDirection {
    INPUT,
    OUTPUT
};

enum class ActivePolarity {
    ACTIVE_HIGH, // Standard direct drive: 3.3V = Active/ON, 0V = Inactive/OFF
    ACTIVE_LOW   // Optocoupled relay drive: 0V = Active/ON, 3.3V = Inactive/OFF
};

struct SignalDefinition {
    std::string symbolicName;
    int physicalPin{-1}; // BCM GPIO number (e.g. 17)
    PinDirection direction{PinDirection::OUTPUT};
    ActivePolarity polarity{ActivePolarity::ACTIVE_HIGH};
    std::string description;
    bool currentLogicalState{false}; // True = Asserted/Active
    uint64_t lastToggleTimestampMs{0};
};

/**
 * @brief Thread-safe GPIO Hardware Abstraction Layer mapping symbolic signal names to BCM pins.
 */
class SymbolicGpioHAL {
public:
    explicit SymbolicGpioHAL(std::shared_ptr<traffic::IGpioDriver> driver = nullptr);
    ~SymbolicGpioHAL();

    // Prevent copying
    SymbolicGpioHAL(const SymbolicGpioHAL&) = delete;
    SymbolicGpioHAL& operator=(const SymbolicGpioHAL&) = delete;

    /**
     * @brief Set or replace underlying physical/mock GPIO driver.
     */
    void SetDriver(std::shared_ptr<traffic::IGpioDriver> driver);
    std::shared_ptr<traffic::IGpioDriver> GetDriver() const;

    /**
     * @brief Initialize underlying GPIO driver.
     */
    bool Initialize();

    /**
     * @brief Shutdown and de-assert all registered output signals safely.
     */
    void Shutdown();

    /**
     * @brief Register a symbolic signal mapping to a physical BCM pin.
     * @return true on success, false if physical pin conflicts or already registered.
     */
    bool RegisterSignal(const std::string& symbolicName,
                        int physicalPin,
                        PinDirection direction,
                        ActivePolarity polarity,
                        const std::string& description = "");

    /**
     * @brief Check whether a symbolic signal name is currently registered.
     */
    bool IsSignalRegistered(const std::string& symbolicName) const;

    /**
     * @brief Get physical BCM GPIO pin mapped to symbolic name.
     */
    int GetPhysicalPin(const std::string& symbolicName) const;

    /**
     * @brief Get full signal definition by symbolic name.
     */
    std::optional<SignalDefinition> GetSignalDefinition(const std::string& symbolicName) const;

    /**
     * @brief Assert or de-assert an output signal logically (handles active-high/active-low polarity).
     */
    bool SetSignal(const std::string& symbolicName, bool active);

    /**
     * @brief Read logical signal state (handles active-high/active-low polarity).
     */
    bool GetSignal(const std::string& symbolicName);

    /**
     * @brief Toggle logical state of an output signal.
     */
    bool ToggleSignal(const std::string& symbolicName);

    /**
     * @brief Read raw physical pin state (direct electrical level 0/1).
     */
    bool ReadRawPin(int physicalPin);

    /**
     * @brief Write raw physical pin state (direct electrical level 0/1).
     */
    bool WriteRawPin(int physicalPin, bool high);

    /**
     * @brief Retrieve snapshot of all registered signals and their current status.
     */
    std::vector<SignalDefinition> GetAllSignals() const;

    /**
     * @brief Clear all registered signals.
     */
    void ClearSignals();

private:
    std::shared_ptr<traffic::IGpioDriver> driver_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, SignalDefinition> signalsByName_;
    std::unordered_map<int, std::string> signalsByPin_;
    bool isInitialized_{false};
};

} // namespace aegis::hardware_integration
