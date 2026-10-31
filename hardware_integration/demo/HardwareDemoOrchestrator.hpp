/**
 * @file HardwareDemoOrchestrator.hpp
 * @brief Master DEMO MODE Orchestrator for Phase 12 Hardware Demonstration.
 * Automates and sequences the 5 key hackathon demonstration missions across the 4 Raspberry Pis.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "hardware_integration/gpio/SymbolicGpioHAL.hpp"
#include "hardware_integration/demo/DemoHardwareSignals.hpp"
#include "hardware_integration/demo/BuzzerController.hpp"
#include "hardware_integration/demo/EmergencySwitchDriver.hpp"
#include "hardware_integration/demo/PoliceOverrideSwitchDriver.hpp"
#include "hardware_integration/demo/OledV2XDisplay.hpp"
#include "hardware_integration/demo/Intersection2PriorityDemonstrator.hpp"
#include "hardware_integration/demo/PoliceReplanningCoordinator.hpp"
#include "hardware_integration/relay/ProductionRelayDriver.hpp"
#include "hardware_integration/sensor/IRSensorDriver.hpp"
#include "digital_twin/DigitalTwinEngine.hpp"
#include "coordinator/intelligence/graph/RoadGraph.hpp"
#include "coordinator/intelligence/planner/DijkstraRoutePlanner.hpp"
#include "coordinator/intelligence/corridor/GreenCorridorPlanner.hpp"
#include "coordinator/intelligence/scheduler/PriorityScheduler.hpp"
#include "coordinator/orchestration/event_bus/EventBus.hpp"

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>

namespace aegis::hardware_integration::demo {

enum class DemoMissionId {
    MISSION_1_NORMAL_CORRIDOR,
    MISSION_2_PRIORITY_SCHEDULING,
    MISSION_3_POLICE_REPLANNING,
    MISSION_4_FAULT_INJECTION,
    MISSION_5_MISSION_REPLAY
};

struct DemoExecutionMetrics {
    DemoMissionId missionId{DemoMissionId::MISSION_1_NORMAL_CORRIDOR};
    std::string missionName;
    bool success{false};
    uint64_t durationMs{0};
    std::string summary;
};

class HardwareDemoOrchestrator {
public:
    HardwareDemoOrchestrator(
        std::shared_ptr<SymbolicGpioHAL> hal = nullptr,
        std::shared_ptr<digital_twin::DigitalTwinEngine> digitalTwin = nullptr,
        uint32_t stepDelayMs = 0);
    ~HardwareDemoOrchestrator() = default;

    // Prevent copying
    HardwareDemoOrchestrator(const HardwareDemoOrchestrator&) = delete;
    HardwareDemoOrchestrator& operator=(const HardwareDemoOrchestrator&) = delete;

    /**
     * @brief Initialize all 4 Raspberry Pi sub-controllers, drivers, road graph, and event bus.
     */
    bool Initialize();

    /**
     * @brief Set delay between demonstration stages (0 for instant/test mode, 1500ms for live presentation).
     */
    void SetStepDelayMs(uint32_t delayMs);

    /**
     * @brief Mission 1: Normal Green Corridor (Home -> Int 1 -> Int 2 -> Int 3 -> Hospital).
     */
    DemoExecutionMetrics ExecuteMission1_NormalCorridor();

    /**
     * @brief Mission 2: Dual-Ambulance Priority Conflict Arbitration at Intersection 2.
     */
    DemoExecutionMetrics ExecuteMission2_PriorityScheduling();

    /**
     * @brief Mission 3: Police-Assisted Dynamic Route Replanning around barricaded Intersection 2.
     */
    DemoExecutionMetrics ExecuteMission3_PoliceReplanning();

    /**
     * @brief Mission 4: Roadside Fault Injection & Fail-Safe Fallback.
     */
    DemoExecutionMetrics ExecuteMission4_FaultInjection();

    /**
     * @brief Mission 5: Mission Replay & Post-Mission Telemetry Review.
     */
    DemoExecutionMetrics ExecuteMission5_MissionReplay();

    /**
     * @brief Execute all 5 missions in sequence within the 5-7 minute demonstration window.
     */
    std::vector<DemoExecutionMetrics> ExecuteFullDemoSuite();

    // Component Getters
    std::shared_ptr<BuzzerController> GetBuzzer() const { return buzzer_; }
    std::shared_ptr<EmergencySwitchDriver> GetEmergencySwitch() const { return emergencySwitch_; }
    std::shared_ptr<PoliceOverrideSwitchDriver> GetPoliceSwitch() const { return policeSwitch_; }
    std::shared_ptr<OledV2XDisplay> GetOled() const { return oled_; }
    std::shared_ptr<Intersection2PriorityDemonstrator> GetPriorityDemonstrator() const { return priorityDemonstrator_; }
    std::shared_ptr<PoliceReplanningCoordinator> GetPoliceReplanner() const { return policeReplanner_; }
    std::shared_ptr<coordinator::intelligence::RoadGraph> GetRoadGraph() const { return graph_; }
    std::shared_ptr<digital_twin::DigitalTwinEngine> GetDigitalTwin() const { return digitalTwin_; }

private:
    void SetupDemoRoadNetwork();
    void DelayStep();

    std::shared_ptr<SymbolicGpioHAL> hal_;
    std::shared_ptr<digital_twin::DigitalTwinEngine> digitalTwin_;
    uint32_t stepDelayMs_{0};

    std::shared_ptr<coordinator::orchestration::EventBus> eventBus_;
    std::shared_ptr<coordinator::intelligence::RoadGraph> graph_;
    std::shared_ptr<coordinator::intelligence::DijkstraRoutePlanner> routePlanner_;
    std::shared_ptr<coordinator::intelligence::GreenCorridorPlanner> corridorPlanner_;
    std::shared_ptr<coordinator::intelligence::PriorityScheduler> priorityScheduler_;

    std::shared_ptr<BuzzerController> buzzer_;
    std::shared_ptr<EmergencySwitchDriver> emergencySwitch_;
    std::shared_ptr<PoliceOverrideSwitchDriver> policeSwitch_;
    std::shared_ptr<OledV2XDisplay> oled_;
    std::shared_ptr<Intersection2PriorityDemonstrator> priorityDemonstrator_;
    std::shared_ptr<PoliceReplanningCoordinator> policeReplanner_;

    std::shared_ptr<ProductionRelayDriver> relayInt1_;
    std::shared_ptr<ProductionRelayDriver> relayInt2_;
    std::shared_ptr<ProductionRelayDriver> relayInt3_;
    std::shared_ptr<ProductionRelayDriver> relayBypass_;

    std::shared_ptr<IRSensorDriver> irInt1_;
    std::shared_ptr<IRSensorDriver> irInt2Top_;
    std::shared_ptr<IRSensorDriver> irInt2Right_;
    std::shared_ptr<IRSensorDriver> irInt3_;

    mutable std::mutex mutex_;
    std::vector<DemoExecutionMetrics> history_;
    bool isInitialized_{false};
};

} // namespace aegis::hardware_integration::demo
