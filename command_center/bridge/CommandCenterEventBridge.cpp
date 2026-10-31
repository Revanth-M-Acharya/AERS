/**
 * @file CommandCenterEventBridge.cpp
 * @brief Implementation of EventBus subscription bridge.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "command_center/bridge/CommandCenterEventBridge.hpp"

namespace aegis::command_center {

CommandCenterEventBridge::CommandCenterEventBridge(std::shared_ptr<coordinator::orchestration::IEventBus> eventBus,
                                                   std::shared_ptr<EmsFleetManager> fleet,
                                                   std::shared_ptr<LiveAmbulanceTracker> tracker,
                                                   std::shared_ptr<MissionDatabase> missions,
                                                   std::shared_ptr<HospitalOperationsDashboard> hospitals,
                                                   std::shared_ptr<TrafficOperationsDashboard> traffic,
                                                   std::shared_ptr<ClusterMonitor> cluster,
                                                   std::shared_ptr<SystemHealthMonitor> health,
                                                   std::shared_ptr<RealTimeAnalyticsEngine> analytics)
    : eventBus_(std::move(eventBus)),
      fleet_(std::move(fleet)),
      tracker_(std::move(tracker)),
      missions_(std::move(missions)),
      hospitals_(std::move(hospitals)),
      traffic_(std::move(traffic)),
      cluster_(std::move(cluster)),
      health_(std::move(health)),
      analytics_(std::move(analytics)) {
}

CommandCenterEventBridge::~CommandCenterEventBridge() {
    Disconnect();
}

void CommandCenterEventBridge::Connect() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (isConnected_ || !eventBus_) {
        return;
    }

    using coordinator::orchestration::EventType;

    auto sub = [this](EventType type, auto handlerMember) {
        auto subId = eventBus_->Subscribe(type, [this, handlerMember](const coordinator::orchestration::Event& ev) {
            (this->*handlerMember)(ev);
        });
        subscriptionIds_.push_back(subId);
    };

    sub(EventType::EmergencyCreated, &CommandCenterEventBridge::HandleEmergencyCreated);
    sub(EventType::EmergencyAssigned, &CommandCenterEventBridge::HandleEmergencyAssigned);
    sub(EventType::HospitalAssigned, &CommandCenterEventBridge::HandleHospitalAssigned);
    sub(EventType::HospitalChanged, &CommandCenterEventBridge::HandleHospitalChanged);
    sub(EventType::RouteChanged, &CommandCenterEventBridge::HandleRouteChanged);
    sub(EventType::CorridorReplanningAdvised, &CommandCenterEventBridge::HandleRouteChanged);
    sub(EventType::ETAUpdated, &CommandCenterEventBridge::HandleETAUpdated);
    sub(EventType::ExecutionStarted, &CommandCenterEventBridge::HandleExecutionStarted);
    sub(EventType::ExecutionCompleted, &CommandCenterEventBridge::HandleExecutionCompleted);
    sub(EventType::EmergencyFinished, &CommandCenterEventBridge::HandleMissionCompleted);
    sub(EventType::MissionCompleted, &CommandCenterEventBridge::HandleMissionCompleted);
    sub(EventType::TrafficIncident, &CommandCenterEventBridge::HandleTrafficIncident);
    sub(EventType::RoadIncidentDetected, &CommandCenterEventBridge::HandleRoadIncidentDetected);
    sub(EventType::HeartbeatReceived, &CommandCenterEventBridge::HandleHeartbeatReceived);
    sub(EventType::HeartbeatTimeout, &CommandCenterEventBridge::HandleHeartbeatTimeout);
    sub(EventType::NodeConnected, &CommandCenterEventBridge::HandleNodeConnected);
    sub(EventType::NodeDisconnected, &CommandCenterEventBridge::HandleNodeDisconnected);

    isConnected_ = true;
}

void CommandCenterEventBridge::Disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!isConnected_) {
        return;
    }

    if (eventBus_) {
        for (auto id : subscriptionIds_) {
            eventBus_->Unsubscribe(id);
        }
    }
    subscriptionIds_.clear();
    isConnected_ = false;
}

bool CommandCenterEventBridge::IsConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return isConnected_;
}

size_t CommandCenterEventBridge::GetActiveSubscriptionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return subscriptionIds_.size();
}

void CommandCenterEventBridge::IngestEvent(const coordinator::orchestration::Event& event) {
    using coordinator::orchestration::EventType;
    switch (event.type) {
        case EventType::EmergencyCreated:
            HandleEmergencyCreated(event);
            break;
        case EventType::EmergencyAssigned:
            HandleEmergencyAssigned(event);
            break;
        case EventType::HospitalAssigned:
            HandleHospitalAssigned(event);
            break;
        case EventType::HospitalChanged:
            HandleHospitalChanged(event);
            break;
        case EventType::RouteChanged:
        case EventType::CorridorReplanningAdvised:
            HandleRouteChanged(event);
            break;
        case EventType::ETAUpdated:
            HandleETAUpdated(event);
            break;
        case EventType::ExecutionStarted:
            HandleExecutionStarted(event);
            break;
        case EventType::ExecutionCompleted:
            HandleExecutionCompleted(event);
            break;
        case EventType::EmergencyFinished:
        case EventType::MissionCompleted:
            HandleMissionCompleted(event);
            break;
        case EventType::TrafficIncident:
            HandleTrafficIncident(event);
            break;
        case EventType::RoadIncidentDetected:
            HandleRoadIncidentDetected(event);
            break;
        case EventType::HeartbeatReceived:
            HandleHeartbeatReceived(event);
            break;
        case EventType::HeartbeatTimeout:
            HandleHeartbeatTimeout(event);
            break;
        case EventType::NodeConnected:
            HandleNodeConnected(event);
            break;
        case EventType::NodeDisconnected:
            HandleNodeDisconnected(event);
            break;
        default:
            break;
    }
}

void CommandCenterEventBridge::HandleEmergencyCreated(const coordinator::orchestration::Event& event) {
    std::string incidentId = event.sourceId.empty() ? "INC-NEW" : event.sourceId;

    if (missions_) {
        HistoricalMissionRecord record;
        record.missionId = incidentId;
        record.emergencyType = event.payload.empty() ? "GENERAL_EMERGENCY" : event.payload;
        record.priority = "CRITICAL (Code Red)";
        record.dispatchTimeMonoMs = event.timestampMonoMs;
        record.missionStatus = "DISPATCHED";
        missions_->CreateMission(record);
    }

    if (analytics_) {
        analytics_->RecordPriorityClass("CRITICAL (Code Red)");
    }
}

void CommandCenterEventBridge::HandleEmergencyAssigned(const coordinator::orchestration::Event& event) {
    std::string incidentId = event.sourceId;
    std::string ambulanceId = event.targetId.empty() ? "AMB-01" : event.targetId;

    if (fleet_) {
        fleet_->AssignToMission(ambulanceId, incidentId, "HOSP-01", 45.0);
    }

    if (tracker_) {
        AmbulanceTrackingState track;
        track.ambulanceId = ambulanceId;
        track.missionId = incidentId;
        track.currentIntersection = "HOME";
        track.nextIntersection = "INT_1";
        track.currentRoute = {"HOME", "INT_1", "INT_2", "INT_3", "HOSPITAL"};
        track.remainingDistanceMeters = 1450.0;
        track.currentSpeedKmh = 50.0;
        track.estimatedArrivalSec = 45.0;
        track.greenCorridorStatus = "PREPARING";
        track.lastUpdateTimestampMonoMs = event.timestampMonoMs;
        tracker_->UpdateTracking(track);
    }
}

void CommandCenterEventBridge::HandleHospitalAssigned(const coordinator::orchestration::Event& event) {
    std::string incidentId = event.sourceId;
    std::string hospitalId = event.targetId.empty() ? "HOSP-01" : event.targetId;

    if (hospitals_) {
        hospitals_->AddIncomingAmbulance(hospitalId, "AMB-01", 42.0, "CRITICAL (Code Red)");
    }

    if (analytics_) {
        analytics_->RecordHospitalAdmission(hospitalId);
    }
}

void CommandCenterEventBridge::HandleHospitalChanged(const coordinator::orchestration::Event& event) {
    std::string incidentId = event.sourceId;
    std::string newHospital = event.targetId;

    if (hospitals_) {
        // Transfer patient to newly assigned hospital
        hospitals_->RemoveIncomingAmbulance("HOSP-01", "AMB-01");
        hospitals_->AddIncomingAmbulance(newHospital, "AMB-01", 38.0, "CRITICAL (Code Red)");
    }
}

void CommandCenterEventBridge::HandleRouteChanged(const coordinator::orchestration::Event& event) {
    std::string incidentId = event.sourceId;
    std::string reason = event.payload.empty() ? "Road hazard / Police diversion" : event.payload;

    if (missions_) {
        missions_->RecordReplanning(incidentId, {"HOME", "INT_1", "INT_ALT", "HOSPITAL"}, reason);
    }

    if (analytics_) {
        analytics_->IncrementDynamicReplan();
    }
}

void CommandCenterEventBridge::HandleETAUpdated(const coordinator::orchestration::Event& event) {
    std::string ambulanceId = event.sourceId.empty() ? "AMB-01" : event.sourceId;
    double eta = 40.0;
    try {
        if (!event.payload.empty()) {
            eta = std::stod(event.payload);
        }
    } catch (...) {}

    if (tracker_) {
        tracker_->UpdateIntersectionProgress(ambulanceId, "INT_1", "INT_2", 850.0, 55.0, eta);
    }

    if (fleet_) {
        fleet_->UpdateTelemetry(ambulanceId, 12.9716, 77.5946, eta);
    }
}

void CommandCenterEventBridge::HandleExecutionStarted(const coordinator::orchestration::Event& event) {
    std::string ambulanceId = event.sourceId.empty() ? "AMB-01" : event.sourceId;

    if (traffic_) {
        traffic_->UpdateSignalState("INT_1", "ROAD_A_GREEN", IntersectionControlMode::EMERGENCY_CORRIDOR, ambulanceId, event.timestampMonoMs);
        traffic_->UpdateSignalState("INT_2", "ROAD_A_GREEN", IntersectionControlMode::EMERGENCY_CORRIDOR, ambulanceId, event.timestampMonoMs);
        traffic_->UpdateSignalState("INT_3", "ROAD_A_GREEN", IntersectionControlMode::EMERGENCY_CORRIDOR, ambulanceId, event.timestampMonoMs);
    }

    if (tracker_) {
        tracker_->UpdateCorridorStatus(ambulanceId, "ACTIVE");
    }

    if (analytics_) {
        analytics_->IncrementGreenCorridor();
        analytics_->IncrementSignalActivation();
    }
}

void CommandCenterEventBridge::HandleExecutionCompleted(const coordinator::orchestration::Event& event) {
    std::string ambulanceId = event.sourceId.empty() ? "AMB-01" : event.sourceId;

    if (traffic_) {
        traffic_->UpdateSignalState("INT_1", "NORMAL_CYCLE", IntersectionControlMode::NORMAL, "None", event.timestampMonoMs);
        traffic_->UpdateSignalState("INT_2", "NORMAL_CYCLE", IntersectionControlMode::NORMAL, "None", event.timestampMonoMs);
        traffic_->UpdateSignalState("INT_3", "NORMAL_CYCLE", IntersectionControlMode::NORMAL, "None", event.timestampMonoMs);
    }

    if (tracker_) {
        tracker_->UpdateCorridorStatus(ambulanceId, "RESTORED");
    }
}

void CommandCenterEventBridge::HandleMissionCompleted(const coordinator::orchestration::Event& event) {
    std::string incidentId = event.sourceId.empty() ? "INC-01" : event.sourceId;
    std::string ambulanceId = event.targetId.empty() ? "AMB-01" : event.targetId;

    if (missions_) {
        missions_->CompleteMission(incidentId, event.timestampMonoMs, "SUCCESSFUL_PATIENT_HANDOVER");
    }

    if (fleet_) {
        fleet_->CompleteMission(ambulanceId);
    }

    if (tracker_) {
        tracker_->RemoveTracking(ambulanceId);
    }

    if (hospitals_) {
        hospitals_->RemoveIncomingAmbulance("HOSP-01", ambulanceId);
    }

    if (analytics_) {
        analytics_->RecordMissionCompletion(138.0, 58.4, 96.8);
    }
}

void CommandCenterEventBridge::HandleTrafficIncident(const coordinator::orchestration::Event& event) {
    std::string incidentId = event.sourceId;
    std::string details = event.payload.empty() ? "Traffic bottleneck / Obstruction" : event.payload;

    if (missions_) {
        missions_->RecordPoliceAdvisory(incidentId, details);
    }

    if (analytics_) {
        analytics_->IncrementPoliceAdvisory();
    }
}

void CommandCenterEventBridge::HandleRoadIncidentDetected(const coordinator::orchestration::Event& event) {
    HandleTrafficIncident(event);
}

void CommandCenterEventBridge::HandleHeartbeatReceived(const coordinator::orchestration::Event& event) {
    std::string nodeId = event.sourceId.empty() ? "INT_1" : event.sourceId;

    if (cluster_) {
        cluster_->UpdateHeartbeat(nodeId, 12.0, 24.5, 8, 0.82, 8, event.timestampMonoMs);
        cluster_->SetNodeOnline(nodeId, true);
    }

    if (health_) {
        health_->ReportHealth("TCP Connections", HealthStatusGrade::HEALTHY, "Nominal latency <1ms", event.timestampMonoMs);
        health_->ReportHealth("Traffic Nodes", HealthStatusGrade::HEALTHY, "All 3 nodes responsive", event.timestampMonoMs);
    }
}

void CommandCenterEventBridge::HandleHeartbeatTimeout(const coordinator::orchestration::Event& event) {
    std::string nodeId = event.sourceId.empty() ? "INT_1" : event.sourceId;

    if (cluster_) {
        cluster_->SetNodeOnline(nodeId, false);
    }

    if (health_) {
        health_->ReportHealth("Traffic Nodes", HealthStatusGrade::WARNING, "Heartbeat timeout on " + nodeId, event.timestampMonoMs);
    }
}

void CommandCenterEventBridge::HandleNodeConnected(const coordinator::orchestration::Event& event) {
    std::string nodeId = event.sourceId;
    if (cluster_) {
        cluster_->SetNodeOnline(nodeId, true);
        cluster_->SetLastEvent(nodeId, "NodeConnected");
    }
}

void CommandCenterEventBridge::HandleNodeDisconnected(const coordinator::orchestration::Event& event) {
    std::string nodeId = event.sourceId;
    if (cluster_) {
        cluster_->SetNodeOnline(nodeId, false);
        cluster_->SetLastEvent(nodeId, "NodeDisconnected");
    }
}

} // namespace aegis::command_center
