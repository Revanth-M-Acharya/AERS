#include "aegis/core/dispatch_engine.hpp"
#include "aegis/common/geo_utils.hpp"
#include <algorithm>
#include <limits>

namespace aegis {
namespace core {
namespace coordinator {

bool DispatchEngine::CheckCapabilities(const std::vector<std::string>& required,
                                       const std::vector<std::string>& available) const {
    for (const auto& req : required) {
        if (std::find(available.begin(), available.end(), req) == available.end()) {
            return false;
        }
    }
    return true;
}

common::Result<DispatchDecision> DispatchEngine::SelectBestAmbulance(
    const IncidentRecord& incident,
    const FleetManager& fleet) {

    auto availableUnits = fleet.GetAvailableUnits();
    if (availableUnits.empty()) {
        return common::Result<DispatchDecision>::Err(common::ErrorCode::ERR_PREEMPT_REJECTED);
    }

    double bestScore = std::numeric_limits<double>::max();
    DispatchDecision bestDecision;
    bool found = false;

    for (const auto& unit : availableUnits) {
        // 1. Verify capability requirements
        if (!CheckCapabilities(incident.requiredCapabilities, unit.capabilities)) {
            continue;
        }

        // 2. Haversine distance
        double distMeters = common::CalculateHaversineDistance(unit.location, incident.location);

        // 3. Priority penalty / weighting
        double score = distMeters;
        if (incident.triageCategory == common::TriageCategory::CODE_RED && unit.vehicleType == "ALS") {
            // Favor ALS for critical patients
            score *= 0.8;
        }

        if (score < bestScore) {
            bestScore = score;
            bestDecision.unitId = unit.unitId;
            bestDecision.estimatedDistanceMeters = distMeters;
            // Assume 50 km/h average emergency transit speed = ~13.89 m/s
            bestDecision.estimatedArrivalSec = distMeters / 13.8889;
            bestDecision.matchScore = static_cast<float>(score);
            found = true;
        }
    }

    if (!found) {
        return common::Result<DispatchDecision>::Err(common::ErrorCode::ERR_PREEMPT_REJECTED);
    }

    return common::Result<DispatchDecision>::Ok(bestDecision);
}

} // namespace coordinator
} // namespace core
} // namespace aegis
