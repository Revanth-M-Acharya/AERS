/**
 * @file PoliceReplanningCoordinator.hpp
 * @brief Dynamic route replanning triggered by police verified road event (ROAD CLOSED / BARRICADE).
 * Coordinator retains 100% operational authority and autonomously re-routes around the blockage with zero mission pause.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/intelligence/graph/RoadGraph.hpp"
#include "coordinator/intelligence/planner/IRoutePlanner.hpp"
#include "coordinator/intelligence/corridor/ICorridorPlanner.hpp"
#include "coordinator/orchestration/event_bus/IEventBus.hpp"
#include "digital_twin/DigitalTwinEngine.hpp"
#include "hardware_integration/demo/BuzzerController.hpp"
#include "hardware_integration/demo/OledV2XDisplay.hpp"
#include "hardware_integration/relay/ProductionRelayDriver.hpp"

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <chrono>

namespace aegis::hardware_integration::demo {

struct PoliceReplanReport {
    bool success{false};
    std::string blockedIntersectionId{"INT_2"};
    std::string incidentReason{"POLICE_BARRICADE_ROAD_CLOSED"};
    std::vector<std::string> oldRouteNodes;
    std::vector<std::string> newRouteNodes;
    double oldEtaSec{35.0};
    double newEtaSec{42.0};
    uint64_t replanDurationMicroseconds{0};
    bool missionPaused{false};       // MUST BE FALSE (Zero pause constraint)
    bool coordinatorAuthorityRetained{true}; // MUST BE TRUE (Coordinator authority constraint)
    uint64_t timestampMonoMs{0};
};

class PoliceReplanningCoordinator {
public:
    PoliceReplanningCoordinator(
        std::shared_ptr<coordinator::intelligence::RoadGraph> graph,
        std::shared_ptr<coordinator::intelligence::IRoutePlanner> routePlanner,
        std::shared_ptr<coordinator::intelligence::ICorridorPlanner> corridorPlanner,
        std::shared_ptr<coordinator::orchestration::IEventBus> eventBus = nullptr,
        std::shared_ptr<digital_twin::DigitalTwinEngine> digitalTwin = nullptr,
        std::shared_ptr<BuzzerController> buzzer = nullptr,
        std::shared_ptr<OledV2XDisplay> oled = nullptr);
    ~PoliceReplanningCoordinator() = default;

    // Prevent copying
    PoliceReplanningCoordinator(const PoliceReplanningCoordinator&) = delete;
    PoliceReplanningCoordinator& operator=(const PoliceReplanningCoordinator&) = delete;

    /**
     * @brief Register signal driver for an intersection node to manipulate signals during replanning.
     */
    void RegisterNodeRelayDriver(const std::string& nodeId, std::shared_ptr<ProductionRelayDriver> relayDriver);

    /**
     * @brief Execute autonomous dynamic replanning upon receipt of verified police road event.
     * @param blockedIntersectionId Intersection where police report barricade/closure.
     * @param reason Description of the police report.
     * @param nowMonoMs Monotonic timestamp in milliseconds.
     * @param currentAmbulanceLocation Node where ambulance is currently located when report occurs.
     * @return Execution report.
     */
    PoliceReplanReport ExecutePoliceReplanning(
        const std::string& blockedIntersectionId = "INT_2",
        const std::string& reason = "TEMPORARY POLICE BARRICADE / ROAD CLOSED",
        uint64_t nowMonoMs = 0,
        const std::string& currentAmbulanceLocation = "INT_1");

    /**
     * @brief Render formatted dashboard view as specified in Phase 12 specifications.
     */
    std::string RenderDashboardDisplay() const;

    /**
     * @brief Retrieve the most recent replanning execution report.
     */
    PoliceReplanReport GetLastReport() const;

    /**
     * @brief Total number of successful autonomous replanning events handled.
     */
    uint64_t GetReplanningCount() const;

private:
    std::shared_ptr<coordinator::intelligence::RoadGraph> graph_;
    std::shared_ptr<coordinator::intelligence::IRoutePlanner> routePlanner_;
    std::shared_ptr<coordinator::intelligence::ICorridorPlanner> corridorPlanner_;
    std::shared_ptr<coordinator::orchestration::IEventBus> eventBus_;
    std::shared_ptr<digital_twin::DigitalTwinEngine> digitalTwin_;
    std::shared_ptr<BuzzerController> buzzer_;
    std::shared_ptr<OledV2XDisplay> oled_;

    std::unordered_map<std::string, std::shared_ptr<ProductionRelayDriver>> relayDrivers_;

    mutable std::mutex mutex_;
    PoliceReplanReport lastReport_;
    uint64_t replanCount_{0};
};

} // namespace aegis::hardware_integration::demo
