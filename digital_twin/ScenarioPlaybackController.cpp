/**
 * @file ScenarioPlaybackController.cpp
 * @brief Implementation of ScenarioPlaybackController.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "digital_twin/ScenarioPlaybackController.hpp"
#include <algorithm>

namespace aegis::digital_twin {

ScenarioPlaybackController::ScenarioPlaybackController() {
    Reset();
}

void ScenarioPlaybackController::LoadScript(const std::vector<ScenarioScriptEvent>& script) {
    std::lock_guard<std::mutex> lock(mutex_);
    scriptEvents_ = script;
    // Sort chronologically by time offset
    std::sort(scriptEvents_.begin(), scriptEvents_.end(), [](const ScenarioScriptEvent& a, const ScenarioScriptEvent& b) {
        return a.timeOffsetSeconds < b.timeOffsetSeconds;
    });
    for (auto& ev : scriptEvents_) {
        ev.hasExecuted = false;
    }
    isComplete_ = scriptEvents_.empty();
}

void ScenarioPlaybackController::SetTriggerCallback(ScenarioEventTriggerCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = std::move(callback);
}

void ScenarioPlaybackController::Update(double currentSimTimeSeconds) {
    std::vector<ScenarioScriptEvent> toTrigger;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (scriptEvents_.empty()) {
            isComplete_ = true;
            return;
        }

        bool allDone = true;
        for (auto& ev : scriptEvents_) {
            if (!ev.hasExecuted) {
                if (currentSimTimeSeconds >= ev.timeOffsetSeconds) {
                    ev.hasExecuted = true;
                    toTrigger.push_back(ev);
                } else {
                    allDone = false;
                }
            }
        }
        isComplete_ = allDone;
    }

    if (callback_) {
        for (const auto& ev : toTrigger) {
            callback_(ev);
        }
    }
}

void ScenarioPlaybackController::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& ev : scriptEvents_) {
        ev.hasExecuted = false;
    }
    isComplete_ = scriptEvents_.empty();
}

bool ScenarioPlaybackController::IsComplete() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return isComplete_;
}

size_t ScenarioPlaybackController::GetTotalEvents() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return scriptEvents_.size();
}

size_t ScenarioPlaybackController::GetExecutedEventsCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& ev : scriptEvents_) {
        if (ev.hasExecuted) count++;
    }
    return count;
}

size_t ScenarioPlaybackController::GetPendingEventsCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& ev : scriptEvents_) {
        if (!ev.hasExecuted) count++;
    }
    return count;
}

} // namespace aegis::digital_twin

