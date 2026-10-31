/**
 * @file SimulationScenarioManager.cpp
 * @brief Implementation of SimulationScenarioManager.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "digital_twin/SimulationScenarioManager.hpp"
#include <iostream>

namespace aegis::digital_twin {

std::string ScenarioTypeToString(ScenarioType type) {
    switch (type) {
        case ScenarioType::SINGLE_AMBULANCE:     return "SINGLE_AMBULANCE";
        case ScenarioType::MULTIPLE_AMBULANCES:   return "MULTIPLE_AMBULANCES";
        case ScenarioType::ACCIDENT:             return "ACCIDENT";
        case ScenarioType::FLOODING:             return "FLOODING";
        case ScenarioType::ROAD_CLOSURE:         return "ROAD_CLOSURE";
        case ScenarioType::HEAVY_CONGESTION:     return "HEAVY_CONGESTION";
        case ScenarioType::CAMERA_FAILURE:       return "CAMERA_FAILURE";
        case ScenarioType::HOSPITAL_DIVERSION:   return "HOSPITAL_DIVERSION";
        case ScenarioType::COMMUNICATION_DELAY:  return "COMMUNICATION_DELAY";
        case ScenarioType::NODE_FAILURE:         return "NODE_FAILURE";
        case ScenarioType::COORDINATOR_RECOVERY: return "COORDINATOR_RECOVERY";
    }
    return "UNKNOWN";
}

SimulationScenarioManager::SimulationScenarioManager(
    std::shared_ptr<CityModel> cityModel,
    std::shared_ptr<MissionTimeline> timeline)
    : cityModel_(std::move(cityModel)),
      timeline_(std::move(timeline)) {
    playbackController_.SetTriggerCallback([this](const ScenarioScriptEvent& ev) {
        this->HandleScenarioEvent(ev);
    });
}

bool SimulationScenarioManager::LoadScenario(ScenarioType type) {
    std::lock_guard<std::mutex> lock(mutex_);
    currentScenario_ = type;
    auto script = GenerateScenarioScript(type);
    playbackController_.LoadScript(script);
    isActive_ = true;

    if (timeline_) {
        timeline_->RecordEvent("SCENARIO_" + ScenarioTypeToString(type), "SIM_MANAGER", "SCENARIO_LOADED",
            "Loaded simulation scenario: " + ScenarioTypeToString(type), "INFO");
    }
    return true;
}

void SimulationScenarioManager::Update(double currentSimTimeSeconds) {
    playbackController_.Update(currentSimTimeSeconds);
    if (playbackController_.IsComplete()) {
        std::lock_guard<std::mutex> lock(mutex_);
        isActive_ = false;
    }
}

void SimulationScenarioManager::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    playbackController_.Reset();
    isActive_ = false;
}

bool SimulationScenarioManager::IsActive() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return isActive_;
}

bool SimulationScenarioManager::IsComplete() const {
    return playbackController_.IsComplete();
}

ScenarioType SimulationScenarioManager::GetCurrentScenario() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return currentScenario_;
}

size_t SimulationScenarioManager::GetTotalEvents() const {
    return playbackController_.GetTotalEvents();
}

size_t SimulationScenarioManager::GetExecutedEvents() const {
    return playbackController_.GetExecutedEventsCount();
}

void SimulationScenarioManager::HandleScenarioEvent(const ScenarioScriptEvent& event) {
    if (timeline_) {
        timeline_->RecordEvent("SCENARIO_" + ScenarioTypeToString(currentScenario_),
            event.targetEntity, event.eventType, event.action + " - " + event.payload, "ALERT");
    }

    if (!cityModel_) return;

    if (event.eventType == "SIGNAL_PREEMPTION") {
        cityModel_->UpdateIntersectionSignal(event.targetEntity, "GREEN", "PREEMPTED", "CORR_SIM");
    } else if (event.eventType == "SIGNAL_RESTORE") {
        cityModel_->UpdateIntersectionSignal(event.targetEntity, "RED", "RESTORED", "");
    } else if (event.eventType == "ROAD_HAZARD") {
        RoadConditionEntity cond;
        cond.eventId = "EVT_SIM_" + event.targetEntity;
        cond.incidentId = "INC_SIM_" + event.targetEntity;
        cond.roadSegment = event.targetEntity;
        cond.type = event.action;
        cond.severity = "CRITICAL";
        cond.confidence = 0.95;
        cond.isBlocking = (event.action == "ROAD_CLOSED" || event.action == "FLOODING");
        cond.lifecycleState = "ACTIVE";
        cityModel_->AddOrUpdateRoadCondition(cond);
        if (cond.isBlocking) {
            cityModel_->UpdateSegmentTraffic(event.targetEntity, 1.0, 0.0, 0.0, "IMPASSABLE", true);
        }
    } else if (event.eventType == "CAMERA_EVENT") {
        cityModel_->UpdateCameraStatus(event.targetEntity, event.action, 0.30, 0.0, 50, false);
    } else if (event.eventType == "HOSPITAL_EVENT") {
        cityModel_->UpdateHospitalCapacity(event.targetEntity, 0, 0, 100.0, true);
    } else if (event.eventType == "AMBULANCE_UPDATE") {
        cityModel_->UpdateAmbulanceTelemetry(event.targetEntity, 37.7800, -122.4150, 65.0, "SEG_INTA_INTB", 45.0, event.action);
    }
}

std::vector<ScenarioScriptEvent> SimulationScenarioManager::GenerateScenarioScript(ScenarioType type) {
    std::vector<ScenarioScriptEvent> s;

    switch (type) {
        case ScenarioType::SINGLE_AMBULANCE:
            s.push_back({0.0, "AMBULANCE_UPDATE", "AMB_01", "DISPATCHED", "Cardiac Emergency 911 Call"});
            s.push_back({5.0, "CORRIDOR_PLAN", "CORRIDOR_01", "PLANNED", "Route: INT_A -> INT_B -> INT_C"});
            s.push_back({10.0, "SIGNAL_PREEMPTION", "INT_A", "PREEMPT", "Green Wave Preemption Wave 1"});
            s.push_back({25.0, "SIGNAL_PREEMPTION", "INT_B", "PREEMPT", "Green Wave Preemption Wave 2"});
            s.push_back({40.0, "SIGNAL_PREEMPTION", "INT_C", "PREEMPT", "Green Wave Preemption Wave 3"});
            s.push_back({55.0, "AMBULANCE_UPDATE", "AMB_01", "ARRIVED", "Arrived at City General Trauma Center"});
            s.push_back({60.0, "SIGNAL_RESTORE", "INT_A", "CLEAR", "All Preemptions Safely Cleared"});
            break;

        case ScenarioType::MULTIPLE_AMBULANCES:
            s.push_back({0.0, "AMBULANCE_UPDATE", "AMB_01", "DISPATCHED", "Cardiac Emergency (Priority 95)"});
            s.push_back({5.0, "AMBULANCE_UPDATE", "AMB_02", "DISPATCHED", "Trauma Fall (Priority 80)"});
            s.push_back({15.0, "CORRIDOR_CONFLICT", "INT_B", "ARBITRATE", "AMB_01 granted priority, AMB_02 held"});
            s.push_back({30.0, "SIGNAL_PREEMPTION", "INT_B", "PREEMPT_AMB1", "Clearing for AMB_01"});
            s.push_back({45.0, "SIGNAL_PREEMPTION", "INT_B", "PREEMPT_AMB2", "Switching Corridor to AMB_02"});
            break;

        case ScenarioType::ACCIDENT:
            s.push_back({0.0, "AMBULANCE_UPDATE", "AMB_01", "DISPATCHED", "Mission Active"});
            s.push_back({8.0, "ROAD_HAZARD", "SEG_INTA_INTB", "ACCIDENT", "Multi-vehicle collision blocking 2 lanes"});
            s.push_back({15.0, "IRSA_ALERT", "SEG_INTA_INTB", "REPLAN_ADVISED", "IRSA detected blockage, advises reroute"});
            s.push_back({20.0, "CORRIDOR_REPLAN", "CORRIDOR_01", "REPLAN_APPROVED", "Coordinator approved alternate via SEG_INTA_INTC"});
            break;

        case ScenarioType::FLOODING:
            s.push_back({0.0, "WEATHER_ALERT", "CITY", "FLASH_FLOOD", "Severe Weather Incident"});
            s.push_back({5.0, "ROAD_HAZARD", "SEG_INTA_INTC", "FLOODING", "Water depth 60cm, impassable"});
            s.push_back({10.0, "ROAD_UPDATE", "SEG_INTA_INTC", "BLOCKED", "Road Suitability drops to 0.0"});
            break;

        case ScenarioType::ROAD_CLOSURE:
            s.push_back({0.0, "MUNICIPAL_EVENT", "SEG_INTB_INTC", "ROAD_CLOSED", "Emergency gas line repair"});
            s.push_back({5.0, "ROAD_HAZARD", "SEG_INTB_INTC", "ROAD_CLOSED", "Barricades deployed"});
            break;

        case ScenarioType::HEAVY_CONGESTION:
            s.push_back({0.0, "TRAFFIC_ALERT", "SEG_INTA_INTB", "GRIDLOCK", "Evening peak congestion (LOS F)"});
            s.push_back({8.0, "QUEUE_ESTIMATOR", "INT_B", "SPILLBACK", "Spillback risk detected, queue length 240m"});
            s.push_back({12.0, "SIGNAL_PREEMPTION", "INT_B", "EARLY_CLEAR", "Pre-clearing queue 25s ahead of ambulance"});
            break;

        case ScenarioType::CAMERA_FAILURE:
            s.push_back({0.0, "CAMERA_EVENT", "CAM_INTB_01", "HEARTBEAT_LOST", "Camera network silent"});
            s.push_back({10.0, "CAMERA_EVENT", "CAM_INTB_01", "DEGRADED", "Health drops to 0.30"});
            s.push_back({15.0, "CAMERA_EVENT", "CAM_INTA_01", "PTZ_PAN", "Adjacent camera pans to cover segment"});
            break;

        case ScenarioType::HOSPITAL_DIVERSION:
            s.push_back({0.0, "AMBULANCE_UPDATE", "AMB_01", "TO_HOSPITAL", "Target: HOSP_CITY_TRAUMA"});
            s.push_back({10.0, "HOSPITAL_EVENT", "HOSP_CITY_TRAUMA", "SATURATED", "ER 100% full, trauma beds 0"});
            s.push_back({15.0, "HOSPITAL_DIVERSION", "AMB_01", "DIVERTED", "Diverting to St. Jude Memorial"});
            s.push_back({20.0, "AMBULANCE_UPDATE", "AMB_01", "REROUTED", "Heading to HOSP_ST_JUDE"});
            break;

        case ScenarioType::COMMUNICATION_DELAY:
            s.push_back({0.0, "NETWORK_EVENT", "INT_B", "LATENCY_SPIKE", "RTT increases to 250ms"});
            s.push_back({5.0, "NETWORK_EVENT", "INT_B", "RETRY_DISPATCH", "ACK retry triggered automatically"});
            s.push_back({8.0, "NETWORK_EVENT", "INT_B", "ACK_CONFIRMED", "Command verified via CRC16"});
            break;

        case ScenarioType::NODE_FAILURE:
            s.push_back({0.0, "NODE_EVENT", "INT_C", "CRASH", "Simulated power failure"});
            s.push_back({3.0, "NODE_EVENT", "INT_C", "WATCHDOG_EXPIRED", "Watchdog triggered"});
            s.push_back({4.0, "SIGNAL_PREEMPTION", "INT_C", "FAILSAFE_ALL_RED", "Hardware relays fallback to ALL_RED"});
            break;

        case ScenarioType::COORDINATOR_RECOVERY:
            s.push_back({0.0, "COORDINATOR_EVENT", "COORD", "RESTART", "Warm restart initiated"});
            s.push_back({3.0, "COORDINATOR_EVENT", "COORD", "LISTENING", "TCP server re-bound to port 19123"});
            s.push_back({6.0, "NODE_EVENT", "INT_A", "RECONNECT", "INT_A reconnected via HELLO"});
            s.push_back({7.0, "NODE_EVENT", "INT_B", "RECONNECT", "INT_B reconnected via HELLO"});
            s.push_back({8.0, "NODE_EVENT", "INT_C", "RECONNECT", "INT_C reconnected via HELLO"});
            break;
    }
    return s;
}

} // namespace aegis::digital_twin

