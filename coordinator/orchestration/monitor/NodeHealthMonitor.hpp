/**
 * @file NodeHealthMonitor.hpp
 * @brief Distributed node health, telemetry, and connection quality supervision.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/orchestration/state/CoordinatorStateManager.hpp"
#include "coordinator/orchestration/event_bus/IEventBus.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <memory>

namespace aegis::coordinator::orchestration {

class NodeHealthMonitor {
public:
    explicit NodeHealthMonitor(std::shared_ptr<CoordinatorStateManager> stateManager = nullptr,
                               std::shared_ptr<IEventBus> eventBus = nullptr,
                               uint32_t heartbeatTimeoutMs = 5000);
    ~NodeHealthMonitor() = default;

    /**
     * @brief Record heartbeat packet receipt and parse hardware telemetry metrics.
     */
    void RecordHeartbeat(const std::string& nodeId,
                         double cpuUsagePercent,
                         double memUsagePercent,
                         double tempCelsius,
                         uint32_t rttMs = 0,
                         uint64_t currentMonoMs = 0);

    /**
     * @brief Update packet loss ratio for a node.
     */
    void RecordPacketLoss(const std::string& nodeId, double lossPercent);

    /**
     * @brief Periodic timeout check; flags unresponsive nodes and publishes alerts.
     */
    void CheckHealthTimeouts(uint64_t currentMonoMs = 0);

    /**
     * @brief Manually mark a node as FAILED (e.g. fatal hardware fault reported).
     */
    void MarkNodeFailed(const std::string& nodeId, const std::string& reason = "");

    NodeHealthState GetNodeHealthState(const std::string& nodeId) const;
    std::optional<NodeHealthRecord> GetHealthRecord(const std::string& nodeId) const;
    std::vector<std::string> GetOnlineNodes() const;
    std::vector<std::string> GetOfflineNodes() const;
    std::vector<std::string> GetFailedNodes() const;

    void SetHeartbeatTimeoutMs(uint32_t timeoutMs);
    uint32_t GetHeartbeatTimeoutMs() const;

    void Reset();

private:
    std::shared_ptr<CoordinatorStateManager> stateManager_;
    std::shared_ptr<IEventBus> eventBus_;
    uint32_t heartbeatTimeoutMs_{5000};

    mutable std::mutex monitorMutex_;
    std::unordered_map<std::string, NodeHealthRecord> nodeRecords_;
};

} // namespace aegis::coordinator::orchestration

