/**
 * @file HistoricalTrafficAnalytics.hpp
 * @brief Rolling historical traffic statistics, bottleneck identification, and availability metrics.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/perception/RoadConditionEvent.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace aegis::coordinator::perception::analytics {

struct HistoricalIncidentRecord {
    RoadEventType type;
    double durationSec{0.0};
    uint64_t timestampMonoMs{0};
};

struct SegmentHistoricalStats {
    std::string segmentId;
    std::vector<double> speedSamples;
    std::vector<HistoricalIncidentRecord> incidents;
    uint64_t totalSamplesCount{0};
    uint64_t availableSamplesCount{0};
    double totalClearanceTimeSec{0.0};
    uint32_t resolvedIncidentsCount{0};
};

struct BottleneckRank {
    std::string segmentId;
    double averageCongestionRatio{0.0}; // 0.0 (free) to 1.0 (severe congestion)
    double incidentFrequencyPerHour{0.0};
    double roadAvailabilityPercent{100.0};
    double averageClearanceTimeSec{0.0};
    double bottleneckScore{0.0}; // Higher means worse bottleneck
};

class HistoricalTrafficAnalytics {
public:
    HistoricalTrafficAnalytics() = default;
    ~HistoricalTrafficAnalytics() = default;

    void RecordSpeedSample(const std::string& segmentId, double speedKmh, uint64_t timestampMonoMs = 0);
    void RecordIncident(const std::string& segmentId, RoadEventType type, double durationSec, uint64_t timestampMonoMs = 0);
    void RecordAvailabilitySample(const std::string& segmentId, bool isPassable);

    double GetAverageSpeed(const std::string& segmentId, double defaultSpeed = 50.0) const;
    double GetAverageCongestion(const std::string& segmentId, double freeFlowSpeed = 50.0) const;
    double GetIncidentFrequency(const std::string& segmentId) const;
    double GetRoadAvailability(const std::string& segmentId) const; // Percentage 0.0 - 100.0%
    double GetAverageClearanceTimeSec(const std::string& segmentId) const;
    std::string GetPeakTrafficPeriods(const std::string& segmentId) const;

    std::vector<BottleneckRank> GetHistoricalBottlenecks(size_t topN = 5) const;

    void Reset();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, SegmentHistoricalStats> statsMap_;
};

} // namespace aegis::coordinator::perception::analytics

