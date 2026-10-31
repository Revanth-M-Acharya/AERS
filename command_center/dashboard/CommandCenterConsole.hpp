/**
 * @file CommandCenterConsole.hpp
 * @brief Professional ASCII operations console formatting all subsystem telemetry, fleet status,
 * mission queue, intersection states, hospital loads, cluster hardware, and KPIs.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "command_center/fleet/EmsFleetManager.hpp"
#include "command_center/tracking/LiveAmbulanceTracker.hpp"
#include "command_center/mission/MissionDatabase.hpp"
#include "command_center/hospital/HospitalOperationsDashboard.hpp"
#include "command_center/traffic/TrafficOperationsDashboard.hpp"
#include "command_center/cluster/ClusterMonitor.hpp"
#include "command_center/health/SystemHealthMonitor.hpp"
#include "command_center/analytics/RealTimeAnalyticsEngine.hpp"

#include <string>
#include <memory>
#include <ostream>

namespace aegis::command_center {

class CommandCenterConsole {
public:
    CommandCenterConsole(std::shared_ptr<EmsFleetManager> fleet,
                         std::shared_ptr<LiveAmbulanceTracker> tracker,
                         std::shared_ptr<MissionDatabase> missions,
                         std::shared_ptr<HospitalOperationsDashboard> hospitals,
                         std::shared_ptr<TrafficOperationsDashboard> traffic,
                         std::shared_ptr<ClusterMonitor> cluster,
                         std::shared_ptr<SystemHealthMonitor> health,
                         std::shared_ptr<RealTimeAnalyticsEngine> analytics);

    ~CommandCenterConsole() = default;

    // Prevent copying
    CommandCenterConsole(const CommandCenterConsole&) = delete;
    CommandCenterConsole& operator=(const CommandCenterConsole&) = delete;

    /**
     * @brief Render the executive summary header exactly matching the Phase 13 specification format.
     */
    std::string RenderExecutiveHeader() const;

    /**
     * @brief Render the active and recent emergency mission queue.
     */
    std::string RenderMissionQueue() const;

    /**
     * @brief Render roadside intersection signals, control modes, and vehicle queues.
     */
    std::string RenderIntersectionStatus() const;

    /**
     * @brief Render hospital trauma capacities, available beds, and condition codes.
     */
    std::string RenderHospitalStatus() const;

    /**
     * @brief Render Raspberry Pi cluster nodes (CPU, RAM, latency, clock drift).
     */
    std::string RenderClusterStatus() const;

    /**
     * @brief Render hardware/software subsystem health status.
     */
    std::string RenderSystemHealth() const;

    /**
     * @brief Render real-time operations analytics and KPI metrics.
     */
    std::string RenderAnalytics() const;

    /**
     * @brief Render the entire unified Operations Command Center ASCII view.
     */
    std::string RenderFullDashboard() const;

    /**
     * @brief Stream full dashboard to output stream.
     */
    void Print(std::ostream& os) const;

private:
    std::shared_ptr<EmsFleetManager> fleet_;
    std::shared_ptr<LiveAmbulanceTracker> tracker_;
    std::shared_ptr<MissionDatabase> missions_;
    std::shared_ptr<HospitalOperationsDashboard> hospitals_;
    std::shared_ptr<TrafficOperationsDashboard> traffic_;
    std::shared_ptr<ClusterMonitor> cluster_;
    std::shared_ptr<SystemHealthMonitor> health_;
    std::shared_ptr<RealTimeAnalyticsEngine> analytics_;

    static std::string FormatSecondsToMinSec(double seconds);
};

} // namespace aegis::command_center
