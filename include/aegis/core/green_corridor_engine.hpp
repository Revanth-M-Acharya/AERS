/**
 * @file green_corridor_engine.hpp
 * @brief Dynamic green corridor spatial calculation and preemption command engine.
 */

#ifndef AEGIS_CORE_GREEN_CORRIDOR_ENGINE_HPP
#define AEGIS_CORE_GREEN_CORRIDOR_ENGINE_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <mutex>
#include "aegis/common/types.hpp"
#include "aegis/common/geo_utils.hpp"
#include "aegis/network/serializer.hpp"

namespace aegis {
namespace core {
namespace coordinator {

struct ActiveCorridor {
    std::string corridorId;
    std::string incidentId;
    std::string unitId;
    std::vector<std::string> orderedIntersections;
    size_t currentIntersectionIndex{0};
    std::unordered_set<std::string> preemptedIntersections;
    std::unordered_set<std::string> clearedIntersections;
    uint64_t startTimeMs{0};
    uint64_t lastEvaluationMs{0};
};

using PreemptionTriggerHandler = std::function<void(const std::string& intersectionId,
                                                    const network::TrafficPreemptionRequestMsg& req)>;
using PreemptionReleaseHandler = std::function<void(const std::string& intersectionId,
                                                    const std::string& unitId)>;

class GreenCorridorEngine {
public:
    GreenCorridorEngine();
    ~GreenCorridorEngine() = default;

    void SetTriggerHandler(PreemptionTriggerHandler handler);
    void SetReleaseHandler(PreemptionReleaseHandler handler);

    bool RegisterCorridor(const std::string& corridorId,
                          const std::string& incidentId,
                          const std::string& unitId,
                          const std::vector<std::string>& intersections);

    void EvaluateVehicleProgress(const std::string& unitId,
                                 const common::GeoCoordinate& currentLocation,
                                 float speedKmH,
                                 float bearingDeg);

    void TriggerPreemption(const std::string& intersectionId,
                           const std::string& unitId,
                           uint16_t arrivalSec = 20);

    void ReleaseCorridor(const std::string& intersectionId);
    void ClearCorridor(const std::string& corridorId);

    bool IsIntersectionPreempted(const std::string& intersectionId) const;
    std::vector<std::string> GetActiveCorridorIds() const;
    size_t GetActiveCorridorCount() const noexcept;

private:
    double m_triggerDistanceMeters{600.0};
    uint16_t m_holdDurationSec{30};

    PreemptionTriggerHandler m_triggerHandler;
    PreemptionReleaseHandler m_releaseHandler;

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, ActiveCorridor> m_corridors; // Keyed by unitId
    std::unordered_set<std::string> m_preemptedIntersections;
};

// Alias for architecture compliance
using DynamicGreenCorridor = GreenCorridorEngine;

} // namespace coordinator
} // namespace core
} // namespace aegis

#endif // AEGIS_CORE_GREEN_CORRIDOR_ENGINE_HPP
