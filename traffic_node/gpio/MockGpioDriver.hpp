/**
 * @file MockGpioDriver.hpp
 * @brief In-memory mock GPIO driver for unit testing and deterministic simulation.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "traffic_node/gpio/IGpioDriver.hpp"
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace aegis::traffic {

class MockGpioDriver : public IGpioDriver {
public:
    MockGpioDriver();
    ~MockGpioDriver() override = default;

    bool Initialize() override;
    void Shutdown() override;
    bool ConfigureOutput(int pin) override;
    bool ConfigureInput(int pin) override;
    virtual bool WritePin(int pin, bool high) override;
    virtual bool ReadPin(int pin) override;
    std::string GetDriverName() const override { return "MockGpioDriver"; }

    // Test Inspection Helpers
    bool GetPinLevel(int pin) const;
    uint32_t GetWriteCount(int pin) const;
    bool IsConfiguredOutput(int pin) const;
    bool IsInitialized() const { return initialized_.load(); }
    void Reset();

private:
    mutable std::mutex mutex_;
    std::atomic<bool> initialized_{false};
    std::unordered_map<int, bool> pinLevels_;
    std::unordered_map<int, bool> isOutput_;
    std::unordered_map<int, uint32_t> writeCounts_;
};

} // namespace aegis::traffic

