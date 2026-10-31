/**
 * @file IETAEngine.hpp
 * @brief Abstract interface for continuous ETA estimation and confidence scoring.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/intelligence/graph/RoadGraph.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace aegis::coordinator::intelligence {

struct ETAReport {
    double etaSeconds{0.0};
    double remainingDistanceMeters{0.0};
    double arrivalConfidencePercent{100.0};
    uint64_t predictedArrivalTimeUnixMs{0};
    double currentAverageSpeedKmh{0.0};
};

class IETAEngine {
public:
    virtual ~IETAEngine() = default;

    /**
     * @brief Estimate real-time ETA, remaining distance, and confidence.
     */
    virtual ETAReport EstimateETA(const std::string& ambulanceId,
                                  const std::vector<std::string>& remainingSegmentIds,
                                  double currentSpeedKmh,
                                  const RoadGraph& graph) = 0;

    /**
     * @brief Incorporate completed edge traversal time into historical statistics.
     */
    virtual void UpdateHistoricalEdgeTraversal(const std::string& segmentId, double observedTimeSec) = 0;
};

} // namespace aegis::coordinator::intelligence

