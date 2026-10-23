/**
 * @file fleet_manager.hpp
 * @brief Fleet management supervisor wrapping the ambulance registry.
 */

#ifndef AEGIS_CORE_FLEET_MANAGER_HPP
#define AEGIS_CORE_FLEET_MANAGER_HPP

#include "aegis/core/ambulance_registry.hpp"

namespace aegis {
namespace core {
namespace coordinator {

class FleetManager {
public:
    FleetManager() = default;
    ~FleetManager() = default;

    AmbulanceRegistry& Registry() noexcept { return m_registry; }
    const AmbulanceRegistry& Registry() const noexcept { return m_registry; }

    bool RegisterUnit(const AmbulanceRecord& rec) {
        return m_registry.RegisterAmbulance(rec);
    }

    bool ReserveUnit(const std::string& unitId, const std::string& incidentId) {
        return m_registry.ReserveUnit(unitId, incidentId);
    }

    bool ReleaseUnit(const std::string& unitId) {
        return m_registry.ReleaseUnit(unitId);
    }

    bool UpdateUnitState(const std::string& unitId, common::AmbulanceState state) {
        return m_registry.UpdateState(unitId, state);
    }

    bool UpdateTelemetry(const std::string& unitId, const common::GeoCoordinate& loc, float speed, float bearing) {
        return m_registry.UpdateTelemetry(unitId, loc, speed, bearing);
    }

    common::Result<AmbulanceRecord> GetUnit(const std::string& unitId) const {
        return m_registry.GetAmbulance(unitId);
    }

    std::vector<AmbulanceRecord> GetAvailableUnits() const {
        return m_registry.GetAvailableAmbulances();
    }

    void PopulateSimulatedFleet();

private:
    AmbulanceRegistry m_registry;
};

} // namespace coordinator
} // namespace core
} // namespace aegis

#endif // AEGIS_CORE_FLEET_MANAGER_HPP
