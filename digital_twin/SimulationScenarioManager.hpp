/**
 * @file SimulationScenarioManager.hpp
 * @brief Catalog and manager of the 11 configurable emergency scenarios.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "digital_twin/ScenarioPlaybackController.hpp"
#include "digital_twin/CityModel.hpp"
#include "digital_twin/MissionTimeline.hpp"
#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace aegis::digital_twin {

enum class ScenarioType {
    SINGLE_AMBULANCE,
    MULTIPLE_AMBULANCES,
    ACCIDENT,
    FLOODING,
    ROAD_CLOSURE,
    HEAVY_CONGESTION,
    CAMERA_FAILURE,
    HOSPITAL_DIVERSION,
    COMMUNICATION_DELAY,
    NODE_FAILURE,
    COORDINATOR_RECOVERY
};

std::string ScenarioTypeToString(ScenarioType type);

class SimulationScenarioManager {
public:
    SimulationScenarioManager(std::shared_ptr<CityModel> cityModel,
                              std::shared_ptr<MissionTimeline> timeline);
    ~SimulationScenarioManager() = default;

    bool LoadScenario(ScenarioType type);
    void Update(double currentSimTimeSeconds);

    void Reset();
    bool IsActive() const;
    bool IsComplete() const;
    ScenarioType GetCurrentScenario() const;
    size_t GetTotalEvents() const;
    size_t GetExecutedEvents() const;

    // Helper to generate the script for any of the 11 scenarios
    static std::vector<ScenarioScriptEvent> GenerateScenarioScript(ScenarioType type);

private:
    void HandleScenarioEvent(const ScenarioScriptEvent& event);

    std::shared_ptr<CityModel> cityModel_;
    std::shared_ptr<MissionTimeline> timeline_;

    mutable std::mutex mutex_;
    ScenarioPlaybackController playbackController_;
    ScenarioType currentScenario_{ScenarioType::SINGLE_AMBULANCE};
    bool isActive_{false};
};

} // namespace aegis::digital_twin

