/**
 * @file CommandCenterFacade.hpp
 * @brief Unified facade for SMART AEGIS Phase 13 Operations, Monitoring & Command Center.
 * Coordinates all telemetry dashboards, fleet inventory, mission history, cluster monitors,
 * replay engines, and EventBus bridges with a clean lifecycle API.
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
#include "command_center/replay/MissionReplayController.hpp"
#include "command_center/dashboard/CommandCenterConsole.hpp"
#include "command_center/bridge/CommandCenterEventBridge.hpp"

#include <memory>
#include <string>
#include <ostream>

namespace aegis::command_center {

class CommandCenterFacade {
public:
    CommandCenterFacade();
    explicit CommandCenterFacade(std::shared_ptr<coordinator::orchestration::IEventBus> eventBus);
    ~CommandCenterFacade();

    // Prevent copying
    CommandCenterFacade(const CommandCenterFacade&) = delete;
    CommandCenterFacade& operator=(const CommandCenterFacade&) = delete;

    /**
     * @brief Initialize all command center subsystems with default benchmark topology.
     */
    void Initialize(std::shared_ptr<coordinator::orchestration::IEventBus> eventBus = nullptr);

    /**
     * @brief Start command center operations and attach event subscriptions.
     */
    void Start();

    /**
     * @brief Stop command center and detach event subscriptions.
     */
    void Stop();

    /**
     * @brief Check whether command center is running.
     */
    bool IsRunning() const;

    // Subsystem accessors
    std::shared_ptr<EmsFleetManager> GetFleetManager() const { return fleet_; }
    std::shared_ptr<LiveAmbulanceTracker> GetAmbulanceTracker() const { return tracker_; }
    std::shared_ptr<MissionDatabase> GetMissionDatabase() const { return missions_; }
    std::shared_ptr<HospitalOperationsDashboard> GetHospitalDashboard() const { return hospitals_; }
    std::shared_ptr<TrafficOperationsDashboard> GetTrafficDashboard() const { return traffic_; }
    std::shared_ptr<ClusterMonitor> GetClusterMonitor() const { return cluster_; }
    std::shared_ptr<SystemHealthMonitor> GetHealthMonitor() const { return health_; }
    std::shared_ptr<RealTimeAnalyticsEngine> GetAnalyticsEngine() const { return analytics_; }
    std::shared_ptr<MissionReplayController> GetReplayController() const { return replay_; }
    std::shared_ptr<CommandCenterConsole> GetConsole() const { return console_; }
    std::shared_ptr<CommandCenterEventBridge> GetEventBridge() const { return eventBridge_; }

    /**
     * @brief Load a mission into the replay controller by ID.
     */
    bool LoadMissionForReplay(const std::string& missionId);

    /**
     * @brief Render the entire unified Operations Command Center ASCII view.
     */
    std::string RenderDashboard() const;

    /**
     * @brief Print dashboard to specified stream.
     */
    void PrintDashboard(std::ostream& os) const;

private:
    std::shared_ptr<coordinator::orchestration::IEventBus> eventBus_;
    std::shared_ptr<EmsFleetManager> fleet_;
    std::shared_ptr<LiveAmbulanceTracker> tracker_;
    std::shared_ptr<MissionDatabase> missions_;
    std::shared_ptr<HospitalOperationsDashboard> hospitals_;
    std::shared_ptr<TrafficOperationsDashboard> traffic_;
    std::shared_ptr<ClusterMonitor> cluster_;
    std::shared_ptr<SystemHealthMonitor> health_;
    std::shared_ptr<RealTimeAnalyticsEngine> analytics_;
    std::shared_ptr<MissionReplayController> replay_;
    std::shared_ptr<CommandCenterConsole> console_;
    std::shared_ptr<CommandCenterEventBridge> eventBridge_;

    bool isRunning_{false};
};

} // namespace aegis::command_center
