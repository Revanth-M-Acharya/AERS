/**
 * @file EventBus.hpp
 * @brief Thread-safe implementation of IEventBus for decoupled inter-module messaging.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/orchestration/event_bus/IEventBus.hpp"
#include <mutex>
#include <unordered_map>
#include <vector>
#include <atomic>

namespace aegis::coordinator::orchestration {

class EventBus : public IEventBus {
public:
    EventBus();
    ~EventBus() override = default;

    SubscriptionId Subscribe(EventType type, EventHandler handler) override;
    SubscriptionId SubscribeAll(EventHandler handler) override;
    void Unsubscribe(SubscriptionId id) override;
    void Publish(const Event& event) override;
    size_t GetSubscriberCount(EventType type) const override;
    void Clear() override;

private:
    struct Subscriber {
        SubscriptionId id{0};
        bool allEvents{false};
        EventType type{EventType::EmergencyCreated};
        EventHandler handler;
    };

    mutable std::mutex mutex_;
    std::atomic<SubscriptionId> nextSubId_{1};
    std::unordered_map<EventType, std::vector<Subscriber>> typeSubscribers_;
    std::vector<Subscriber> allSubscribers_;
};

} // namespace aegis::coordinator::orchestration

