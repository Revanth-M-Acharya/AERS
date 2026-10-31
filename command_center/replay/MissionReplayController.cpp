/**
 * @file MissionReplayController.cpp
 * @brief Implementation of Mission Replay Controller.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "command_center/replay/MissionReplayController.hpp"
#include <sstream>
#include <iomanip>

namespace aegis::command_center {

MissionReplayController::MissionReplayController() {
    HistoricalMissionRecord defaultMission;
    defaultMission.missionId = "MSN-2026-007";
    defaultMission.emergencyType = "CARDIAC_ARREST";
    defaultMission.priority = "CRITICAL (Code Red)";
    defaultMission.assignedAmbulance = "AMB-01";
    defaultMission.assignedHospital = "City Trauma Hospital";
    defaultMission.travelTimeSeconds = 138.0;
    defaultMission.routeUsed = {"HOME", "INT_1", "INT_BYPASS", "INT_3", "HOSPITAL"};
    defaultMission.wasReplanned = true;
    defaultMission.manualPoliceAdvisory = true;
    defaultMission.replanReason = "TEMPORARY_POLICE_BARRICADE_AT_INT_2";
    defaultMission.signalsPreempted = {"INT_1", "INT_2", "INT_3", "INT_BYPASS"};
    defaultMission.outcome = "SUCCESSFUL_PATIENT_HANDOVER";

    LoadMission(defaultMission);
}

void MissionReplayController::ConstructTimelineFromMission(const HistoricalMissionRecord& m) {
    timeline_.clear();
    currentStep_ = 0;

    auto add = [this](uint32_t step, const std::string& stage, double offsetSec,
                      const std::string& desc, const std::string& amb,
                      const std::string& intId, const std::string& sigs,
                      const std::string& adv) {
        ReplayTimelineEvent ev;
        ev.stepIndex = step;
        ev.stageName = stage;
        ev.timeOffsetSeconds = offsetSec;
        ev.description = desc;
        ev.activeAmbulance = amb;
        ev.activeIntersection = intId;
        ev.signalsState = sigs;
        ev.advisoryMessage = adv;
        timeline_.push_back(ev);
    };

    // 1. Dispatch
    add(0, "DISPATCH", 0.0,
        "Emergency call received: " + m.emergencyType + " reported near HOME station.",
        m.assignedAmbulance, "HOME", "NORMAL_CYCLING", "MISSION_DISPATCHED");

    // 2. Ambulance Assignment
    add(1, "AMBULANCE_ASSIGNMENT", 2.5,
        "Ambulance " + m.assignedAmbulance + " assigned with priority " + m.priority + "; hospital: " + m.assignedHospital,
        m.assignedAmbulance, "HOME", "NORMAL_CYCLING", "AMBULANCE_EN_ROUTE");

    // 3. Route
    add(2, "ROUTE", 4.0,
        "Dijkstra computed shortest path: HOME -> INT_1 -> INT_2 -> INT_3 -> HOSPITAL (1450m).",
        m.assignedAmbulance, "INT_1", "NORMAL_CYCLING", "ROUTE_COMPUTED");

    // 4. Green Corridor
    add(3, "GREEN_CORRIDOR", 10.0,
        "Time-staged green wave preemption activated; Continuous tone buzzer sounded; OLED active.",
        m.assignedAmbulance, "INT_1", "INT_1:GREEN, INT_2:GREEN, INT_3:GREEN", "CORRIDOR_ACTIVE");

    // 5. Priority Scheduling
    add(4, "PRIORITY_SCHEDULING", 32.0,
        "Simultaneous arrival at Intersection 2: " + m.assignedAmbulance + " (Code Red) passed; AMB-02 (Code Yellow) held 15s.",
        m.assignedAmbulance, "INT_2", "INT_2:ROAD_A_GREEN, ROAD_B_RED", "PRIORITY_ARBITRATED");

    // 6. Police Advisory
    add(5, "POLICE_ADVISORY", 54.0,
        "Police switch pressed: Traffic police report ROAD CLOSED / BARRICADE at INT_2; Coordinator maintains 100% authority.",
        m.assignedAmbulance, "INT_1", "INT_2:NORMAL_CYCLE", "POLICE_BARRICADE_REPORTED");

    // 7. Dynamic Replanning
    add(6, "DYNAMIC_REPLANNING", 54.5,
        "Zero-pause autonomous rerouting via INT_BYPASS; new ETA 42s generated; bypass preemption active.",
        m.assignedAmbulance, "INT_BYPASS", "INT_BYPASS:GREEN, INT_3:GREEN", "ROUTE_REPLANNED_BYPASS");

    // 8. Hospital Arrival
    add(7, "HOSPITAL_ARRIVAL", 112.0,
        "Ambulance arrived at " + m.assignedHospital + " emergency reception bay; bed reserved.",
        m.assignedAmbulance, "HOSPITAL", "SIGNALS_CLEARING", "AMBULANCE_ARRIVED");

    // 9. Mission Complete
    add(8, "MISSION_COMPLETE", 138.0,
        "Corridor released; all signals returned to normal cycle; 2 long beeps; " + m.outcome,
        m.assignedAmbulance, "HOSPITAL", "ALL_INTERSECTIONS_NORMAL", "MISSION_COMPLETED");
}

bool MissionReplayController::LoadMission(const HistoricalMissionRecord& mission) {
    std::lock_guard<std::mutex> lock(mutex_);
    currentMission_ = mission;
    ConstructTimelineFromMission(mission);
    return !timeline_.empty();
}

bool MissionReplayController::StepForward() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (timeline_.empty() || currentStep_ + 1 >= timeline_.size()) {
        return false;
    }
    currentStep_++;
    return true;
}

bool MissionReplayController::StepBackward() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (currentStep_ == 0 || timeline_.empty()) {
        return false;
    }
    currentStep_--;
    return true;
}

void MissionReplayController::Rewind() {
    std::lock_guard<std::mutex> lock(mutex_);
    currentStep_ = 0;
}

bool MissionReplayController::SeekToStep(uint32_t stepIndex) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stepIndex >= timeline_.size()) {
        return false;
    }
    currentStep_ = stepIndex;
    return true;
}

ReplayTimelineEvent MissionReplayController::GetCurrentEvent() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (timeline_.empty()) {
        return ReplayTimelineEvent();
    }
    return timeline_[currentStep_];
}

std::vector<ReplayTimelineEvent> MissionReplayController::GetAllTimelineEvents() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return timeline_;
}

size_t MissionReplayController::GetTotalSteps() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return timeline_.size();
}

uint32_t MissionReplayController::GetCurrentStepIndex() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return currentStep_;
}

bool MissionReplayController::IsAtEnd() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !timeline_.empty() && (currentStep_ + 1 >= timeline_.size());
}

std::string MissionReplayController::RenderCurrentStageAscii() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (timeline_.empty()) {
        return "[MISSION REPLAY] No mission loaded.\n";
    }

    const auto& ev = timeline_[currentStep_];
    std::ostringstream oss;

    oss << "====================================================\n";
    oss << "        SMART AEGIS MISSION TIMELINE REPLAY        \n";
    oss << "====================================================\n";
    oss << "Mission ID:     " << currentMission_.missionId << " (" << currentMission_.emergencyType << ")\n";
    oss << "Replay Step:    [" << (ev.stepIndex + 1) << "/" << timeline_.size() << "] " << ev.stageName << "\n";
    oss << "Timeline Time:  +" << std::fixed << std::setprecision(1) << ev.timeOffsetSeconds << " sec\n";
    oss << "Active Unit:    " << ev.activeAmbulance << " @ " << ev.activeIntersection << "\n";
    oss << "Signals:        " << ev.signalsState << "\n";
    oss << "Advisory:       " << ev.advisoryMessage << "\n";
    oss << "----------------------------------------------------\n";
    oss << "Stage Detail:   " << ev.description << "\n";
    oss << "====================================================\n";

    return oss.str();
}

} // namespace aegis::command_center
