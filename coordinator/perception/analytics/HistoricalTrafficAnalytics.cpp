/**
 * @file HistoricalTrafficAnalytics.cpp
 * @brief Implementation of HistoricalTrafficAnalytics rolling statistics and bottleneck ranking.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/perception/analytics/HistoricalTrafficAnalytics.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace aegis::coordinator::perception::analytics {

void HistoricalTrafficAnalytics::RecordSpeedSample(const std::string& segmentId, double speedKmh, uint64_t /*timestampMonoMs*/) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& stats = statsMap_[segmentId];
    stats.segmentId = segmentId;

    // Keep rolling window of last 500 samples
    if (stats.speedSamples.size() >= 500) {
        stats.speedSamples.erase(stats.speedSamples.begin());
    }
    stats.speedSamples.push_back(speedKmh);
}

void HistoricalTrafficAnalytics::RecordIncident(const std::string& segmentId, RoadEventType type, double durationSec, uint64_t timestampMonoMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& stats = statsMap_[segmentId];
    stats.segmentId = segmentId;

    HistoricalIncidentRecord rec;
    rec.type = type;
    rec.durationSec = durationSec;
    rec.timestampMonoMs = timestampMonoMs;
    stats.incidents.push_back(rec);

    if (durationSec > 0.0) {
        stats.totalClearanceTimeSec += durationSec;
        stats.resolvedIncidentsCount++;
    }
}

void HistoricalTrafficAnalytics::RecordAvailabilitySample(const std::string& segmentId, bool isPassable) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& stats = statsMap_[segmentId];
    stats.segmentId = segmentId;
    stats.totalSamplesCount++;
    if (isPassable) {
        stats.availableSamplesCount++;
    }
}

double HistoricalTrafficAnalytics::GetAverageSpeed(const std::string& segmentId, double defaultSpeed) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = statsMap_.find(segmentId);
    if (it == statsMap_.end() || it->second.speedSamples.empty()) {
        return defaultSpeed;
    }
    const auto& samples = it->second.speedSamples;
    double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
    return sum / static_cast<double>(samples.size());
}

double HistoricalTrafficAnalytics::GetAverageCongestion(const std::string& segmentId, double freeFlowSpeed) const {
    double avgSpeed = GetAverageSpeed(segmentId, freeFlowSpeed);
    if (freeFlowSpeed <= 0.0 || avgSpeed >= freeFlowSpeed) {
        return 0.0;
    }
    double congestion = (freeFlowSpeed - avgSpeed) / freeFlowSpeed;
    return std::clamp(congestion, 0.0, 1.0);
}

double HistoricalTrafficAnalytics::GetIncidentFrequency(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = statsMap_.find(segmentId);
    if (it == statsMap_.end()) {
        return 0.0;
    }
    return static_cast<double>(it->second.incidents.size());
}

double HistoricalTrafficAnalytics::GetRoadAvailability(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = statsMap_.find(segmentId);
    if (it == statsMap_.end() || it->second.totalSamplesCount == 0) {
        return 100.0; // Default 100% available
    }
    return (static_cast<double>(it->second.availableSamplesCount) * 100.0) /
           static_cast<double>(it->second.totalSamplesCount);
}

double HistoricalTrafficAnalytics::GetAverageClearanceTimeSec(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = statsMap_.find(segmentId);
    if (it == statsMap_.end() || it->second.resolvedIncidentsCount == 0) {
        return 0.0;
    }
    return it->second.totalClearanceTimeSec / static_cast<double>(it->second.resolvedIncidentsCount);
}

std::string HistoricalTrafficAnalytics::GetPeakTrafficPeriods(const std::string& segmentId) const {
    // Determine profile from recorded congestion ratio
    double cong = GetAverageCongestion(segmentId, 50.0);
    if (cong > 0.50) {
        return "Morning (08:00 - 10:30), Evening (17:00 - 19:30)";
    } else if (cong > 0.20) {
        return "Evening Rush (17:30 - 19:00)";
    } else {
        return "Nominal / Off-Peak";
    }
}

std::vector<BottleneckRank> HistoricalTrafficAnalytics::GetHistoricalBottlenecks(size_t topN) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<BottleneckRank> ranks;

    for (const auto& [segId, stats] : statsMap_) {
        BottleneckRank rank;
        rank.segmentId = segId;

        // 1. Congestion ratio
        double avgSpeed = 50.0;
        if (!stats.speedSamples.empty()) {
            double sum = std::accumulate(stats.speedSamples.begin(), stats.speedSamples.end(), 0.0);
            avgSpeed = sum / static_cast<double>(stats.speedSamples.size());
        }
        rank.averageCongestionRatio = (avgSpeed < 50.0) ? ((50.0 - avgSpeed) / 50.0) : 0.0;

        // 2. Incident count
        rank.incidentFrequencyPerHour = static_cast<double>(stats.incidents.size());

        // 3. Availability %
        if (stats.totalSamplesCount > 0) {
            rank.roadAvailabilityPercent = (static_cast<double>(stats.availableSamplesCount) * 100.0) /
                                           static_cast<double>(stats.totalSamplesCount);
        } else {
            rank.roadAvailabilityPercent = 100.0;
        }

        // 4. Clearance time
        if (stats.resolvedIncidentsCount > 0) {
            rank.averageClearanceTimeSec = stats.totalClearanceTimeSec / static_cast<double>(stats.resolvedIncidentsCount);
        } else {
            rank.averageClearanceTimeSec = 0.0;
        }

        // Composite bottleneck score calculation (0 - 100)
        // High congestion (up to 40 pts), frequent incidents (up to 30 pts),
        // low availability (up to 20 pts), long clearance (up to 10 pts)
        double score = (rank.averageCongestionRatio * 40.0) +
                       (std::min(rank.incidentFrequencyPerHour, 10.0) * 3.0) +
                       ((100.0 - rank.roadAvailabilityPercent) * 0.20) +
                       (std::min(rank.averageClearanceTimeSec, 300.0) / 300.0 * 10.0);

        rank.bottleneckScore = std::clamp(score, 0.0, 100.0);
        ranks.push_back(rank);
    }

    // Sort descending by bottleneckScore
    std::sort(ranks.begin(), ranks.end(), [](const BottleneckRank& a, const BottleneckRank& b) {
        return a.bottleneckScore > b.bottleneckScore;
    });

    if (ranks.size() > topN) {
        ranks.resize(topN);
    }

    return ranks;
}

void HistoricalTrafficAnalytics::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    statsMap_.clear();
}

} // namespace aegis::coordinator::perception::analytics

