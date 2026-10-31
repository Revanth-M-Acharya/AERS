/**
 * @file Rpi4GpioDriver.hpp
 * @brief Hardware GPIO driver for Raspberry Pi 4 Model B (BCM2711) on QNX SDP 8.0.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "traffic_node/gpio/IGpioDriver.hpp"
#include <mutex>
#include <atomic>
#include <unordered_map>

namespace aegis::traffic {

class Rpi4GpioDriver : public IGpioDriver {
public:
    Rpi4GpioDriver();
    ~Rpi4GpioDriver() override;

    bool Initialize() override;
    void Shutdown() override;
    bool ConfigureOutput(int pin) override;
    bool ConfigureInput(int pin) override;
    bool WritePin(int pin, bool high) override;
    bool ReadPin(int pin) override;
    std::string GetDriverName() const override { return "Rpi4GpioDriver (BCM2711)"; }

    bool IsHardwareMapped() const { return hardwareMapped_.load(); }

private:
    std::atomic<bool> initialized_{false};
    std::atomic<bool> hardwareMapped_{false};
    mutable std::mutex mutex_;

    // Fallback in-memory state tracking when hardware mapped I/O is unavailable
    std::unordered_map<int, bool> fallbackPinState_;
    void* gpioBaseAddr_{nullptr};
};

} // namespace aegis::traffic

