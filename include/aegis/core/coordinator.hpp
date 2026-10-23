/**
 * @file coordinator.hpp
 * @brief Central dispatch coordinator orchestrating calls, fleet, navigation, and green corridors.
 */

#ifndef AEGIS_CORE_COORDINATOR_HPP
#define AEGIS_CORE_COORDINATOR_HPP

#include <memory>
#include <string>
#include "aegis/core/emergency_call_handler.hpp"
#include "aegis/core/incident_manager.hpp"
#include "aegis/core/ambulance_registry.hpp"
#include "aegis/core/fleet_manager.hpp"
#include "aegis/core/dispatch_engine.hpp"
#include "aegis/core/route_planner.hpp"
#include "aegis/core/dynamic_green_corridor.hpp"
#include "aegis/core/priority_scheduler.hpp"
#include "aegis/network/network_manager.hpp"
#include "aegis/common/result.hpp"

namespace aegis {
namespace core {
namespace coordinator {

struct IncidentDispatchResult {
    std::string incidentId;
    std::string assignedUnitId;
    routing::NavigationRoute route;
    std::string corridorId;
    bool success{false};
    std::string failureReason;
};

class CentralCoordinator {
public:
    CentralCoordinator();
    explicit CentralCoordinator(std::shared_ptr<network::NetworkManager> netManager);
    ~CentralCoordinator();

    bool Start();
    void Stop();
    bool IsRunning() const noexcept;

    // Call Intake & Automated Dispatch
    common::Result<IncidentDispatchResult> SubmitEmergencyCall(const EmergencyCallInput& input);

    // Fleet & Kinematics Ingestion
    void ProcessGpsUpdate(const std::string& unitId,
                          const common::GeoCoordinate& location,
                          float speed, float bearing);

    // Subsystem Accessors
    EmergencyCallHandler& CallHandler() noexcept { return m_callHandler; }
    IncidentManager& Incidents() noexcept { return m_incidentManager; }
    FleetManager& Fleet() noexcept { return m_fleetManager; }
    routing::RoutePlanner& RouteEngine() noexcept { return m_routePlanner; }
    DynamicGreenCorridor& CorridorEngine() noexcept { return m_corridorEngine; }
    network::NetworkManager& Network() noexcept { return *m_network; }

private:
    void SetupNetworkSubscriptions();
    void SetupCorridorHandlers();

    std::shared_ptr<network::NetworkManager> m_network;
    bool m_ownsNetwork{false};

    EmergencyCallHandler m_callHandler;
    IncidentManager m_incidentManager;
    FleetManager m_fleetManager;
    DispatchEngine m_dispatchEngine;
    routing::RoutePlanner m_routePlanner;
    DynamicGreenCorridor m_corridorEngine;
    scheduler::PriorityScheduler m_priorityScheduler;

    bool m_running{false};
};

// Alias for architecture compliance
using Coordinator = CentralCoordinator;

} // namespace coordinator
} // namespace core
} // namespace aegis

#endif // AEGIS_CORE_COORDINATOR_HPP
