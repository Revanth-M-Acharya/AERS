/**
 * @file EventBus.cpp
 * @brief Thread-safe EventBus implementation.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/orchestration/event_bus/EventBus.hpp"
#include <algorithm>

namespace aegis::coordinator::orchestration {

EventBus::EventBus() = default;

SubscriptionId EventBus::Subscribe(EventType type, EventHandler handler) {
    if (!handler) return 0;
    std::lock_guard<std::mutex> lock(mutex_);
    SubscriptionId id = nextSubId_++;
    typeSubscribers_[type].push_back({id, false, type, std::move(handler)});
    return id;
}

SubscriptionId EventBus::SubscribeAll(EventHandler handler) {
    if (!handler) return 0;
    std::lock_guard<std::mutex> lock(mutex_);
    SubscriptionId id = nextSubId_++;
    allSubscribers_.push_back({id, true, EventType::EmergencyCreated, std::move(handler)});
    return id;
}

void EventBus::Unsubscribe(SubscriptionId id) {
    if (id == 0) return;
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [type, subs] : typeSubscribers_) {
        subs.erase(std::remove_if(subs.begin(), subs.end(),
                                  [id](const Subscriber& s) { return s.id == id; }),
                   subs.end());
    }
    allSubscribers_.erase(std::remove_if(allSubscribers_.begin(), allSubscribers_.end(),
                                         [id](const Subscriber& s) { return s.id == id; }),
                          allSubscribers_.end());
}

void EventBus::Publish(const Event& event) {
    std::vector<EventHandler> handlersToCall;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = typeSubscribers_.find(event.type);
        if (it != typeSubscribers_.end()) {
            for (const auto& sub : it->second) {
                if (sub.handler) handlersToCall.push_back(sub.handler);
            }
        }
        for (const auto& sub : allSubscribers_) {
            if (sub.handler) handlersToCall.push_back(sub.handler);
        }
    }

    // Invoke handlers outside lock to prevent re-entrancy deadlock
    for (const auto& handler : handlersToCall) {
        handler(event);
    }
}

size_t EventBus::GetSubscriberCount(EventType type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = allSubscribers_.size();
    auto it = typeSubscribers_.find(type);
    if (it != typeSubscribers_.end()) {
        count += it->second.size();
    }
    return count;
}

void EventBus::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    typeSubscribers_.clear();
    allSubscribers_.clear();
}

} // namespace aegis::coordinator::orchestration

