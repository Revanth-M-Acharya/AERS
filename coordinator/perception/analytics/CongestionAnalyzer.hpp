/**
 * @file CongestionAnalyzer.hpp
 * @brief Classifies congestion patterns and delay propagation across road segments.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace aegis::coordinator::perception::analytics {

enum class CongestionTrend {
    FREE_FLOW,
    BUILDING,
    GRIDLOCK,
    DISSIPATING
};

std::string CongestionTrendToString(CongestionTrend trend);

struct CongestionMetrics {
    std::string segmentId;
    double currentAverageSpeedKmh{50.0};
    double freeFlowSpeedKmh{50.0};
    double speedDeficitRatio{0.0};     // (freeFlow - current) / freeFlow
    double estimatedTransitDelaySec{0.0};
    CongestionTrend trend{CongestionTrend::FREE_FLOW};
    uint64_t lastUpdatedMonoMs{0};
};

class CongestionAnalyzer {
public:
    CongestionAnalyzer() = default;
    ~CongestionAnalyzer() = default;

    CongestionMetrics UpdateSpeed(const std::string& segmentId,
                                  double currentSpeedKmh,
                                  double freeFlowSpeedKmh = 50.0,
                                  double segmentLengthMeters = 500.0);

    CongestionMetrics GetMetrics(const std::string& segmentId) const;
    double GetEstimatedDelay(const std::string& segmentId) const;
    CongestionTrend GetTrend(const std::string& segmentId) const;

    void Reset();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, CongestionMetrics> metricsMap_;
};

} // namespace aegis::coordinator::perception::analytics

