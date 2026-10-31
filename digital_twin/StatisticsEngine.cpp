/**
 * @file StatisticsEngine.cpp
 * @brief Implementation of StatisticsEngine.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "digital_twin/StatisticsEngine.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace aegis::digital_twin {

StatisticsEngine::StatisticsEngine() {
    Reset();
}

void StatisticsEngine::RecordMissionStarted(const std::string& /*missionId*/) {
    std::lock_guard<std::mutex> lock(mutex_);
    missionsDispatched_++;
}

void StatisticsEngine::RecordMissionCompleted(const std::string& /*missionId*/, double etaReductionSec, double handoverSec) {
    std::lock_guard<std::mutex> lock(mutex_);
    missionsCompleted_++;
    totalEtaReductionSec_ += std::max(0.0, etaReductionSec);
    totalHandoverSec_ += std::max(0.0, handoverSec);
}

void StatisticsEngine::RecordRouteReplan(const std::string& /*missionId*/, const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    totalReplans_++;
    replanReasons_[reason]++;
}

void StatisticsEngine::RecordSignalSync(const std::string& /*intersectionId*/, double clearanceSec) {
    std::lock_guard<std::mutex> lock(mutex_);
    totalSignalSyncs_++;
    totalClearanceSec_ += std::max(0.0, clearanceSec);
}

void StatisticsEngine::RecordIncidentClearance(const std::string& /*incidentId*/, double clearanceDurationSec) {
    std::lock_guard<std::mutex> lock(mutex_);
    totalIncidents_++;
    totalIncidentClearanceSec_ += std::max(0.0, clearanceDurationSec);
}

MissionStatisticsReport StatisticsEngine::GenerateReport() const {
    std::lock_guard<std::mutex> lock(mutex_);
    MissionStatisticsReport r;
    r.totalMissionsDispatched = missionsDispatched_;
    r.totalMissionsCompleted = missionsCompleted_;
    r.missionSuccessRatePercent = (missionsDispatched_ > 0) ? (static_cast<double>(missionsCompleted_) * 100.0 / missionsDispatched_) : 100.0;
    r.averageEtaReductionSeconds = (missionsCompleted_ > 0) ? (totalEtaReductionSec_ / missionsCompleted_) : 0.0;
    r.totalTimeSavedMinutes = totalEtaReductionSec_ / 60.0;

    r.totalRouteReplans = totalReplans_;
    r.replanReasons = replanReasons_;

    r.totalSignalSyncs = totalSignalSyncs_;
    r.averageCorridorClearanceSeconds = (totalSignalSyncs_ > 0) ? (totalClearanceSec_ / totalSignalSyncs_) : 0.0;
    r.averageHospitalHandoverSeconds = (missionsCompleted_ > 0) ? (totalHandoverSec_ / missionsCompleted_) : 0.0;

    r.totalIncidentsDetected = totalIncidents_;
    r.averageIncidentDetectionToClearanceSeconds = (totalIncidents_ > 0) ? (totalIncidentClearanceSec_ / totalIncidents_) : 0.0;

    return r;
}

void StatisticsEngine::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    missionsDispatched_ = 0;
    missionsCompleted_ = 0;
    totalEtaReductionSec_ = 0.0;
    totalHandoverSec_ = 0.0;
    totalReplans_ = 0;
    replanReasons_.clear();
    totalSignalSyncs_ = 0;
    totalClearanceSec_ = 0.0;
    totalIncidents_ = 0;
    totalIncidentClearanceSec_ = 0.0;
}

std::string MissionStatisticsReport::ToJson() const {
    std::ostringstream ss;
    ss << "{\"dispatched\":" << totalMissionsDispatched
       << ",\"completed\":" << totalMissionsCompleted
       << ",\"successRate\":" << std::fixed << std::setprecision(1) << missionSuccessRatePercent
       << ",\"avgEtaReductionSec\":" << std::setprecision(1) << averageEtaReductionSeconds
       << ",\"totalTimeSavedMin\":" << std::setprecision(1) << totalTimeSavedMinutes
       << ",\"replans\":" << totalRouteReplans
       << ",\"signalSyncs\":" << totalSignalSyncs
       << ",\"avgClearanceSec\":" << std::setprecision(1) << averageCorridorClearanceSeconds
       << ",\"avgHandoverSec\":" << std::setprecision(1) << averageHospitalHandoverSeconds
       << ",\"incidents\":" << totalIncidentsDetected
       << "}";
    return ss.str();
}

std::string MissionStatisticsReport::ToSummaryString() const {
    std::ostringstream ss;
    ss << "=== MISSION & OPERATIONS ANALYTICS SUMMARY =====================================\n"
       << " Missions Dispatched  : " << totalMissionsDispatched << "\n"
       << " Missions Completed   : " << totalMissionsCompleted << " ("
       << std::fixed << std::setprecision(1) << missionSuccessRatePercent << "% Success Rate)\n"
       << " Avg ETA Reduction    : " << std::setprecision(1) << averageEtaReductionSeconds << " seconds per emergency\n"
       << " Total Time Saved     : " << std::setprecision(1) << totalTimeSavedMinutes << " minutes across missions\n"
       << " Route Replans        : " << totalRouteReplans << "\n"
       << " Signal Syncs         : " << totalSignalSyncs << " (Avg Clearance: "
       << std::setprecision(1) << averageCorridorClearanceSeconds << "s)\n"
       << " Hospital Handovers   : " << totalMissionsCompleted << " (Avg Handover Time: "
       << std::setprecision(1) << averageHospitalHandoverSeconds << "s)\n"
       << " Incidents Cleared    : " << totalIncidentsDetected << " (Avg Clearance: "
       << std::setprecision(1) << averageIncidentDetectionToClearanceSeconds << "s)\n"
       << "================================================================================\n";
    return ss.str();
}

} // namespace aegis::digital_twin

