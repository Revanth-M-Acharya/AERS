/**
 * @file RoadConfidenceEngine.hpp
 * @brief Multi-camera cross-verification, temporal decay, and prediction-weighted confidence engine.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/perception/RoadConditionEvent.hpp"
#include "coordinator/perception/CameraRegistry.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace aegis::coordinator::perception {

class RoadConfidenceEngine {
public:
    explicit RoadConfidenceEngine(std::shared_ptr<CameraRegistry> registry = nullptr);
    ~RoadConfidenceEngine() = default;

    /**
     * @brief Evaluates an incoming AI road condition detection, fusing temporal & multi-camera observations.
     * @param rawEvent The incoming AI-generated observation.
     * @param predictionConfidence Confidence score from downstream traffic prediction model (0.0 to 1.0).
     * @return Fully validated and calibrated RoadConditionEvent with updated confidence and lifecycle.
     */
    RoadConditionEvent EvaluateConfidence(const RoadConditionEvent& rawEvent, double predictionConfidence = 0.90);

    /**
     * @brief Computes temporal decay for an aging observation based on elapsed time.
     */
    double ComputeTemporalDecay(uint64_t elapsedMs) const;

    /**
     * @brief Set minimum confidence required for an event to be published as valid (default 0.50).
     */
    void SetConfidenceThreshold(double threshold) { confidenceThreshold_ = threshold; }
    double GetConfidenceThreshold() const { return confidenceThreshold_; }

    void SetDecayHalfLifeSeconds(double halfLifeSec) { decayHalfLifeSec_ = halfLifeSec; }
    double GetDecayHalfLifeSeconds() const { return decayHalfLifeSec_; }

    void Reset();

private:
    std::shared_ptr<CameraRegistry> registry_;
    double confidenceThreshold_{0.50};
    double decayHalfLifeSec_{25.0}; // After 25s without detection, confidence halved
    mutable std::mutex mutex_;

    // Tracks ongoing incident observations by roadSegment + type for temporal/spatial fusion
    struct ObservationRecord {
        RoadConditionEvent latestEvent;
        std::vector<std::string> reportingCameraIds;
        uint32_t consecutiveFrameHits{1};
        uint64_t firstSeenMonoMs{0};
        uint64_t lastSeenMonoMs{0};
    };

    // key: roadSegment + "_" + RoadEventTypeToString(type)
    std::unordered_map<std::string, ObservationRecord> activeObservations_;
    uint64_t incidentSeq_{1000};
};

} // namespace aegis::coordinator::perception
