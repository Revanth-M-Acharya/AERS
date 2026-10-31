/**
 * @file QueueLengthEstimator.cpp
 * @brief Implementation of queue length estimation and green corridor pre-clearance times.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/perception/analytics/QueueLengthEstimator.hpp"
#include "common/utils/TimeUtils.hpp"
#include <algorithm>

namespace aegis::coordinator::perception::analytics {

QueueMetrics QueueLengthEstimator::UpdateQueue(
    const std::string& approachId,
    uint32_t stoppedVehicleCount,
    double segmentLengthMeters,
    double averageVehicleSpacingMeters,
    double dischargeRateVehPerSec) {

    if (averageVehicleSpacingMeters <= 0.0) averageVehicleSpacingMeters = 6.5;
    if (dischargeRateVehPerSec <= 0.0) dischargeRateVehPerSec = 0.5;

    double queueLengthM = static_cast<double>(stoppedVehicleCount) * averageVehicleSpacingMeters;

    // Start-up lost time (~2.5s) + discharge time
    double clearanceSec = 0.0;
    if (stoppedVehicleCount > 0) {
        clearanceSec = 2.5 + (static_cast<double>(stoppedVehicleCount) / dischargeRateVehPerSec);
    }

    bool spillback = (segmentLengthMeters > 0.0) && (queueLengthM >= 0.80 * segmentLengthMeters);

    std::lock_guard<std::mutex> lock(mutex_);
    QueueMetrics metrics;
    metrics.approachId = approachId;
    metrics.queuedVehicleCount = stoppedVehicleCount;
    metrics.queueLengthMeters = queueLengthM;
    metrics.estimatedClearanceSec = clearanceSec;
    metrics.isSpillbackRisk = spillback;
    metrics.lastUpdatedMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    approachQueues_[approachId] = metrics;
    return metrics;
}

QueueMetrics QueueLengthEstimator::GetQueue(const std::string& approachId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = approachQueues_.find(approachId);
    if (it != approachQueues_.end()) return it->second;
    return QueueMetrics{approachId};
}

double QueueLengthEstimator::GetRequiredClearanceTime(const std::string& approachId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = approachQueues_.find(approachId);
    if (it != approachQueues_.end()) return it->second.estimatedClearanceSec;
    return 0.0;
}

bool QueueLengthEstimator::HasSpillbackRisk(const std::string& approachId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = approachQueues_.find(approachId);
    if (it != approachQueues_.end()) return it->second.isSpillbackRisk;
    return false;
}

void QueueLengthEstimator::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    approachQueues_.clear();
}

} // namespace aegis::coordinator::perception::analytics

