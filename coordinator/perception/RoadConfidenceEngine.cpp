/**
 * @file RoadConfidenceEngine.cpp
 * @brief Implementation of RoadConfidenceEngine multi-sensor fusion, temporal decay, and prediction weighting.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/perception/RoadConfidenceEngine.hpp"
#include "common/utils/TimeUtils.hpp"
#include <algorithm>
#include <cmath>

namespace aegis::coordinator::perception {

RoadConfidenceEngine::RoadConfidenceEngine(std::shared_ptr<CameraRegistry> registry)
    : registry_(std::move(registry)) {}

double RoadConfidenceEngine::ComputeTemporalDecay(uint64_t elapsedMs) const {
    if (elapsedMs == 0) return 1.0;
    double elapsedSec = static_cast<double>(elapsedMs) / 1000.0;
    // Exponential decay: e^(-ln(2) * t / t_half)
    double lambda = 0.69314718 / (decayHalfLifeSec_ > 0.0 ? decayHalfLifeSec_ : 25.0);
    return std::exp(-lambda * elapsedSec);
}

RoadConditionEvent RoadConfidenceEngine::EvaluateConfidence(const RoadConditionEvent& rawEvent,
                                                           double predictionConfidence) {
    uint64_t now = rawEvent.timestampMonoMs;
    if (now == 0) now = common::TimeUtils::GetMonotonicTimeMs();

    RoadConditionEvent validated = rawEvent;
    validated.timestampMonoMs = now;
    if (validated.creationTimeMonoMs == 0) validated.creationTimeMonoMs = now;
    validated.lastUpdatedMonoMs = now;

    // 1. Camera health weight calibration
    double cameraHealth = 1.0;
    if (registry_ && !rawEvent.cameraId.empty()) {
        auto cam = registry_->GetCamera(rawEvent.cameraId);
        if (cam.has_value()) {
            cameraHealth = cam->healthScore;
        }
    }

    // Blend AI confidence (70%), camera health (15%), and prediction confidence (15%)
    predictionConfidence = std::clamp(predictionConfidence, 0.0, 1.0);
    double baseConf = (0.70 * rawEvent.confidence + 0.15 * cameraHealth + 0.15 * predictionConfidence);
    baseConf = std::clamp(baseConf, 0.0, 1.0);

    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = rawEvent.roadSegment + "_" + RoadEventTypeToString(rawEvent.type);

    auto it = activeObservations_.find(key);
    if (it != activeObservations_.end()) {
        auto& obs = it->second;
        uint64_t elapsedSinceLast = (now >= obs.lastSeenMonoMs) ? (now - obs.lastSeenMonoMs) : 0;

        // If last seen within temporal correlation window (30 seconds)
        if (elapsedSinceLast <= 30000) {
            obs.consecutiveFrameHits++;
            obs.lastSeenMonoMs = now;

            // Track distinct reporting cameras
            if (std::find(obs.reportingCameraIds.begin(), obs.reportingCameraIds.end(), rawEvent.cameraId)
                == obs.reportingCameraIds.end() && !rawEvent.cameraId.empty()) {
                obs.reportingCameraIds.push_back(rawEvent.cameraId);
            }

            // Multi-camera spatial fusion boost:
            // C_fused = 1 - (1 - C_prev) * (1 - C_new)
            double fusedConf = 1.0 - (1.0 - obs.latestEvent.confidence) * (1.0 - baseConf);

            // Temporal persistence bonus (+5% per consecutive frame, max +20%)
            double persistenceBonus = std::min(0.20, (obs.consecutiveFrameHits - 1) * 0.05);
            fusedConf = std::clamp(fusedConf + persistenceBonus, 0.0, 1.0);

            // Apply minor decay if there was a gap between frames
            double decay = ComputeTemporalDecay(elapsedSinceLast);
            fusedConf = std::clamp(fusedConf * (0.85 + 0.15 * decay), 0.0, 1.0);

            obs.latestEvent = validated;
            obs.latestEvent.confidence = fusedConf;
            obs.latestEvent.confirmingCameraCount = static_cast<uint32_t>(obs.reportingCameraIds.size());
            obs.latestEvent.consecutiveFrameDetections = obs.consecutiveFrameHits;
            obs.latestEvent.lastUpdatedMonoMs = now;

            // Preserve incidentId
            if (obs.latestEvent.incidentId.empty()) {
                obs.latestEvent.incidentId = obs.latestEvent.eventId.empty()
                    ? ("INC-" + std::to_string(++incidentSeq_))
                    : obs.latestEvent.eventId;
            }

            // Record history
            obs.latestEvent.RecordConfidenceUpdate(fusedConf, now);
            obs.latestEvent.RecordSeverityUpdate(validated.severity, now);

            // Lifecycle progression
            if (obs.reportingCameraIds.size() >= 2) {
                obs.latestEvent.verificationStatus = VerificationStatus::MULTI_CAMERA_VERIFIED;
                obs.latestEvent.lifecycleState = IncidentLifecycleState::ACTIVE;
            } else if (obs.consecutiveFrameHits >= 3 && fusedConf >= confidenceThreshold_) {
                obs.latestEvent.verificationStatus = VerificationStatus::CONFIRMED;
                obs.latestEvent.lifecycleState = IncidentLifecycleState::ACTIVE;
            } else if (fusedConf >= confidenceThreshold_) {
                obs.latestEvent.verificationStatus = VerificationStatus::CONFIRMED;
                obs.latestEvent.lifecycleState = IncidentLifecycleState::CONFIRMED;
            } else {
                obs.latestEvent.verificationStatus = VerificationStatus::UNVERIFIED;
                obs.latestEvent.lifecycleState = IncidentLifecycleState::DETECTED;
            }

            return obs.latestEvent;
        } else {
            // Expired observation
            activeObservations_.erase(it);
        }
    }

    // New observation
    ObservationRecord obs;
    obs.latestEvent = validated;
    if (obs.latestEvent.incidentId.empty()) {
        obs.latestEvent.incidentId = obs.latestEvent.eventId.empty()
            ? ("INC-" + std::to_string(++incidentSeq_))
            : obs.latestEvent.eventId;
    }
    obs.latestEvent.confidence = baseConf;
    obs.latestEvent.confirmingCameraCount = 1;
    obs.latestEvent.consecutiveFrameDetections = 1;
    obs.firstSeenMonoMs = now;
    obs.lastSeenMonoMs = now;
    obs.latestEvent.creationTimeMonoMs = now;
    obs.latestEvent.lastUpdatedMonoMs = now;
    obs.latestEvent.RecordConfidenceUpdate(baseConf, now);
    obs.latestEvent.RecordSeverityUpdate(validated.severity, now);

    if (!rawEvent.cameraId.empty()) {
        obs.reportingCameraIds.push_back(rawEvent.cameraId);
    }

    if (baseConf < 0.35) {
        obs.latestEvent.verificationStatus = VerificationStatus::FALSE_POSITIVE;
        obs.latestEvent.lifecycleState = IncidentLifecycleState::DETECTED;
    } else if (baseConf >= 0.85) {
        obs.latestEvent.verificationStatus = VerificationStatus::CONFIRMED;
        obs.latestEvent.lifecycleState = IncidentLifecycleState::CONFIRMED;
    } else {
        obs.latestEvent.verificationStatus = VerificationStatus::UNVERIFIED;
        obs.latestEvent.lifecycleState = IncidentLifecycleState::DETECTED;
    }

    activeObservations_[key] = obs;
    return obs.latestEvent;
}

void RoadConfidenceEngine::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    activeObservations_.clear();
}

} // namespace aegis::coordinator::perception
