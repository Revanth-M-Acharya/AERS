/**
 * @file BenchmarkRecorder.hpp
 * @brief High-precision benchmarking recorder measuring latency percentiles (Min, Max, Mean, P95, P99) and SLA conformance.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace aegis::digital_twin {

struct BenchmarkMetric {
    std::string operationName;
    uint64_t sampleCount{0};
    double minLatencyUs{0.0};
    double maxLatencyUs{0.0};
    double meanLatencyUs{0.0};
    double p95LatencyUs{0.0};
    double p99LatencyUs{0.0};
    double targetSlaUs{5000.0}; // 5 ms target SLA for QNX real-time operations
    bool passesSla{true};
};

class BenchmarkRecorder {
public:
    BenchmarkRecorder() = default;
    ~BenchmarkRecorder() = default;

    void RecordLatency(const std::string& operationName, double latencyUs, double targetSlaUs = 5000.0);
    BenchmarkMetric GetMetric(const std::string& operationName) const;
    std::vector<BenchmarkMetric> GetAllMetrics() const;

    void Reset();

    std::string ToJson() const;
    std::string ToAsciiTable() const;

private:
    struct OperationHistory {
        std::string name;
        double targetSlaUs{5000.0};
        std::vector<double> samplesUs;
    };

    mutable std::mutex mutex_;
    std::unordered_map<std::string, OperationHistory> history_;
};

} // namespace aegis::digital_twin

