/**
 * @file Intersection2PriorityDemonstrator.hpp
 * @brief Demonstration coordinator for Feature 3: Intersection 2 Dual-Ambulance Priority Scheduling.
 * Arbitrates simultaneous converging ambulances using the existing Coordinator PriorityScheduler.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/intelligence/scheduler/PriorityScheduler.hpp"
#include "hardware_integration/demo/BuzzerController.hpp"
#include "hardware_integration/demo/OledV2XDisplay.hpp"

#include <string>
#include <memory>
#include <mutex>
#include <vector>

namespace aegis::hardware_integration::demo {

struct PriorityArbitrationDetails {
    std::string intersectionId{"INT_2"};
    std::string missionAId{"MISSION_A"};
    std::string ambulanceAId{"AMB_01"};
    std::string approachA{"TOP"};
    coordinator::intelligence::EmergencyPriorityLevel priorityA{coordinator::intelligence::EmergencyPriorityLevel::CRITICAL};

    std::string missionBId{"MISSION_B"};
    std::string ambulanceBId{"AMB_02"};
    std::string approachB{"RIGHT"};
    coordinator::intelligence::EmergencyPriorityLevel priorityB{coordinator::intelligence::EmergencyPriorityLevel::HIGH};

    std::string selectedMissionId{"MISSION_A"};
    std::string selectedAmbulanceId{"AMB_01"};
    std::string selectionReason{"Higher triage severity level (Critical Code Red vs High Code Yellow)"};
    double estimatedWaitTimeSec{15.0};
    uint64_t timestampMonoMs{0};
};

class Intersection2PriorityDemonstrator {
public:
    Intersection2PriorityDemonstrator(
        std::shared_ptr<coordinator::intelligence::PriorityScheduler> scheduler,
        std::shared_ptr<BuzzerController> buzzer = nullptr,
        std::shared_ptr<OledV2XDisplay> oled = nullptr);
    ~Intersection2PriorityDemonstrator() = default;

    // Prevent copying
    Intersection2PriorityDemonstrator(const Intersection2PriorityDemonstrator&) = delete;
    Intersection2PriorityDemonstrator& operator=(const Intersection2PriorityDemonstrator&) = delete;

    /**
     * @brief Execute the simultaneous arrival priority scheduling demonstration at Intersection 2.
     * @param nowMonoMs Monotonic timestamp in milliseconds.
     * @return Full arbitration details.
     */
    PriorityArbitrationDetails DemonstratePriorityConflict(uint64_t nowMonoMs);

    /**
     * @brief Render formatted dashboard view as specified in Phase 12 requirements.
     */
    std::string RenderDashboardPanel() const;

    /**
     * @brief Retrieve the most recent arbitration outcome.
     */
    PriorityArbitrationDetails GetLastArbitration() const;

private:
    std::shared_ptr<coordinator::intelligence::PriorityScheduler> scheduler_;
    std::shared_ptr<BuzzerController> buzzer_;
    std::shared_ptr<OledV2XDisplay> oled_;

    mutable std::mutex mutex_;
    PriorityArbitrationDetails lastDetails_;
    bool hasExecuted_{false};
};

} // namespace aegis::hardware_integration::demo
