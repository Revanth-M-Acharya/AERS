/**
 * @file RoadEventPublisher.hpp
 * @brief Publishes validated road condition events to the SMART AEGIS EventBus with rate limiting.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/perception/RoadConditionEvent.hpp"
#include "coordinator/orchestration/event_bus/IEventBus.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace aegis::coordinator::perception {

class RoadEventPublisher {
public:
    explicit RoadEventPublisher(std::shared_ptr<orchestration::IEventBus> eventBus = nullptr,
                                uint64_t debounceWindowMs = 1500);
    ~RoadEventPublisher() = default;

    /**
     * @brief Publish a validated road condition event to the EventBus.
     * @param event The event to publish.
     * @return True if event passed rate limiting and was successfully published.
     */
    bool PublishRoadEvent(const RoadConditionEvent& event);

    std::vector<RoadConditionEvent> GetPublishedHistory() const;
    size_t GetTotalPublishedCount() const;

    void SetDebounceWindow(uint64_t ms) { debounceWindowMs_ = ms; }
    void Reset();

private:
    std::shared_ptr<orchestration::IEventBus> eventBus_;
    uint64_t debounceWindowMs_{1500};
    mutable std::mutex mutex_;

    // key: roadSegment + "_" + RoadEventTypeToString(type) -> lastPublishedMonoMs
    std::unordered_map<std::string, uint64_t> lastPublishedMap_;
    std::vector<RoadConditionEvent> history_;
};

} // namespace aegis::coordinator::perception

