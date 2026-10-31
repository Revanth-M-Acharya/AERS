/**
 * @file SimulationGpioController.hpp
 * @brief Thread-safe in-memory cross-platform simulation backend for IGpioController.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "gpio/IGpioController.hpp"
#include <atomic>
#include <mutex>
#include <chrono>

namespace aegis::gpio {

class SimulationGpioController : public IGpioController {
public:
    explicit SimulationGpioController(int intersectionId = 1);
    ~SimulationGpioController() override;

    bool initialize() override;
    void shutdown() override;

    bool setBlue(bool on) override;
    bool setRed(bool on) override;

    bool getBlue() const override;
    bool getRed() const override;

    bool flashBlue(uint32_t durationMs = 250, uint32_t count = 3) override;
    virtual bool flashRed(uint32_t durationMs = 250, uint32_t count = 3) override;

    void reset() override;

    bool hasRedLed() const override { return hasRed_; }
    bool hasBlueLed() const override { return hasBlue_; }
    int getIntersectionId() const override { return intersectionId_; }
    std::string getBackendType() const override { return "SIMULATION_IN_MEMORY"; }

    // Test inspection helpers
    uint32_t getWriteCountBlue() const { return writeCountBlue_.load(); }
    uint32_t getWriteCountRed() const { return writeCountRed_.load(); }

private:
    int intersectionId_{1};
    bool hasRed_{true};
    bool hasBlue_{true};

    mutable std::mutex mutex_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> blueState_{false};
    std::atomic<bool> redState_{false};
    std::atomic<uint32_t> writeCountBlue_{0};
    std::atomic<uint32_t> writeCountRed_{0};
};

} // namespace aegis::gpio
