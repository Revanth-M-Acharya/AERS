/**
 * @file QnxHardwareGpioController.hpp
 * @brief Production QNX Neutrino RTOS 8.0 Raspberry Pi 4 BCM2711 direct GPIO controller.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "gpio/IGpioController.hpp"
#include <atomic>
#include <mutex>
#include <cstdint>

namespace aegis::gpio {

class QnxHardwareGpioController : public IGpioController {
public:
    explicit QnxHardwareGpioController(int intersectionId = 1);
    ~QnxHardwareGpioController() override;

    bool initialize() override;
    void shutdown() override;

    bool setBlue(bool on) override;
    bool setRed(bool on) override;

    bool getBlue() const override;
    bool getRed() const override;

    bool flashBlue(uint32_t durationMs = 250, uint32_t count = 3) override;
    bool flashRed(uint32_t durationMs = 250, uint32_t count = 3) override;

    void reset() override;

    bool hasRedLed() const override { return hasRed_; }
    bool hasBlueLed() const override { return hasBlue_; }
    int getIntersectionId() const override { return intersectionId_; }
    std::string getBackendType() const override { return isHardwareMapped_ ? "QNX_HARDWARE_BCM2711" : "QNX_SIMULATION_FALLBACK"; }

    bool isHardwareMapped() const { return isHardwareMapped_.load(); }

private:
    void writePinRaw(int pin, bool high);
    bool readPinRaw(int pin) const;
    void configurePinOutput(int pin);

    int intersectionId_{1};
    bool hasRed_{true};
    bool hasBlue_{true};

    int pinRed_{5};   // BCM GPIO 5, Header Pin 29
    int pinBlue_{6};  // BCM GPIO 6, Header Pin 31

    mutable std::mutex mutex_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> isHardwareMapped_{false};
    void* gpioBaseAddr_{nullptr};

    // Software mirror of states for readback and non-hardware fallback
    std::atomic<bool> blueState_{false};
    std::atomic<bool> redState_{false};
};

} // namespace aegis::gpio
