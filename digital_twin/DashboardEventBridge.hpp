/**
 * @file DashboardEventBridge.hpp
 * @brief Subscribes to Coordinator EventBus and propagates events to CityModel and TelemetryCollector (read-only).
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/orchestration/event_bus/IEventBus.hpp"
#include "digital_twin/CityModel.hpp"
#include "digital_twin/TelemetryCollector.hpp"
#include <memory>
#include <vector>
#include <mutex>

namespace aegis::digital_twin {

class DashboardEventBridge {
public:
    DashboardEventBridge(std::shared_ptr<coordinator::orchestration::IEventBus> eventBus,
                         std::shared_ptr<CityModel> cityModel,
                         std::shared_ptr<TelemetryCollector> telemetryCollector);
    ~DashboardEventBridge();

    void StartListening();
    void StopListening();

    bool IsListening() const;
    size_t GetProcessedEventCount() const;

private:
    void HandleEvent(const coordinator::orchestration::Event& event);

    std::shared_ptr<coordinator::orchestration::IEventBus> eventBus_;
    std::shared_ptr<CityModel> cityModel_;
    std::shared_ptr<TelemetryCollector> telemetryCollector_;

    mutable std::mutex mutex_;
    std::vector<coordinator::orchestration::SubscriptionId> subscriptionIds_;
    bool isListening_{false};
    size_t processedEventsCount_{0};
};

} // namespace aegis::digital_twin

