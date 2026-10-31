/**
 * @file RoadHealthAnalyzer.hpp
 * @brief Analyzes road surface physical condition and safety hazard indices.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/perception/RoadConditionEvent.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace aegis::coordinator::perception::analytics {

struct RoadHealthMetrics {
    std::string segmentId;
    double healthScore{1.0};           // 1.0 = Pristine asphalt, 0.0 = Impassable/Hazardous
    double recommendedSpeedKmh{60.0};  // Advisory max speed
    bool hasPotholes{false};
    bool hasFlooding{false};
    bool hasDebrisOrFallenTree{false};
    bool isSafeForEmergencyTransit{true};
    uint64_t lastUpdatedMonoMs{0};
};

class RoadHealthAnalyzer {
public:
    RoadHealthAnalyzer() = default;
    ~RoadHealthAnalyzer() = default;

    RoadHealthMetrics RecordHazard(const std::string& segmentId,
                                   RoadEventType hazardType,
                                   EventSeverity severity,
                                   double defaultSpeedLimitKmh = 60.0);

    RoadHealthMetrics ClearHazards(const std::string& segmentId, double defaultSpeedLimitKmh = 60.0);

    RoadHealthMetrics GetHealth(const std::string& segmentId) const;
    bool IsSegmentSafe(const std::string& segmentId) const;
    double GetSpeedMultiplier(const std::string& segmentId) const;

    void Reset();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, RoadHealthMetrics> healthMap_;
};

} // namespace aegis::coordinator::perception::analytics

