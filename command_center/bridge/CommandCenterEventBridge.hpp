/**
 * @file CommandCenterEventBridge.hpp
 * @brief EventBus subscription bridge listening to all coordinator, routing, hospital,
 * and roadside events to update Command Center state in a purely read-only manner.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/orchestration/event_bus/IEventBus.hpp"
#include "command_center/fleet/EmsFleetManager.hpp"
#include "command_center/tracking/LiveAmbulanceTracker.hpp"
#include "command_center/mission/MissionDatabase.hpp"
#include "command_center/hospital/HospitalOperationsDashboard.hpp"
#include "command_center/traffic/TrafficOperationsDashboard.hpp"
#include "command_center/cluster/ClusterMonitor.hpp"
#include "command_center/health/SystemHealthMonitor.hpp"
#include "command_center/analytics/RealTimeAnalyticsEngine.hpp"

#include <memory>
#include <vector>
#include <mutex>

namespace aegis::command_center {

class CommandCenterEventBridge {
public:
    CommandCenterEventBridge(std::shared_ptr<coordinator::orchestration::IEventBus> eventBus,
                             std::shared_ptr<EmsFleetManager> fleet,
                             std::shared_ptr<LiveAmbulanceTracker> tracker,
                             std::shared_ptr<MissionDatabase> missions,
                             std::shared_ptr<HospitalOperationsDashboard> hospitals,
                             std::shared_ptr<TrafficOperationsDashboard> traffic,
                             std::shared_ptr<ClusterMonitor> cluster,
                             std::shared_ptr<SystemHealthMonitor> health,
                             std::shared_ptr<RealTimeAnalyticsEngine> analytics);

    ~CommandCenterEventBridge();

    // Prevent copying
    CommandCenterEventBridge(const CommandCenterEventBridge&) = delete;
    CommandCenterEventBridge& operator=(const CommandCenterEventBridge&) = delete;

    /**
     * @brief Subscribe to all relevant operational events across coordinator and field subsystems.
     */
    void Connect();

    /**
     * @brief Disconnect and unsubscribe from EventBus.
     */
    void Disconnect();

    /**
     * @brief Check whether currently connected and actively subscribed.
     */
    bool IsConnected() const;

    /**
     * @brief Directly ingest an event (allows direct pipeline feeds and deterministic unit tests).
     */
    void IngestEvent(const coordinator::orchestration::Event& event);

    /**
     * @brief Number of active EventBus subscriptions maintained.
     */
    size_t GetActiveSubscriptionCount() const;

private:
    void HandleEmergencyCreated(const coordinator::orchestration::Event& event);
    void HandleEmergencyAssigned(const coordinator::orchestration::Event& event);
    void HandleHospitalAssigned(const coordinator::orchestration::Event& event);
    void HandleHospitalChanged(const coordinator::orchestration::Event& event);
    void HandleRouteChanged(const coordinator::orchestration::Event& event);
    void HandleETAUpdated(const coordinator::orchestration::Event& event);
    void HandleExecutionStarted(const coordinator::orchestration::Event& event);
    void HandleExecutionCompleted(const coordinator::orchestration::Event& event);
    void HandleMissionCompleted(const coordinator::orchestration::Event& event);
    void HandleTrafficIncident(const coordinator::orchestration::Event& event);
    void HandleRoadIncidentDetected(const coordinator::orchestration::Event& event);
    void HandleHeartbeatReceived(const coordinator::orchestration::Event& event);
    void HandleHeartbeatTimeout(const coordinator::orchestration::Event& event);
    void HandleNodeConnected(const coordinator::orchestration::Event& event);
    void HandleNodeDisconnected(const coordinator::orchestration::Event& event);

    std::shared_ptr<coordinator::orchestration::IEventBus> eventBus_;
    std::shared_ptr<EmsFleetManager> fleet_;
    std::shared_ptr<LiveAmbulanceTracker> tracker_;
    std::shared_ptr<MissionDatabase> missions_;
    std::shared_ptr<HospitalOperationsDashboard> hospitals_;
    std::shared_ptr<TrafficOperationsDashboard> traffic_;
    std::shared_ptr<ClusterMonitor> cluster_;
    std::shared_ptr<SystemHealthMonitor> health_;
    std::shared_ptr<RealTimeAnalyticsEngine> analytics_;

    mutable std::mutex mutex_;
    std::vector<coordinator::orchestration::SubscriptionId> subscriptionIds_;
    bool isConnected_{false};
};

} // namespace aegis::command_center
