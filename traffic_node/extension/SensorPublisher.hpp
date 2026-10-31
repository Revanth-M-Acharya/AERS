/**
 * @file SensorPublisher.hpp
 * @brief Extension interface for future AI sensor telemetry publishing.
 * Pure virtual interface — contains ZERO implementation.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <cstdint>

namespace aegis::traffic::extension {

class ISensorPublisher {
public:
    virtual ~ISensorPublisher() = default;

    /**
     * @brief Publish telemetry sampled from physical IR/optical sensors.
     */
    virtual void PublishSensorData(const std::string& sensorId, double value, uint64_t timestamp) = 0;
};

} // namespace aegis::traffic::extension

