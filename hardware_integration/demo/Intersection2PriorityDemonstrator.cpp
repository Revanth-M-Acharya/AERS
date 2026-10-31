/**
 * @file Intersection2PriorityDemonstrator.cpp
 * @brief Implementation of Intersection 2 dual-ambulance priority scheduling demonstration.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "hardware_integration/demo/Intersection2PriorityDemonstrator.hpp"
#include <sstream>
#include <iomanip>

namespace aegis::hardware_integration::demo {

Intersection2PriorityDemonstrator::Intersection2PriorityDemonstrator(
    std::shared_ptr<coordinator::intelligence::PriorityScheduler> scheduler,
    std::shared_ptr<BuzzerController> buzzer,
    std::shared_ptr<OledV2XDisplay> oled)
    : scheduler_(std::move(scheduler)),
      buzzer_(std::move(buzzer)),
      oled_(std::move(oled)) {
}

PriorityArbitrationDetails Intersection2PriorityDemonstrator::DemonstratePriorityConflict(uint64_t nowMonoMs) {
    std::lock_guard<std::mutex> lock(mutex_);

    PriorityArbitrationDetails details;
    details.timestampMonoMs = nowMonoMs;

    if (scheduler_) {
        // Clear previous state for clean demonstration
        scheduler_->Clear();

        coordinator::intelligence::AmbulanceRequest reqA;
        reqA.ambulanceId = details.ambulanceAId;
        reqA.priority = details.priorityA; // CRITICAL
        reqA.startIntersection = "HOME";
        reqA.destinationIntersection = "HOSPITAL";
        reqA.requestTimeMonoMs = nowMonoMs;

        coordinator::intelligence::AmbulanceRequest reqB;
        reqB.ambulanceId = details.ambulanceBId;
        reqB.priority = details.priorityB; // HIGH
        reqB.startIntersection = "DEPOT_EAST";
        reqB.destinationIntersection = "HOSPITAL";
        reqB.requestTimeMonoMs = nowMonoMs;

        scheduler_->RegisterAmbulance(reqA);
        scheduler_->RegisterAmbulance(reqB);

        // Schedule ambulances using existing priority scheduling logic
        auto schedule = scheduler_->ScheduleAmbulances(nowMonoMs);

        if (!schedule.empty()) {
            details.selectedAmbulanceId = schedule.front().ambulanceId;
            if (details.selectedAmbulanceId == details.ambulanceAId) {
                details.selectedMissionId = details.missionAId;
                details.selectionReason = "Higher medical triage priority (Code Red Priority 1 > Code Yellow Priority 2)";
                details.estimatedWaitTimeSec = 15.0; // Clearing time for Intersection 2
            } else {
                details.selectedMissionId = details.missionBId;
                details.selectionReason = "Higher medical triage priority";
                details.estimatedWaitTimeSec = 15.0;
            }

            // Reserve intersection for winning ambulance
            scheduler_->ReserveIntersection(
                details.intersectionId,
                details.selectedAmbulanceId,
                nowMonoMs,
                nowMonoMs + static_cast<uint64_t>(details.estimatedWaitTimeSec * 1000.0));
        }
    }

    // Hardware Feedback
    if (buzzer_) {
        buzzer_->PlayPattern(BuzzerPattern::THREE_SHORT_BEEPS, 1);
    }
    if (oled_) {
        oled_->ShowPriorityArbitration(details.selectedAmbulanceId, details.ambulanceBId);
    }

    lastDetails_ = details;
    hasExecuted_ = true;
    return details;
}

std::string Intersection2PriorityDemonstrator::RenderDashboardPanel() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;

    oss << "======================================================\n";
    oss << "       INTERSECTION 2 PRIORITY SCHEDULING PANEL       \n";
    oss << "======================================================\n";
    oss << "Mission A: " << lastDetails_.missionAId << " | "
        << lastDetails_.ambulanceAId << " [Approach: " << lastDetails_.approachA << "]\n";
    oss << "Priority 1 (CRITICAL - Cardiac Arrest / Stroke)\n";
    oss << "\n";
    oss << "Mission B: " << lastDetails_.missionBId << " | "
        << lastDetails_.ambulanceBId << " [Approach: " << lastDetails_.approachB << "]\n";
    oss << "Priority 2 (HIGH - Limb Fracture / Trauma)\n";
    oss << "------------------------------------------------------\n";
    oss << "Selected Mission:       " << lastDetails_.selectedMissionId << " (" << lastDetails_.selectedAmbulanceId << ")\n";
    oss << "Arbitration Reason:     " << lastDetails_.selectionReason << "\n";
    oss << "Ambulance B Status:     HELD AT SIGNAL (RED)\n";
    oss << "Estimated Waiting Time: " << std::fixed << std::setprecision(1) << lastDetails_.estimatedWaitTimeSec << " sec\n";
    oss << "======================================================\n";

    return oss.str();
}

PriorityArbitrationDetails Intersection2PriorityDemonstrator::GetLastArbitration() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return lastDetails_;
}

} // namespace aegis::hardware_integration::demo
