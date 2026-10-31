/**
 * @file IRSensorDriver.cpp
 * @brief Implementation of IRSensorDriver.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "hardware_integration/sensor/IRSensorDriver.hpp"
#include "common/utils/TimeUtils.hpp"
#include <algorithm>

namespace aegis::hardware_integration {

IRSensorDriver::IRSensorDriver(std::shared_ptr<SymbolicGpioHAL> hal, IRSensorConfig config)
    : hal_(std::move(hal)), config_(std::move(config)) {
}

IRSensorDriver::~IRSensorDriver() = default;

bool IRSensorDriver::Initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!hal_) return false;

    if (!hal_->IsSignalRegistered(config_.symbolicSignal)) {
        hal_->RegisterSignal(config_.symbolicSignal,
                             config_.physicalPin,
                             PinDirection::INPUT,
                             config_.polarity,
                             "Optical IR Vehicle Proximity Sensor (" + config_.sensorId + ")");
    }

    lastRawReading_ = hal_->GetSignal(config_.symbolicSignal);
    debouncedState_ = lastRawReading_ ? IRSensorState::BEAM_INTERRUPTED : IRSensorState::BEAM_CLEAR;
    lastRawChangeMonoMs_ = common::TimeUtils::GetMonotonicTimeMs();
    return true;
}

IRSensorState IRSensorDriver::Poll(uint64_t currentTimeMonoMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!hal_) return debouncedState_;

    uint64_t now = (currentTimeMonoMs > 0) ? currentTimeMonoMs : common::TimeUtils::GetMonotonicTimeMs();
    bool rawReading = hal_->GetSignal(config_.symbolicSignal);

    // Edge transition detected on raw input
    if (rawReading != lastRawReading_) {
        lastRawReading_ = rawReading;
        lastRawChangeMonoMs_ = now;
    }

    // Debounce & Glitch Filter: Require stable reading for config_.debounceWindowMs
    uint64_t stableDuration = (now >= lastRawChangeMonoMs_) ? (now - lastRawChangeMonoMs_) : 0;
    if (stableDuration >= config_.debounceWindowMs) {
        IRSensorState candidateState = rawReading ? IRSensorState::BEAM_INTERRUPTED : IRSensorState::BEAM_CLEAR;

        if (candidateState != debouncedState_) {
            // Valid debounced state transition
            debouncedState_ = candidateState;

            if (debouncedState_ == IRSensorState::BEAM_INTERRUPTED) {
                // Vehicle entered sensor beam
                totalDetections_++;
                occupancyStartTimeMs_ = now;
                lastTriggerTimestampMs_ = now;
                recentTriggerTimestamps_.push_back(now);

                if (onVehicleDetected_) {
                    onVehicleDetected_(config_.sensorId, now);
                }
            } else {
                // Vehicle cleared sensor beam
                lastClearTimestampMs_ = now;
                uint64_t occupancy = (occupancyStartTimeMs_ > 0 && now >= occupancyStartTimeMs_)
                                         ? (now - occupancyStartTimeMs_) : 0;
                totalOccupancyDurationMs_ += occupancy;
                occupancyStartTimeMs_ = 0;

                if (onVehicleCleared_) {
                    onVehicleCleared_(config_.sensorId, occupancy);
                }
            }
        }
    }

    // Run diagnostics
    RunDiagnostics(now);

    return debouncedState_;
}

void IRSensorDriver::SimulateRawPinLevel(bool activeLevel, uint64_t currentTimeMonoMs) {
    if (hal_) {
        hal_->SetSignal(config_.symbolicSignal, activeLevel);
    }
    Poll(currentTimeMonoMs);
}

SensorHealthGrade IRSensorDriver::RunDiagnostics(uint64_t currentTimeMonoMs) {
    uint64_t now = (currentTimeMonoMs > 0) ? currentTimeMonoMs : common::TimeUtils::GetMonotonicTimeMs();

    // 1. Stuck-Active Check: Beam blocked longer than threshold
    if (debouncedState_ == IRSensorState::BEAM_INTERRUPTED && occupancyStartTimeMs_ > 0) {
        uint64_t currentOccSec = (now - occupancyStartTimeMs_) / 1000;
        if (currentOccSec > config_.maxContinuousTriggerSec) {
            healthGrade_ = SensorHealthGrade::FAULTY;
            activeFault_ = SensorFaultType::STUCK_ACTIVE;
            return healthGrade_;
        }
    }

    // 2. High-Frequency Oscillation / Chatter Check: Count triggers in sliding 60s window
    while (!recentTriggerTimestamps_.empty() && (now - recentTriggerTimestamps_.front() > 60000)) {
        recentTriggerTimestamps_.erase(recentTriggerTimestamps_.begin());
    }

    if (recentTriggerTimestamps_.size() > config_.maxTriggersPerMinute) {
        healthGrade_ = SensorHealthGrade::DEGRADED;
        activeFault_ = SensorFaultType::OSCILLATING;
        return healthGrade_;
    }

    // Nominal
    healthGrade_ = SensorHealthGrade::HEALTHY;
    activeFault_ = SensorFaultType::NONE;
    return healthGrade_;
}

void IRSensorDriver::SetOnVehicleDetected(VehicleDetectedCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    onVehicleDetected_ = std::move(cb);
}

void IRSensorDriver::SetOnVehicleCleared(VehicleClearedCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    onVehicleCleared_ = std::move(cb);
}

IRSensorState IRSensorDriver::GetCurrentState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return debouncedState_;
}

bool IRSensorDriver::IsVehiclePresent() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return (debouncedState_ == IRSensorState::BEAM_INTERRUPTED);
}

IRSensorMetrics IRSensorDriver::GetMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    IRSensorMetrics m;
    m.sensorId = config_.sensorId;
    m.symbolicSignal = config_.symbolicSignal;
    m.physicalPin = config_.physicalPin;
    m.currentState = debouncedState_;
    m.healthGrade = healthGrade_;
    m.activeFault = activeFault_;
    m.totalVehicleDetections = totalDetections_;

    uint64_t now = common::TimeUtils::GetMonotonicTimeMs();
    if (debouncedState_ == IRSensorState::BEAM_INTERRUPTED && occupancyStartTimeMs_ > 0) {
        m.currentOccupancyDurationMs = (now >= occupancyStartTimeMs_) ? (now - occupancyStartTimeMs_) : 0;
    } else {
        m.currentOccupancyDurationMs = 0;
    }

    m.totalOccupancyDurationMs = totalOccupancyDurationMs_ + m.currentOccupancyDurationMs;
    m.lastTriggerTimestampMs = lastTriggerTimestampMs_;
    m.lastClearTimestampMs = lastClearTimestampMs_;
    m.triggerFrequencyHz = static_cast<double>(recentTriggerTimestamps_.size()) / 60.0;
    return m;
}

void IRSensorDriver::ResetCounters() {
    std::lock_guard<std::mutex> lock(mutex_);
    totalDetections_ = 0;
    totalOccupancyDurationMs_ = 0;
    occupancyStartTimeMs_ = 0;
    recentTriggerTimestamps_.clear();
    healthGrade_ = SensorHealthGrade::HEALTHY;
    activeFault_ = SensorFaultType::NONE;
}

} // namespace aegis::hardware_integration
