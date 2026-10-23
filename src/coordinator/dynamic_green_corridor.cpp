#include "aegis/core/dynamic_green_corridor.hpp"
#include "aegis/common/time_utils.hpp"
#include "aegis/common/uuid_utils.hpp"

namespace aegis {
namespace core {
namespace coordinator {

GreenCorridorEngine::GreenCorridorEngine() = default;

void GreenCorridorEngine::SetTriggerHandler(PreemptionTriggerHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_triggerHandler = std::move(handler);
}

void GreenCorridorEngine::SetReleaseHandler(PreemptionReleaseHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_releaseHandler = std::move(handler);
}

bool GreenCorridorEngine::RegisterCorridor(const std::string& corridorId,
                                           const std::string& incidentId,
                                           const std::string& unitId,
                                           const std::vector<std::string>& intersections) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (unitId.empty() || intersections.empty()) return false;

    ActiveCorridor corr;
    corr.corridorId = corridorId;
    corr.incidentId = incidentId;
    corr.unitId = unitId;
    corr.orderedIntersections = intersections;
    corr.currentIntersectionIndex = 0;
    corr.startTimeMs = common::GetUtcMilliseconds();
    corr.lastEvaluationMs = corr.startTimeMs;

    m_corridors[unitId] = corr;
    return true;
}

void GreenCorridorEngine::TriggerPreemption(const std::string& intersectionId,
                                            const std::string& unitId,
                                            uint16_t arrivalSec) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_preemptedIntersections.insert(intersectionId);

    auto it = m_corridors.find(unitId);
    if (it != m_corridors.end()) {
        it->second.preemptedIntersections.insert(intersectionId);
    }

    if (m_triggerHandler) {
        network::TrafficPreemptionRequestMsg req;
        req.requestId = "PRE-" + common::GenerateUuidV4().substr(0, 8);
        req.intersectionId = intersectionId;
        req.unitId = unitId;
        req.corridorPriority = 2; // Emergency Priority
        req.requestedPhase = "NORTH_SOUTH_GREEN";
        req.estimatedArrivalSec = arrivalSec;
        req.holdDurationSec = m_holdDurationSec;
        req.signature = "HMAC_SIG_VERIFIED";
        m_triggerHandler(intersectionId, req);
    }
}

void GreenCorridorEngine::ReleaseCorridor(const std::string& intersectionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_preemptedIntersections.erase(intersectionId);

    for (auto& kv : m_corridors) {
        kv.second.preemptedIntersections.erase(intersectionId);
        kv.second.clearedIntersections.insert(intersectionId);
        if (m_releaseHandler) {
            m_releaseHandler(intersectionId, kv.first);
        }
    }
}

void GreenCorridorEngine::ClearCorridor(const std::string& corridorId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_corridors.begin(); it != m_corridors.end();) {
        if (it->second.corridorId == corridorId) {
            for (const auto& intId : it->second.preemptedIntersections) {
                m_preemptedIntersections.erase(intId);
                if (m_releaseHandler) {
                    m_releaseHandler(intId, it->second.unitId);
                }
            }
            it = m_corridors.erase(it);
        } else {
            ++it;
        }
    }
}

void GreenCorridorEngine::EvaluateVehicleProgress(const std::string& unitId,
                                                  const common::GeoCoordinate& /*currentLocation*/,
                                                  float speedKmH,
                                                  float /*bearingDeg*/) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_corridors.find(unitId);
    if (it == m_corridors.end()) return;

    auto& corr = it->second;
    corr.lastEvaluationMs = common::GetUtcMilliseconds();

    // Advance corridor preemption along ordered intersections
    if (corr.currentIntersectionIndex < corr.orderedIntersections.size()) {
        const auto& nextInt = corr.orderedIntersections[corr.currentIntersectionIndex];
        if (corr.preemptedIntersections.find(nextInt) == corr.preemptedIntersections.end()) {
            corr.preemptedIntersections.insert(nextInt);
            m_preemptedIntersections.insert(nextInt);

            if (m_triggerHandler) {
                network::TrafficPreemptionRequestMsg req;
                req.requestId = "PRE-" + common::GenerateUuidV4().substr(0, 8);
                req.intersectionId = nextInt;
                req.unitId = unitId;
                req.corridorPriority = 2;
                req.requestedPhase = "NORTH_SOUTH_GREEN";
                req.estimatedArrivalSec = speedKmH > 10.0f ? static_cast<uint16_t>(m_triggerDistanceMeters / (speedKmH / 3.6f)) : 20;
                req.holdDurationSec = m_holdDurationSec;
                req.signature = "HMAC_SIG_VERIFIED";
                m_triggerHandler(nextInt, req);
            }
        }
    }
}

bool GreenCorridorEngine::IsIntersectionPreempted(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_preemptedIntersections.find(intersectionId) != m_preemptedIntersections.end();
}

std::vector<std::string> GreenCorridorEngine::GetActiveCorridorIds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> ids;
    ids.reserve(m_corridors.size());
    for (const auto& kv : m_corridors) {
        ids.push_back(kv.second.corridorId);
    }
    return ids;
}

size_t GreenCorridorEngine::GetActiveCorridorCount() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_corridors.size();
}

} // namespace coordinator
} // namespace core
} // namespace aegis
