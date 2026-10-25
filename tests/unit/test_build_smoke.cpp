#include <iostream>
#include <cassert>
#include "aegis/common/types.hpp"
#include "aegis/common/geo_utils.hpp"
#include "aegis/config/config_manager.hpp"
#include "aegis/logger/logger.hpp"
#include "aegis/safety/safety_monitor.hpp"
#include "aegis/network/network_manager.hpp"
#include "aegis/core/priority_scheduler.hpp"
#include "aegis/core/route_planner.hpp"
#include "aegis/core/coordinator.hpp"
#include "aegis/edge/ambulance_node.hpp"
#include "aegis/edge/traffic_controller.hpp"
#include "aegis/edge/traffic_signal_node.hpp"
#include "aegis/dashboard/hospital_dashboard_backend.hpp"
#include "aegis/dashboard/traffic_dashboard_backend.hpp"

int main() {
    std::cout << "[SMOKE TEST] Initializing all 12 SMART AEGIS modules..." << std::endl;

    // 1. Common
    aegis::common::GeoCoordinate origin{12.971598, 77.594562, 0.0};
    aegis::common::GeoCoordinate dest{12.973412, 77.597120, 0.0};
    assert(aegis::common::CalculateHaversineDistance(origin, dest) >= 0.0);

    // 2. Config
    aegis::config::ConfigManager config;
    assert(config.LoadProfile("config/default.json") == aegis::common::ErrorCode::SUCCESS);

    // 3. Logger
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::INFO, "TEST", "Smoke test log entry");

    // 4. Safety
    aegis::safety::SafetyMonitor safety;
    assert(safety.Initialize(200, 500));
    assert(safety.IsSystemHealthy());

    // 5. Network
    aegis::network::NetworkManager network;
    assert(network.Initialize("127.0.0.1", 1883, false));

    // 6. Priority Scheduler
    aegis::core::scheduler::PriorityScheduler scheduler;
    aegis::core::scheduler::TriageEvaluationInput input;
    input.distanceKm = 4.2;
    assert(scheduler.CalculatePriorityScore(input) > 0.0f);

    // 7. Route Planner
    aegis::core::routing::RoutePlanner planner;
    auto route = planner.CalculateRoute(origin, dest);
    assert(!route.routeId.empty());

    // 8. Coordinator
    aegis::core::coordinator::CentralCoordinator coordinator;
    assert(coordinator.Start());
    coordinator.Stop();

    // 9. Ambulance Node
    aegis::edge::ambulance::AmbulanceNode ambulance;
    assert(ambulance.Initialize("AMB-01", "config/default.json"));
    assert(ambulance.Start());
    ambulance.Stop();

    // 10. Traffic Controller & Node
    aegis::edge::traffic::TrafficController traffic;
    assert(traffic.Initialize("INT-101", 3000));
    assert(traffic.Start());
    traffic.Stop();

    aegis::edge::traffic::TrafficSignalNode signalNode;
    assert(signalNode.Start());
    signalNode.Tick(50);
    signalNode.Stop();

    // 11. Hospital Dashboard
    aegis::dashboard::HospitalDashboardBackend hospitalDash;
    assert(hospitalDash.Initialize(9001));

    // 12. Traffic Dashboard
    aegis::dashboard::TrafficDashboardBackend trafficDash;
    assert(trafficDash.Initialize(9002));

    std::cout << "[PASS] All 12 SMART AEGIS modules compiled and verified successfully!" << std::endl;
    return 0;
}
