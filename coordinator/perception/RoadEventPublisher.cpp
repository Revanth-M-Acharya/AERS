/**
 * @file RoadEventPublisher.cpp
 * @brief Implementation of RoadEventPublisher rate-limited EventBus dispatch.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/perception/RoadEventPublisher.hpp"
#include "common/utils/TimeUtils.hpp"

namespace aegis::coordinator::perception {

RoadEventPublisher::RoadEventPublisher(std::shared_ptr<orchestration::IEventBus> eventBus,
                                       uint64_t debounceWindowMs)
    : eventBus_(std::move(eventBus)), debounceWindowMs_(debounceWindowMs) {}

bool RoadEventPublisher::PublishRoadEvent(const RoadConditionEvent& event) {
    if (event.roadSegment.empty()) return false;

    uint64_t now = event.timestampMonoMs;
    if (now == 0) now = common::TimeUtils::GetMonotonicTimeMs();

    std::string key = event.roadSegment + "_" + RoadEventTypeToString(event.type);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = lastPublishedMap_.find(key);
        if (it != lastPublishedMap_.end()) {
            if (now >= it->second && (now - it->second) < debounceWindowMs_) {
                // Rate limited (debounced)
                return false;
            }
        }

        lastPublishedMap_[key] = now;
        history_.push_back(event);
    }

    if (eventBus_) {
        // Emit high-level incident event
        orchestration::EventType busType = (event.severity >= EventSeverity::HIGH || event.isBlocking)
            ? orchestration::EventType::RoadIncidentDetected
            : orchestration::EventType::RoadConditionUpdated;

        eventBus_->Publish(orchestration::Event(
            busType,
            event.cameraId.empty() ? "IRSA_ENGINE" : event.cameraId,
            event.roadSegment,
            event.ToJson(),
            now
        ));
    }

    return true;
}

std::vector<RoadConditionEvent> RoadEventPublisher::GetPublishedHistory() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return history_;
}

size_t RoadEventPublisher::GetTotalPublishedCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return history_.size();
}

void RoadEventPublisher::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    lastPublishedMap_.clear();
    history_.clear();
}

} // namespace aegis::coordinator::perception

