/**
 * @file TrafficDensityAnalyzer.hpp
 * @brief Analyzes traffic density, vehicular flow rate, and Level of Service (LOS).
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <mutex>

namespace aegis::coordinator::perception::analytics {

enum class LevelOfService {
    LOS_A, // Free flow
    LOS_B, // Reasonably free flow
    LOS_C, // Stable flow
    LOS_D, // Approaching unstable flow
    LOS_E, // Unstable flow / Capacity
    LOS_F  // Forced or breakdown flow (Gridlock)
};

std::string LevelOfServiceToString(LevelOfService los);

struct SegmentDensityMetrics {
    std::string segmentId;
    uint32_t currentVehicleCount{0};
    uint32_t laneCount{2};
    double segmentLengthMeters{500.0};
    double vehiclesPerKmPerLane{0.0};
    double normalizedDensityScore{0.0}; // 0.0 (empty) to 1.0 (jam)
    LevelOfService los{LevelOfService::LOS_A};
    uint64_t lastUpdatedMonoMs{0};
};

class TrafficDensityAnalyzer {
public:
    TrafficDensityAnalyzer() = default;
    ~TrafficDensityAnalyzer() = default;

    SegmentDensityMetrics UpdateDensity(const std::string& segmentId,
                                        uint32_t detectedVehicles,
                                        uint32_t laneCount = 2,
                                        double segmentLengthMeters = 500.0);

    SegmentDensityMetrics GetMetrics(const std::string& segmentId) const;
    LevelOfService GetLevelOfService(const std::string& segmentId) const;
    double GetNormalizedDensity(const std::string& segmentId) const;

    void Reset();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, SegmentDensityMetrics> segmentMetrics_;
};

} // namespace aegis::coordinator::perception::analytics

