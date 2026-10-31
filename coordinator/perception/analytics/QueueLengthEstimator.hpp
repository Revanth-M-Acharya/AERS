/**
 * @file QueueLengthEstimator.hpp
 * @brief Estimates intersection queue lengths and required green wave pre-clearance time.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace aegis::coordinator::perception::analytics {

struct QueueMetrics {
    std::string approachId;          // e.g. "INT_A_NORTH"
    uint32_t queuedVehicleCount{0};
    double queueLengthMeters{0.0};   // Assuming ~6.5m average per stopped vehicle
    double estimatedClearanceSec{0.0}; // Time needed to discharge queue before ambulance arrives
    bool isSpillbackRisk{false};     // True if queue length > 80% of segment length
    uint64_t lastUpdatedMonoMs{0};
};

class QueueLengthEstimator {
public:
    QueueLengthEstimator() = default;
    ~QueueLengthEstimator() = default;

    QueueMetrics UpdateQueue(const std::string& approachId,
                             uint32_t stoppedVehicleCount,
                             double segmentLengthMeters = 300.0,
                             double averageVehicleSpacingMeters = 6.5,
                             double dischargeRateVehPerSec = 0.5); // ~2s saturation headway

    QueueMetrics GetQueue(const std::string& approachId) const;
    double GetRequiredClearanceTime(const std::string& approachId) const;
    bool HasSpillbackRisk(const std::string& approachId) const;

    void Reset();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, QueueMetrics> approachQueues_;
};

} // namespace aegis::coordinator::perception::analytics

