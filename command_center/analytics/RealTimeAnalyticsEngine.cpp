/**
 * @file RealTimeAnalyticsEngine.cpp
 * @brief Implementation of Real-Time Analytics Engine.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "command_center/analytics/RealTimeAnalyticsEngine.hpp"
#include <numeric>

namespace aegis::command_center {

RealTimeAnalyticsEngine::RealTimeAnalyticsEngine() {
    InitializeDefaultAnalytics();
}

void RealTimeAnalyticsEngine::InitializeDefaultAnalytics() {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_ = RealTimeAnalyticsSnapshot();

    snapshot_.todaysMissionsCount = 15;
    snapshot_.completedMissionsCount = 14;
    snapshot_.averageResponseTimeSec = 138.0; // 2m 18s
    snapshot_.averageCorridorTimeSec = 58.4;
    snapshot_.averageEtaAccuracyPercent = 96.8;
    snapshot_.greenCorridorsCount = 18;
    snapshot_.policeAdvisoriesCount = 2;
    snapshot_.dynamicReplansCount = 2;
    snapshot_.trafficSignalActivations = 54;
    snapshot_.coordinatorCpuUsagePercent = 14.8;
    snapshot_.coordinatorMemoryUsagePercent = 28.4;
    snapshot_.ambulanceUtilizationPercent = 75.0;

    snapshot_.hospitalUsage = {
        {"City Trauma Hospital",          6},
        {"Metro Heart Institute",         4},
        {"St. Jude Multi-Specialty",       2},
        {"Apex Pediatric Hospital",       1},
        {"Greenfield Emergency Hospital", 1}
    };

    snapshot_.priorityDistribution = {
        {"CRITICAL (Code Red)",    9},
        {"HIGH (Code Yellow)",     4},
        {"MEDIUM (Code Green)",    2}
    };

    responseTimes_ = {135.0, 142.0, 130.0, 145.0, 138.0, 140.0, 134.0, 136.0, 144.0, 137.0, 139.0, 133.0, 141.0, 138.0};
    corridorTimes_ = {56.0, 59.0, 54.0, 62.0, 58.0, 60.0, 55.0, 57.0, 61.0, 58.0, 59.0, 56.0, 60.0, 58.0};
    etaAccuracies_ = {97.0, 96.5, 98.0, 95.5, 97.2, 96.8, 97.5, 96.2, 95.8, 97.1, 96.9, 97.4, 96.0, 97.3};
}

void RealTimeAnalyticsEngine::RecordMissionCompletion(double responseTimeSec,
                                                     double corridorTimeSec,
                                                     double etaAccuracyPercent) {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.todaysMissionsCount++;
    snapshot_.completedMissionsCount++;

    responseTimes_.push_back(responseTimeSec);
    corridorTimes_.push_back(corridorTimeSec);
    etaAccuracies_.push_back(etaAccuracyPercent);

    if (!responseTimes_.empty()) {
        snapshot_.averageResponseTimeSec = std::accumulate(responseTimes_.begin(), responseTimes_.end(), 0.0) / static_cast<double>(responseTimes_.size());
    }
    if (!corridorTimes_.empty()) {
        snapshot_.averageCorridorTimeSec = std::accumulate(corridorTimes_.begin(), corridorTimes_.end(), 0.0) / static_cast<double>(corridorTimes_.size());
    }
    if (!etaAccuracies_.empty()) {
        snapshot_.averageEtaAccuracyPercent = std::accumulate(etaAccuracies_.begin(), etaAccuracies_.end(), 0.0) / static_cast<double>(etaAccuracies_.size());
    }
}

void RealTimeAnalyticsEngine::IncrementGreenCorridor() {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.greenCorridorsCount++;
}

void RealTimeAnalyticsEngine::IncrementPoliceAdvisory() {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.policeAdvisoriesCount++;
}

void RealTimeAnalyticsEngine::IncrementDynamicReplan() {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.dynamicReplansCount++;
}

void RealTimeAnalyticsEngine::IncrementSignalActivation() {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.trafficSignalActivations++;
}

void RealTimeAnalyticsEngine::RecordHospitalAdmission(const std::string& hospitalName) {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.hospitalUsage[hospitalName]++;
}

void RealTimeAnalyticsEngine::RecordPriorityClass(const std::string& priorityLevel) {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.priorityDistribution[priorityLevel]++;
}

void RealTimeAnalyticsEngine::UpdateResourceMetrics(double cpuPercent, double memPercent, double ambulanceUtilPercent) {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.coordinatorCpuUsagePercent = cpuPercent;
    snapshot_.coordinatorMemoryUsagePercent = memPercent;
    snapshot_.ambulanceUtilizationPercent = ambulanceUtilPercent;
}

RealTimeAnalyticsSnapshot RealTimeAnalyticsEngine::GetSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_;
}

void RealTimeAnalyticsEngine::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_ = RealTimeAnalyticsSnapshot();
    responseTimes_.clear();
    corridorTimes_.clear();
    etaAccuracies_.clear();
}

} // namespace aegis::command_center
