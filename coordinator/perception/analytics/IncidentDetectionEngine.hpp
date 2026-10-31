/**
 * @file IncidentDetectionEngine.hpp
 * @brief Identifies anomalous road incidents from camera vision metadata.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/perception/RoadConditionEvent.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>

namespace aegis::coordinator::perception::analytics {

struct VisionDetectionCluster {
    std::string cameraId;
    std::string roadSegment;
    uint32_t stoppedVehicleCount{0};
    bool pedestrianOnRoadway{false};
    bool smokeOrFogDetected{false};
    bool wrongWayDetected{false};
    double stationaryDurationSec{0.0};
    double confidence{0.80};
};

class IncidentDetectionEngine {
public:
    IncidentDetectionEngine() = default;
    ~IncidentDetectionEngine() = default;

    std::optional<RoadConditionEvent> AnalyzeVisionCluster(const VisionDetectionCluster& cluster);

    void RecordIncident(const RoadConditionEvent& incident);
    std::vector<RoadConditionEvent> GetActiveIncidents(const std::string& roadSegment) const;
    std::vector<RoadConditionEvent> GetAllIncidents() const;
    bool ResolveIncident(const std::string& eventId);

    void Reset();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, RoadConditionEvent> activeIncidents_;
    uint64_t incidentCounter_{1000};
};

} // namespace aegis::coordinator::perception::analytics

