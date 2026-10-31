/**
 * @file PerformanceMonitor.hpp
 * @brief Real-time performance supervisory monitor measuring CPU, memory, latency, and throughput.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "digital_twin/TelemetryCollector.hpp"
#include <string>
#include <memory>
#include <mutex>

namespace aegis::digital_twin {

struct HardwarePerformanceMetrics {
    double cpuUsagePercent{0.0};
    uint64_t memoryUsageKb{0};
    uint32_t activeThreadCount{0};
    double eventThroughputPerSec{0.0};
    uint32_t deadlineMisses{0};
    double avgLatencyMs{0.5};
    double maxLatencyMs{1.2};
    uint64_t sampledMonoMs{0};
};

class PerformanceMonitor {
public:
    explicit PerformanceMonitor(std::shared_ptr<TelemetryCollector> telemetryCollector = nullptr);
    ~PerformanceMonitor() = default;

    void Sample();
    HardwarePerformanceMetrics GetMetrics() const;

    std::string RenderAsciiDashboard() const;
    std::string ToJson() const;

private:
    std::shared_ptr<TelemetryCollector> telemetryCollector_;
    mutable std::mutex mutex_;
    HardwarePerformanceMetrics cachedMetrics_;
    uint64_t lastSampleTimeMonoMs_{0};
    uint64_t lastUserTimeUs_{0};
};

} // namespace aegis::digital_twin

