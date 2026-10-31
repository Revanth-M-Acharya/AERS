/**
 * @file ScenarioPlaybackController.hpp
 * @brief Controller for automated and manual execution of scripted simulation scenarios.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <cstdint>

namespace aegis::digital_twin {

struct ScenarioScriptEvent {
    double timeOffsetSeconds{0.0};
    std::string eventType;      // e.g. "SPAWN_AMBULANCE", "CREATE_HAZARD", "TRIGGER_PREEMPTION", "FAIL_CAMERA"
    std::string targetEntity;   // e.g. "AMB_01", "SEG_INTA_INTB", "CAM_INTB_01"
    std::string action;         // e.g. "BLOCK_LANE", "UPDATE_VITALS", "DIVERT_HOSPITAL"
    std::string payload;        // parameters or JSON data
    bool hasExecuted{false};
};

using ScenarioEventTriggerCallback = std::function<void(const ScenarioScriptEvent&)>;

class ScenarioPlaybackController {
public:
    ScenarioPlaybackController();
    ~ScenarioPlaybackController() = default;

    void LoadScript(const std::vector<ScenarioScriptEvent>& script);
    void SetTriggerCallback(ScenarioEventTriggerCallback callback);

    // Call periodically with current simulation elapsed time
    void Update(double currentSimTimeSeconds);

    void Reset();
    bool IsComplete() const;
    size_t GetTotalEvents() const;
    size_t GetExecutedEventsCount() const;
    size_t GetPendingEventsCount() const;

private:
    mutable std::mutex mutex_;
    std::vector<ScenarioScriptEvent> scriptEvents_;
    ScenarioEventTriggerCallback callback_;
    bool isComplete_{false};
};

} // namespace aegis::digital_twin

