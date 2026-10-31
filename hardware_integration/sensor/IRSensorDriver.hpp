/**
 * @file IRSensorDriver.hpp
 * @brief Optical IR Proximity Sensor driver with debouncing, health monitoring, and diagnostics.
 * Detects approaching emergency vehicles and civilian traffic on intersection approaches.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "hardware_integration/gpio/SymbolicGpioHAL.hpp"
#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>

namespace aegis::hardware_integration {

enum class IRSensorState {
    BEAM_CLEAR,     // No vehicle present
    BEAM_INTERRUPTED // Vehicle detected in approach zone
};

enum class SensorHealthGrade {
    HEALTHY,    // Normal operation
    DEGRADED,   // Intermittent glitching or minor noise detected
    FAULTY      // Stuck active or oscillating uncontrollably
};

enum class SensorFaultType {
    NONE,
    STUCK_ACTIVE,   // Beam interrupted continuously > timeout (dirt, physical obstruction)
    OSCILLATING,    // Rapid transitions exceeding physical vehicle passing limits
    ELECTRICAL_DISCONNECT // Constant open-circuit reading
};

struct IRSensorMetrics {
    std::string sensorId;
    std::string symbolicSignal;
    int physicalPin{-1};
    IRSensorState currentState{IRSensorState::BEAM_CLEAR};
    SensorHealthGrade healthGrade{SensorHealthGrade::HEALTHY};
    SensorFaultType activeFault{SensorFaultType::NONE};

    uint64_t totalVehicleDetections{0};
    uint64_t currentOccupancyDurationMs{0};
    uint64_t totalOccupancyDurationMs{0};
    uint64_t lastTriggerTimestampMs{0};
    uint64_t lastClearTimestampMs{0};
    double triggerFrequencyHz{0.0};
};

struct IRSensorConfig {
    std::string sensorId{"IR_APPROACH_1"};
    std::string symbolicSignal{StandardSignals::APPROACH_IR_SENSOR};
    int physicalPin{12};
    ActivePolarity polarity{ActivePolarity::ACTIVE_LOW}; // Most active IR modules pull LOW on detection
    uint32_t debounceWindowMs{30};       // Filter contact/optical bounce
    uint32_t minGlitchDurationMs{5};      // Reject sub-5ms electrical spikes
    uint32_t maxContinuousTriggerSec{120}; // Max 2 minutes continuous before flagging STUCK
    uint32_t maxTriggersPerMinute{45};    // Max plausible vehicle count per minute per lane
};

class IRSensorDriver {
public:
    IRSensorDriver(std::shared_ptr<SymbolicGpioHAL> hal, IRSensorConfig config = {});
    ~IRSensorDriver();

    // Prevent copying
    IRSensorDriver(const IRSensorDriver&) = delete;
    IRSensorDriver& operator=(const IRSensorDriver&) = delete;

    /**
     * @brief Initialize hardware pin and register signal.
     */
    bool Initialize();

    /**
     * @brief Poll and process digital pin reading with debouncing and glitch filtering.
     * @param currentTimeMonoMs Current monotonic time in milliseconds (0 = fetch via TimeUtils).
     * @return Current debounced IRSensorState.
     */
    IRSensorState Poll(uint64_t currentTimeMonoMs = 0);

    /**
     * @brief Force state update in test / simulation harnesses.
     */
    void SimulateRawPinLevel(bool activeLevel, uint64_t currentTimeMonoMs = 0);

    /**
     * @brief Execute health diagnostics audit.
     */
    SensorHealthGrade RunDiagnostics(uint64_t currentTimeMonoMs = 0);

    // Callbacks
    using VehicleDetectedCallback = std::function<void(const std::string& sensorId, uint64_t timestampMs)>;
    using VehicleClearedCallback = std::function<void(const std::string& sensorId, uint64_t durationMs)>;
    void SetOnVehicleDetected(VehicleDetectedCallback cb);
    void SetOnVehicleCleared(VehicleClearedCallback cb);

    // Queries
    IRSensorState GetCurrentState() const;
    bool IsVehiclePresent() const;
    IRSensorMetrics GetMetrics() const;
    void ResetCounters();

private:
    std::shared_ptr<SymbolicGpioHAL> hal_;
    IRSensorConfig config_;
    mutable std::mutex mutex_;

    IRSensorState debouncedState_{IRSensorState::BEAM_CLEAR};
    bool lastRawReading_{false};
    uint64_t lastRawChangeMonoMs_{0};

    // Tracking & Health
    uint64_t totalDetections_{0};
    uint64_t occupancyStartTimeMs_{0};
    uint64_t totalOccupancyDurationMs_{0};
    uint64_t lastTriggerTimestampMs_{0};
    uint64_t lastClearTimestampMs_{0};

    std::vector<uint64_t> recentTriggerTimestamps_; // Sliding 60-second window
    SensorHealthGrade healthGrade_{SensorHealthGrade::HEALTHY};
    SensorFaultType activeFault_{SensorFaultType::NONE};

    VehicleDetectedCallback onVehicleDetected_;
    VehicleClearedCallback onVehicleCleared_;
};

} // namespace aegis::hardware_integration
