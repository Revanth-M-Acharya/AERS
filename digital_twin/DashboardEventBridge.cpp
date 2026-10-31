/**
 * @file DashboardEventBridge.cpp
 * @brief Implementation of DashboardEventBridge read-only subscriber.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "digital_twin/DashboardEventBridge.hpp"

namespace aegis::digital_twin {

DashboardEventBridge::DashboardEventBridge(
    std::shared_ptr<coordinator::orchestration::IEventBus> eventBus,
    std::shared_ptr<CityModel> cityModel,
    std::shared_ptr<TelemetryCollector> telemetryCollector)
    : eventBus_(std::move(eventBus)),
      cityModel_(std::move(cityModel)),
      telemetryCollector_(std::move(telemetryCollector)) {
    StartListening();
}

DashboardEventBridge::~DashboardEventBridge() {
    StopListening();
}

void DashboardEventBridge::StartListening() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (isListening_ || !eventBus_) return;

    auto subId = eventBus_->SubscribeAll([this](const coordinator::orchestration::Event& ev) {
        this->HandleEvent(ev);
    });
    subscriptionIds_.push_back(subId);
    isListening_ = true;
}

void DashboardEventBridge::StopListening() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!isListening_ || !eventBus_) return;

    for (auto id : subscriptionIds_) {
        eventBus_->Unsubscribe(id);
    }
    subscriptionIds_.clear();
    isListening_ = false;
}

bool DashboardEventBridge::IsListening() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return isListening_;
}

size_t DashboardEventBridge::GetProcessedEventCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return processedEventsCount_;
}

void DashboardEventBridge::HandleEvent(const coordinator::orchestration::Event& event) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        processedEventsCount_++;
    }

    if (telemetryCollector_) {
        telemetryCollector_->RecordEventDispatched();
    }

    using EventType = coordinator::orchestration::EventType;

    switch (event.type) {
        case EventType::NodeConnected:
        case EventType::HeartbeatReceived:
            if (telemetryCollector_) telemetryCollector_->RecordNodeHeartbeat(event.sourceId);
            if (cityModel_) cityModel_->UpdateIntersectionHeartbeat(event.sourceId, event.timestampMonoMs);
            break;

        case EventType::NodeDisconnected:
            if (telemetryCollector_) telemetryCollector_->RecordNodeDisconnect(event.sourceId);
            break;

        case EventType::HeartbeatTimeout:
            if (telemetryCollector_) telemetryCollector_->RecordNodeMissedHeartbeat(event.sourceId);
            break;

        case EventType::RouteChanged:
            if (telemetryCollector_) telemetryCollector_->IncrementRouteReplans();
            break;

        case EventType::ExecutionStarted:
            if (telemetryCollector_) telemetryCollector_->IncrementSignalSyncs();
            if (cityModel_ && !event.targetId.empty()) {
                cityModel_->UpdateIntersectionSignal(event.targetId, "GREEN", "PREEMPTED", event.sourceId);
            }
            break;

        case EventType::ExecutionCompleted:
        case EventType::EmergencyFinished:
        case EventType::MissionCompleted:
            if (telemetryCollector_) telemetryCollector_->RecordCompletedMission(48.0);
            if (cityModel_ && !event.targetId.empty()) {
                cityModel_->UpdateIntersectionSignal(event.targetId, "RED", "RESTORED", "");
            }
            break;

        case EventType::HospitalAssigned:
        case EventType::HospitalChanged:
        case EventType::HospitalUpdated:
            if (telemetryCollector_) telemetryCollector_->IncrementHospitalNotifications();
            if (cityModel_) {
                cityModel_->AssignAmbulanceToHospital(event.targetId, event.sourceId);
            }
            break;

        case EventType::RoadIncidentDetected:
        case EventType::RoadConditionUpdated: {
            if (telemetryCollector_) telemetryCollector_->IncrementCameraEvents();
            if (cityModel_) {
                RoadConditionEntity cond;
                cond.eventId = event.sourceId.empty() ? "EVT_UNKNOWN" : event.sourceId;
                cond.incidentId = event.targetId.empty() ? cond.eventId : event.targetId;
                cond.roadSegment = "SEG_INTA_INTB";
                cond.type = "INCIDENT";
                cond.severity = "HIGH";
                cond.confidence = 0.90;
                cond.lifecycleState = "ACTIVE";
                cond.timestampMonoMs = event.timestampMonoMs;
                cityModel_->AddOrUpdateRoadCondition(cond);
            }
            break;
        }

        case EventType::CameraStatusChanged:
            if (telemetryCollector_) telemetryCollector_->IncrementCameraEvents();
            break;

        default:
            break;
    }
}

} // namespace aegis::digital_twin

