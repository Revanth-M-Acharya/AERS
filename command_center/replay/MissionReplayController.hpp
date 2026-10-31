/**
 * @file MissionReplayController.hpp
 * @brief Step-by-step and automated mission replay engine reconstructing historical emergency corridor executions.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "command_center/mission/MissionDatabase.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

namespace aegis::command_center {

struct ReplayTimelineEvent {
    uint32_t stepIndex{0};
    std::string stageName; // "DISPATCH", "AMBULANCE_ASSIGNMENT", "ROUTE", "GREEN_CORRIDOR", "PRIORITY_SCHEDULING", "POLICE_ADVISORY", "DYNAMIC_REPLANNING", "HOSPITAL_ARRIVAL", "MISSION_COMPLETE"
    double timeOffsetSeconds{0.0};
    std::string description;
    std::string activeAmbulance;
    std::string activeIntersection;
    std::string signalsState;
    std::string advisoryMessage;
};

class MissionReplayController {
public:
    MissionReplayController();
    ~MissionReplayController() = default;

    // Prevent copying
    MissionReplayController(const MissionReplayController&) = delete;
    MissionReplayController& operator=(const MissionReplayController&) = delete;

    /**
     * @brief Load a completed mission from MissionDatabase and reconstruct timeline steps.
     */
    bool LoadMission(const HistoricalMissionRecord& mission);

    /**
     * @brief Advance replay to the next sequential event stage.
     */
    bool StepForward();

    /**
     * @brief Step backwards to preceding stage.
     */
    bool StepBackward();

    /**
     * @brief Rewind replay back to beginning (Step 0).
     */
    void Rewind();

    /**
     * @brief Seek to a specific step index.
     */
    bool SeekToStep(uint32_t stepIndex);

    /**
     * @brief Fetch current timeline event.
     */
    ReplayTimelineEvent GetCurrentEvent() const;

    /**
     * @brief Fetch all timeline events for currently loaded mission.
     */
    std::vector<ReplayTimelineEvent> GetAllTimelineEvents() const;

    /**
     * @brief Total steps in loaded mission.
     */
    size_t GetTotalSteps() const;

    /**
     * @brief Current step index (0-based).
     */
    uint32_t GetCurrentStepIndex() const;

    /**
     * @brief Check whether replay has reached final stage.
     */
    bool IsAtEnd() const;

    /**
     * @brief Render formatted ASCII console view of the current replay stage.
     */
    std::string RenderCurrentStageAscii() const;

private:
    void ConstructTimelineFromMission(const HistoricalMissionRecord& mission);

    mutable std::mutex mutex_;
    HistoricalMissionRecord currentMission_;
    std::vector<ReplayTimelineEvent> timeline_;
    uint32_t currentStep_{0};
};

} // namespace aegis::command_center
