/**
 * @file PerceptionDigitalTwinBridge.cpp
 * @brief Implementation of PerceptionDigitalTwinBridge visualization snapshot engine.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/perception/digital_twin/PerceptionDigitalTwinBridge.hpp"
#include "common/utils/TimeUtils.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace aegis::coordinator::perception::digital_twin {

PerceptionDigitalTwinBridge::PerceptionDigitalTwinBridge(
    std::shared_ptr<RoadSituationAwarenessEngine> irsaEngine,
    std::shared_ptr<CoordinatorIntegration> coordinatorIntegration)
    : irsaEngine_(std::move(irsaEngine)),
      coordinatorIntegration_(std::move(coordinatorIntegration)) {}

void PerceptionDigitalTwinBridge::SetActiveAmbulanceContext(
    const std::string& emergencyId,
    const std::vector<std::string>& route,
    double etaSeconds) {

    std::lock_guard<std::mutex> lock(mutex_);
    activeEmergencyId_ = emergencyId;
    activeRoute_ = route;
    activeEtaSeconds_ = etaSeconds;
}

void PerceptionDigitalTwinBridge::UpdateSignalState(const std::string& intersectionId, const std::string& state) {
    std::lock_guard<std::mutex> lock(mutex_);
    signalStates_[intersectionId] = state;
}

void PerceptionDigitalTwinBridge::RecordTimelineEvent(const IncidentTimelineEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    incidentTimeline_.push_back(entry);
    // Keep up to 100 timeline entries
    if (incidentTimeline_.size() > 100) {
        incidentTimeline_.erase(incidentTimeline_.begin());
    }
}

std::vector<IncidentTimelineEntry> PerceptionDigitalTwinBridge::GetIncidentTimeline() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return incidentTimeline_;
}

void PerceptionDigitalTwinBridge::RecordSnapshotForReplay(const DigitalTwinPerceptionSnapshot& snapshot) {
    std::lock_guard<std::mutex> lock(mutex_);
    replayHistory_.push_back(snapshot);
    if (replayHistory_.size() > 50) {
        replayHistory_.erase(replayHistory_.begin());
    }
}

std::vector<DigitalTwinPerceptionSnapshot> PerceptionDigitalTwinBridge::GetReplayHistory(size_t maxCount) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (replayHistory_.size() <= maxCount) {
        return replayHistory_;
    }
    return std::vector<DigitalTwinPerceptionSnapshot>(replayHistory_.end() - static_cast<long>(maxCount), replayHistory_.end());
}

DigitalTwinPerceptionSnapshot PerceptionDigitalTwinBridge::GenerateSnapshot() const {
    DigitalTwinPerceptionSnapshot snap;
    snap.timestampMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        snap.activeEmergencyId = activeEmergencyId_;
        snap.updatedAmbulanceRoute = activeRoute_;
        snap.updatedEtaSeconds = activeEtaSeconds_;
        snap.signalStates = signalStates_;
        snap.incidentTimeline = incidentTimeline_;
    }

    if (irsaEngine_) {
        // 1. Camera Locations & Health Overlay
        auto camReg = irsaEngine_->GetCameraRegistry();
        auto streamMgr = irsaEngine_->GetVideoStreamManager();

        if (camReg) {
            auto allCams = camReg->GetAllCameras();
            for (const auto& c : allCams) {
                CameraMarker m;
                m.cameraId = c.cameraId;
                m.latitude = c.latitude;
                m.longitude = c.longitude;
                m.status = CameraStatusToString(c.status);
                m.intersectionId = c.intersectionId;
                m.coveredSegments = c.coveredSegments;
                m.healthScore = c.healthScore;

                if (streamMgr) {
                    auto sOpt = streamMgr->GetStreamStats(c.cameraId);
                    if (sOpt.has_value()) {
                        m.currentFps = sOpt->currentFps;
                        m.droppedFrames = sOpt->droppedFrames;
                        m.streamStatus = (sOpt->droppedFrames > 10) ? "DEGRADED" : "HEALTHY";
                    }
                }
                snap.cameraLocations.push_back(m);
            }
        }

        // 2. Active Corridor Cameras
        auto feedMgr = irsaEngine_->GetCameraFeedManager();
        if (feedMgr) {
            snap.activeCorridorCameras = feedMgr->GetActiveCameraIds();
        }

        // 3. Simulated Live Feeds
        if (streamMgr) {
            auto allStats = streamMgr->GetAllStreamStats();
            for (const auto& s : allStats) {
                SimulatedFeedState f;
                f.cameraId = s.cameraId;
                f.fps = s.currentFps;
                f.bitrateKbps = s.bitrateKbps;
                f.totalFrames = s.totalFramesReceived;
                f.droppedFrames = s.droppedFrames;
                f.motionDetected = (s.totalFramesReceived > 0);
                snap.simulatedLiveFeeds.push_back(f);
            }
        }

        // 4. Incident Overlays, Aging, and Confidence Breakdown
        auto events = irsaEngine_->GetAllActiveEvents();
        auto confEngine = irsaEngine_->GetConfidenceEngine();

        for (const auto& ev : events) {
            IncidentOverlay o;
            o.eventId = ev.eventId;
            o.incidentId = ev.incidentId.empty() ? ev.eventId : ev.incidentId;
            o.type = RoadEventTypeToString(ev.type);
            o.roadSegment = ev.roadSegment;
            o.latitude = ev.latitude;
            o.longitude = ev.longitude;
            o.confidence = ev.confidence;
            o.severity = EventSeverityToString(ev.severity);
            o.status = VerificationStatusToString(ev.verificationStatus);
            o.lifecycleState = IncidentLifecycleStateToString(ev.lifecycleState);
            o.isBlocking = ev.isBlocking;
            o.description = ev.description;

            // Aging calculation
            o.creationTimeMonoMs = ev.creationTimeMonoMs;
            o.lastUpdatedMonoMs = ev.lastUpdatedMonoMs;
            o.durationSeconds = static_cast<double>(ev.GetDurationMs()) / 1000.0;
            if (o.durationSeconds < 60.0) {
                o.agingStatus = "FRESH";
            } else if (o.durationSeconds < 300.0) {
                o.agingStatus = "AGING";
            } else {
                o.agingStatus = "STALE";
            }

            // Confidence breakdown
            o.aiConfidence = ev.confidence;
            if (confEngine) {
                uint64_t elapsed = (snap.timestampMonoMs > ev.lastUpdatedMonoMs) ? (snap.timestampMonoMs - ev.lastUpdatedMonoMs) : 0;
                o.temporalDecayFactor = confEngine->ComputeTemporalDecay(elapsed);
            } else {
                o.temporalDecayFactor = 1.0;
            }
            o.cameraHealthFactor = 0.95;
            o.predictionConfidence = 0.90;

            snap.incidentOverlays.push_back(o);
        }

        // 5. Road Status Colors & Suitability Score Overlay
        if (!snap.updatedAmbulanceRoute.empty()) {
            for (const auto& seg : snap.updatedAmbulanceRoute) {
                auto state = irsaEngine_->GetSegmentState(seg);
                RoadColorState r;
                r.segmentId = seg;
                r.color = RoadStatusColorToString(state.statusColor);
                r.densityScore = state.densityScore;
                r.delaySeconds = state.estimatedTransitDelaySec;
                r.recommendedEmergencyLane = state.recommendedEmergencyLane;
                r.suitabilityScore = state.suitabilityScore;
                r.suitabilityRating = state.suitabilityRating;
                r.isRecommendedForCorridor = state.isRecommendedForCorridor;
                r.penaltyReasons = state.suitabilityPenaltyReasons;
                snap.roadStatusColours.push_back(r);
            }
        }
    }

    if (coordinatorIntegration_) {
        auto pending = coordinatorIntegration_->GetPendingRecommendations();
        snap.corridorReplanningAdvised = !pending.empty();
    }

    return snap;
}

std::string DigitalTwinPerceptionSnapshot::ToJson() const {
    std::ostringstream ss;
    ss << "{"
       << "\"timestampMonoMs\":" << timestampMonoMs << ","
       << "\"activeEmergencyId\":\"" << activeEmergencyId << "\","
       << "\"updatedEtaSeconds\":" << std::fixed << std::setprecision(1) << updatedEtaSeconds << ","
       << "\"corridorReplanningAdvised\":" << (corridorReplanningAdvised ? "true" : "false") << ","
       << "\"cameraLocations\":[";
    for (size_t i = 0; i < cameraLocations.size(); ++i) {
        const auto& c = cameraLocations[i];
        ss << "{\"id\":\"" << c.cameraId << "\",\"lat\":" << std::fixed << std::setprecision(6) << c.latitude
           << ",\"lon\":" << std::fixed << std::setprecision(6) << c.longitude
           << ",\"status\":\"" << c.status << "\",\"health\":" << std::fixed << std::setprecision(2) << c.healthScore
           << ",\"fps\":" << std::fixed << std::setprecision(1) << c.currentFps << "}"
           << (i + 1 < cameraLocations.size() ? "," : "");
    }
    ss << "],\"activeCorridorCameras\":[";
    for (size_t i = 0; i < activeCorridorCameras.size(); ++i) {
        ss << "\"" << activeCorridorCameras[i] << "\"" << (i + 1 < activeCorridorCameras.size() ? "," : "");
    }
    ss << "],\"incidentOverlays\":[";
    for (size_t i = 0; i < incidentOverlays.size(); ++i) {
        const auto& o = incidentOverlays[i];
        ss << "{\"id\":\"" << o.eventId << "\",\"incidentId\":\"" << o.incidentId << "\",\"type\":\"" << o.type
           << "\",\"seg\":\"" << o.roadSegment << "\",\"conf\":" << std::fixed << std::setprecision(2) << o.confidence
           << ",\"sev\":\"" << o.severity << "\",\"lifecycle\":\"" << o.lifecycleState
           << "\",\"durationSec\":" << std::fixed << std::setprecision(1) << o.durationSeconds
           << ",\"aging\":\"" << o.agingStatus << "\",\"blocking\":" << (o.isBlocking ? "true" : "false") << "}"
           << (i + 1 < incidentOverlays.size() ? "," : "");
    }
    ss << "],\"roadStatusColours\":[";
    for (size_t i = 0; i < roadStatusColours.size(); ++i) {
        const auto& r = roadStatusColours[i];
        ss << "{\"seg\":\"" << r.segmentId << "\",\"color\":\"" << r.color
           << "\",\"density\":" << std::fixed << std::setprecision(2) << r.densityScore
           << ",\"delay\":" << r.delaySeconds << ",\"suitability\":" << std::fixed << std::setprecision(1) << r.suitabilityScore
           << ",\"rating\":\"" << r.suitabilityRating << "\"}"
           << (i + 1 < roadStatusColours.size() ? "," : "");
    }
    ss << "],\"incidentTimeline\":[";
    for (size_t i = 0; i < incidentTimeline.size(); ++i) {
        const auto& t = incidentTimeline[i];
        ss << "{\"incidentId\":\"" << t.incidentId << "\",\"type\":\"" << t.type << "\",\"state\":\"" << t.lifecycleState << "\"}"
           << (i + 1 < incidentTimeline.size() ? "," : "");
    }
    ss << "]}";
    return ss.str();
}

std::string DigitalTwinPerceptionSnapshot::RenderAsciiDashboard() const {
    std::ostringstream ss;
    ss << "================================================================================\n"
       << " SMART AEGIS: IRSA Digital Twin Perception Dashboard\n"
       << "================================================================================\n"
       << " Active Emergency: " << (activeEmergencyId.empty() ? "NONE" : activeEmergencyId)
       << " | Corridor ETA: " << updatedEtaSeconds << "s"
       << " | Replan Advised: " << (corridorReplanningAdvised ? "YES [ALERT]" : "NO [NOMINAL]") << "\n"
       << " Cameras: Total=" << cameraLocations.size()
       << ", Active Corridor Feeds=" << activeCorridorCameras.size() << "\n"
       << "--------------------------------------------------------------------------------\n"
       << " ROAD CORRIDOR STATUS & SEGMENT COLORS:\n";

    for (const auto& r : roadStatusColours) {
        ss << "   - [" << r.color << "] " << r.segmentId
           << " | Density: " << static_cast<int>(r.densityScore * 100) << "%"
           << " | Delay: " << r.delaySeconds << "s"
           << " | Suitability: " << std::fixed << std::setprecision(1) << r.suitabilityScore
           << "/100 (" << r.suitabilityRating << ")"
           << " | Recommended: Lane " << r.recommendedEmergencyLane << "\n";
    }

    if (!incidentOverlays.empty()) {
        ss << "--------------------------------------------------------------------------------\n"
           << " AI DETECTIONS & INCIDENT OVERLAYS (AGING & CONFIDENCE):\n";
        for (const auto& inc : incidentOverlays) {
            ss << "   * " << inc.eventId << " (Inc: " << inc.incidentId << ") [" << inc.type << "] on " << inc.roadSegment
               << " (Conf: " << static_cast<int>(inc.confidence * 100) << "%"
               << ", Sev: " << inc.severity
               << ", Lifecycle: " << inc.lifecycleState
               << ", Age: " << std::fixed << std::setprecision(1) << inc.durationSeconds << "s [" << inc.agingStatus << "]"
               << (inc.isBlocking ? ", BLOCKING" : "") << ")\n";
        }
    }

    if (!signalStates.empty()) {
        ss << "--------------------------------------------------------------------------------\n"
           << " SIGNAL STATES:\n";
        for (const auto& [intId, state] : signalStates) {
            ss << "   * " << intId << ": " << state << "\n";
        }
    }

    if (!incidentTimeline.empty()) {
        ss << "--------------------------------------------------------------------------------\n"
           << " INCIDENT TIMELINE:\n";
        for (const auto& t : incidentTimeline) {
            ss << "   > [" << t.timestampMonoMs << "ms] " << t.incidentId << " -> " << t.lifecycleState
               << " (" << t.type << " on " << t.roadSegment << ")\n";
        }
    }

    ss << "================================================================================\n";
    return ss.str();
}

void PerceptionDigitalTwinBridge::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    activeEmergencyId_.clear();
    activeRoute_.clear();
    activeEtaSeconds_ = 0.0;
    signalStates_.clear();
    incidentTimeline_.clear();
    replayHistory_.clear();
}

} // namespace aegis::coordinator::perception::digital_twin
