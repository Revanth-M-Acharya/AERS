/**
 * @file RealTimeAnalyticsEngine.hpp
 * @brief Real-time operational analytics engine generating aggregated KPIs, resource distributions, and SLA metrics.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace aegis::command_center {

struct RealTimeAnalyticsSnapshot {
    uint32_t todaysMissionsCount{15};
    uint32_t completedMissionsCount{14};
    double averageResponseTimeSec{138.0}; // 2m 18s
    double averageCorridorTimeSec{58.4};  // Transit duration through preemption wave
    double averageEtaAccuracyPercent{96.8};
    uint32_t greenCorridorsCount{18};
    std::unordered_map<std::string, uint32_t> hospitalUsage;
    double ambulanceUtilizationPercent{75.0}; // 3 of 4 active, or fleet ratio
    std::unordered_map<std::string, uint32_t> priorityDistribution;
    uint32_t policeAdvisoriesCount{2};
    uint32_t dynamicReplansCount{2};
    uint32_t trafficSignalActivations{54};
    double coordinatorCpuUsagePercent{14.8};
    double coordinatorMemoryUsagePercent{28.4};
};

class RealTimeAnalyticsEngine {
public:
    RealTimeAnalyticsEngine();
    ~RealTimeAnalyticsEngine() = default;

    // Prevent copying
    RealTimeAnalyticsEngine(const RealTimeAnalyticsEngine&) = delete;
    RealTimeAnalyticsEngine& operator=(const RealTimeAnalyticsEngine&) = delete;

    /**
     * @brief Populate default realistic benchmark data for initial operations center startup.
     */
    void InitializeDefaultAnalytics();

    /**
     * @brief Record completed mission timings and recalculate running averages.
     */
    void RecordMissionCompletion(double responseTimeSec, double corridorTimeSec, double etaAccuracyPercent);

    /**
     * @brief Increment counter for green corridor creation.
     */
    void IncrementGreenCorridor();

    /**
     * @brief Increment counter for verified police advisory report.
     */
    void IncrementPoliceAdvisory();

    /**
     * @brief Increment counter for autonomous dynamic route replanning.
     */
    void IncrementDynamicReplan();

    /**
     * @brief Increment traffic signal preemption activation counter.
     */
    void IncrementSignalActivation();

    /**
     * @brief Record admission to a hospital.
     */
    void RecordHospitalAdmission(const std::string& hospitalName);

    /**
     * @brief Record mission priority assignment into distribution.
     */
    void RecordPriorityClass(const std::string& priorityLevel);

    /**
     * @brief Update cluster resource consumption metrics.
     */
    void UpdateResourceMetrics(double cpuPercent, double memPercent, double ambulanceUtilPercent);

    /**
     * @brief Retrieve snapshot of all aggregated KPIs.
     */
    RealTimeAnalyticsSnapshot GetSnapshot() const;

    /**
     * @brief Clear all analytics counters.
     */
    void Clear();

private:
    mutable std::mutex mutex_;
    RealTimeAnalyticsSnapshot snapshot_;
    std::vector<double> responseTimes_;
    std::vector<double> corridorTimes_;
    std::vector<double> etaAccuracies_;
};

} // namespace aegis::command_center
