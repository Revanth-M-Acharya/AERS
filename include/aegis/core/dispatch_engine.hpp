/**
 * @file dispatch_engine.hpp
 * @brief Dispatch selection logic matching capabilities, distance, and triage priority.
 */

#ifndef AEGIS_CORE_DISPATCH_ENGINE_HPP
#define AEGIS_CORE_DISPATCH_ENGINE_HPP

#include <string>
#include <vector>
#include "aegis/core/incident_manager.hpp"
#include "aegis/core/fleet_manager.hpp"
#include "aegis/common/result.hpp"

namespace aegis {
namespace core {
namespace coordinator {

struct DispatchDecision {
    std::string unitId;
    double estimatedDistanceMeters{0.0};
    double estimatedArrivalSec{0.0};
    float matchScore{0.0f};
};

class DispatchEngine {
public:
    DispatchEngine() = default;
    ~DispatchEngine() = default;

    common::Result<DispatchDecision> SelectBestAmbulance(
        const IncidentRecord& incident,
        const FleetManager& fleet);

    bool CheckCapabilities(const std::vector<std::string>& required,
                           const std::vector<std::string>& available) const;
};

} // namespace coordinator
} // namespace core
} // namespace aegis

#endif // AEGIS_CORE_DISPATCH_ENGINE_HPP
