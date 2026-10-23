#include "aegis/core/fleet_manager.hpp"

namespace aegis {
namespace core {
namespace coordinator {

void FleetManager::PopulateSimulatedFleet() {
    m_registry.Clear();

    // AMB-01: Advanced Life Support, Central Depot
    AmbulanceRecord amb1;
    amb1.unitId = "AMB-01";
    amb1.vehicleType = "ALS";
    amb1.state = common::AmbulanceState::AVAILABLE;
    amb1.location = common::GeoCoordinate{12.9716, 77.5946, 0.0};
    amb1.capabilities = {"ECG", "VENTILATOR", "DEFIBRILLATOR", "OXYGEN", "TRAUMA_KIT"};
    m_registry.RegisterAmbulance(amb1);

    // AMB-02: Basic Life Support, Indiranagar Sector
    AmbulanceRecord amb2;
    amb2.unitId = "AMB-02";
    amb2.vehicleType = "BLS";
    amb2.state = common::AmbulanceState::AVAILABLE;
    amb2.location = common::GeoCoordinate{12.9780, 77.6350, 0.0};
    amb2.capabilities = {"OXYGEN", "TRAUMA_KIT"};
    m_registry.RegisterAmbulance(amb2);

    // AMB-03: Advanced Life Support, Koramangala Sector
    AmbulanceRecord amb3;
    amb3.unitId = "AMB-03";
    amb3.vehicleType = "ALS";
    amb3.state = common::AmbulanceState::AVAILABLE;
    amb3.location = common::GeoCoordinate{12.9350, 77.6250, 0.0};
    amb3.capabilities = {"ECG", "VENTILATOR", "DEFIBRILLATOR", "OXYGEN", "TRAUMA_KIT"};
    m_registry.RegisterAmbulance(amb3);

    // AMB-04: Basic Life Support, Old Airport Sector
    AmbulanceRecord amb4;
    amb4.unitId = "AMB-04";
    amb4.vehicleType = "BLS";
    amb4.state = common::AmbulanceState::AVAILABLE;
    amb4.location = common::GeoCoordinate{12.9570, 77.6550, 0.0};
    amb4.capabilities = {"OXYGEN", "TRAUMA_KIT"};
    m_registry.RegisterAmbulance(amb4);
}

} // namespace coordinator
} // namespace core
} // namespace aegis
