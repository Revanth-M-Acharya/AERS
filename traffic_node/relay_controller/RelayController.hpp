/**
 * @file RelayController.hpp
 * @brief Hardware Abstraction Layer (HAL) for 4-Channel Traffic Signal Relay Controller.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "traffic_node/gpio/IGpioDriver.hpp"
#include <cstdint>
#include <memory>
#include <mutex>
#include <atomic>

namespace aegis::traffic {

/**
 * @brief Hardware Abstraction Layer controlling 4-channel traffic signal relay module.
 * Depends strictly on IGpioDriver. Contains zero networking or high-level FSM code.
 */
class RelayController {
public:
    RelayController();
    explicit RelayController(std::shared_ptr<IGpioDriver> gpioDriver);
    ~RelayController();

    // Prevent copying
    RelayController(const RelayController&) = delete;
    RelayController& operator=(const RelayController&) = delete;

    /**
     * @brief Initialize relay hardware pins and configure output modes.
     * @param redPin BCM GPIO pin for Channel 1 / RED lamp (e.g. GPIO 17).
     * @param yellowPin BCM GPIO pin for Channel 2 / YELLOW lamp (e.g. GPIO 27).
     * @param greenPin BCM GPIO pin for Channel 3 / GREEN lamp (e.g. GPIO 22).
     * @param auxPin BCM GPIO pin for Channel 4 / AUX lamp (e.g. GPIO 23).
     * @param activeLow True if relay module energizes on LOW level (optocoupler relay).
     * @return true on success, false on GPIO configuration failure.
     */
    bool Initialize(int redPin = 17, int yellowPin = 27, int greenPin = 22, int auxPin = 23, bool activeLow = true);

    /**
     * @brief Safe shutdown: de-energizes all relay channels.
     */
    void Shutdown();

    // Channel-Level Actuation
    void SetChannel(int channel, bool active);
    void SetAllChannels(bool ch1, bool ch2, bool ch3, bool ch4);
    bool IsChannelActive(int channel) const;

    // Backward-Compatible Signal Actuation Wrappers
    void SetRed();
    void SetYellow();
    void SetGreen();
    void SetAllRed();
    void FlashYellow();
    void TurnOffAll();

    // State Inspection Queries
    bool IsRedActive() const { return redActive_.load(); }
    bool IsYellowActive() const { return yellowActive_.load(); }
    bool IsGreenActive() const { return greenActive_.load(); }
    bool IsAuxActive() const { return auxActive_.load(); }
    bool IsInitialized() const { return initialized_.load(); }

    std::shared_ptr<IGpioDriver> GetGpioDriver() const { return driver_; }
    void SetGpioDriver(std::shared_ptr<IGpioDriver> driver);

private:
    void ApplyRelayStates(bool red, bool yellow, bool green, bool aux);

    std::shared_ptr<IGpioDriver> driver_;
    mutable std::recursive_mutex mutex_;

    int pinRed_{17};
    int pinYellow_{27};
    int pinGreen_{22};
    int pinAux_{23};
    bool activeLow_{true};

    std::atomic<bool> initialized_{false};
    std::atomic<bool> redActive_{false};
    std::atomic<bool> yellowActive_{false};
    std::atomic<bool> greenActive_{false};
    std::atomic<bool> auxActive_{false};
};

} // namespace aegis::traffic
