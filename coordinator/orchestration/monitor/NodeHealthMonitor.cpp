/**
 * @file NodeHealthMonitor.cpp
 * @brief Implementation of node health telemetry tracking and timeout detection.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/orchestration/monitor/NodeHealthMonitor.hpp"
#include "common/utils/TimeUtils.hpp"

namespace aegis::coordinator::orchestration {

NodeHealthMonitor::NodeHealthMonitor(std::shared_ptr<CoordinatorStateManager> stateManager,
                                     std::shared_ptr<IEventBus> eventBus,
                                     uint32_t heartbeatTimeoutMs)
    : stateManager_(std::move(stateManager)),
      eventBus_(std::move(eventBus)),
      heartbeatTimeoutMs_(heartbeatTimeoutMs) {}

void NodeHealthMonitor::RecordHeartbeat(const std::string& nodeId,
                                        double cpuUsagePercent,
                                        double memUsagePercent,
                                        double tempCelsius,
                                        uint32_t rttMs,
                                        uint64_t currentMonoMs) {
    if (nodeId.empty()) return;
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    NodeHealthState oldState = NodeHealthState::OFFLINE;
    bool isNewNode = false;
    NodeHealthRecord record;

    {
        std::lock_guard<std::mutex> lock(monitorMutex_);
        auto it = nodeRecords_.find(nodeId);
        if (it != nodeRecords_.end()) {
            oldState = it->second.healthState;
            record = it->second;
        } else if (stateManager_ && stateManager_->GetNodeHealth(nodeId).has_value()) {
            record = stateManager_->GetNodeHealth(nodeId).value();
            oldState = record.healthState;
            isNewNode = false;
        } else {
            isNewNode = true;
            record.nodeId = nodeId;
        }

        record.lastHeartbeatMonoMs = currentMonoMs;
        record.consecutiveMissedHeartbeats = 0;
        record.cpuUsagePercent = cpuUsagePercent;
        record.memUsagePercent = memUsagePercent;
        record.tempCelsius = tempCelsius;
        if (rttMs > 0) record.latencyRttMs = rttMs;
        record.connectionStatus = ConnectionStatus::CONNECTED;

        // State transition logic
        if (isNewNode) {
            record.healthState = NodeHealthState::ONLINE;
        } else if (oldState == NodeHealthState::OFFLINE || oldState == NodeHealthState::FAILED) {
            record.healthState = NodeHealthState::RECOVERING;
        } else if (oldState == NodeHealthState::RECOVERING) {
            record.healthState = NodeHealthState::ONLINE;
        } else {
            record.healthState = NodeHealthState::ONLINE;
        }

        nodeRecords_[nodeId] = record;
    }

    if (stateManager_) {
        stateManager_->UpdateNodeHealth(record);
    }

    if (eventBus_) {
        eventBus_->Publish(Event(EventType::HeartbeatReceived, nodeId, "",
                                 "CPU=" + std::to_string(static_cast<int>(cpuUsagePercent)) + "%",
                                 currentMonoMs));

        if (oldState == NodeHealthState::OFFLINE || oldState == NodeHealthState::FAILED) {
            eventBus_->Publish(Event(EventType::NodeRecovered, nodeId, "",
                                     "Node restored from " + NodeHealthStateToString(oldState),
                                     currentMonoMs));
        }
    }
}

void NodeHealthMonitor::RecordPacketLoss(const std::string& nodeId, double lossPercent) {
    if (nodeId.empty()) return;
    std::lock_guard<std::mutex> lock(monitorMutex_);
    auto it = nodeRecords_.find(nodeId);
    if (it != nodeRecords_.end()) {
        it->second.packetLossPercent = lossPercent;
        if (stateManager_) {
            stateManager_->UpdateNodeHealth(it->second);
        }
    }
}

void NodeHealthMonitor::CheckHealthTimeouts(uint64_t currentMonoMs) {
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    std::vector<std::pair<std::string, NodeHealthRecord>> timedOutNodes;

    {
        std::lock_guard<std::mutex> lock(monitorMutex_);
        for (auto& [nodeId, record] : nodeRecords_) {
            if (record.healthState != NodeHealthState::FAILED) {
                if (currentMonoMs > record.lastHeartbeatMonoMs &&
                    (currentMonoMs - record.lastHeartbeatMonoMs) >= heartbeatTimeoutMs_) {
                    uint32_t missedIntervals = static_cast<uint32_t>((currentMonoMs - record.lastHeartbeatMonoMs) / heartbeatTimeoutMs_);
                    record.consecutiveMissedHeartbeats = missedIntervals;
                    if (record.consecutiveMissedHeartbeats >= 3) {
                        record.healthState = NodeHealthState::FAILED;
                    } else {
                        record.healthState = NodeHealthState::OFFLINE;
                    }
                    record.connectionStatus = ConnectionStatus::DISCONNECTED;
                    timedOutNodes.push_back({nodeId, record});
                }
            }
        }
    }

    for (const auto& [nodeId, record] : timedOutNodes) {
        if (stateManager_) {
            stateManager_->UpdateNodeHealth(record);
        }
        if (eventBus_) {
            eventBus_->Publish(Event(EventType::HeartbeatTimeout, nodeId, "",
                                     "Node unresponsive; marked " + NodeHealthStateToString(record.healthState),
                                     currentMonoMs));
        }
    }
}

void NodeHealthMonitor::MarkNodeFailed(const std::string& nodeId, const std::string& reason) {
    if (nodeId.empty()) return;
    uint64_t now = common::TimeUtils::GetMonotonicTimeMs();
    NodeHealthRecord record;

    {
        std::lock_guard<std::mutex> lock(monitorMutex_);
        auto it = nodeRecords_.find(nodeId);
        if (it != nodeRecords_.end()) {
            record = it->second;
        } else {
            record.nodeId = nodeId;
        }
        record.healthState = NodeHealthState::FAILED;
        record.connectionStatus = ConnectionStatus::DISCONNECTED;
        nodeRecords_[nodeId] = record;
    }

    if (stateManager_) {
        stateManager_->UpdateNodeHealth(record);
    }
    if (eventBus_) {
        eventBus_->Publish(Event(EventType::TrafficIncident, nodeId, "",
                                 "Node marked FAILED: " + reason, now));
    }
}

NodeHealthState NodeHealthMonitor::GetNodeHealthState(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(monitorMutex_);
    auto it = nodeRecords_.find(nodeId);
    if (it != nodeRecords_.end()) {
        return it->second.healthState;
    }
    return NodeHealthState::OFFLINE;
}

std::optional<NodeHealthRecord> NodeHealthMonitor::GetHealthRecord(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(monitorMutex_);
    auto it = nodeRecords_.find(nodeId);
    if (it != nodeRecords_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<std::string> NodeHealthMonitor::GetOnlineNodes() const {
    std::lock_guard<std::mutex> lock(monitorMutex_);
    std::vector<std::string> result;
    for (const auto& [nodeId, rec] : nodeRecords_) {
        if (rec.healthState == NodeHealthState::ONLINE) {
            result.push_back(nodeId);
        }
    }
    return result;
}

std::vector<std::string> NodeHealthMonitor::GetOfflineNodes() const {
    std::lock_guard<std::mutex> lock(monitorMutex_);
    std::vector<std::string> result;
    for (const auto& [nodeId, rec] : nodeRecords_) {
        if (rec.healthState == NodeHealthState::OFFLINE) {
            result.push_back(nodeId);
        }
    }
    return result;
}

std::vector<std::string> NodeHealthMonitor::GetFailedNodes() const {
    std::lock_guard<std::mutex> lock(monitorMutex_);
    std::vector<std::string> result;
    for (const auto& [nodeId, rec] : nodeRecords_) {
        if (rec.healthState == NodeHealthState::FAILED) {
            result.push_back(nodeId);
        }
    }
    return result;
}

void NodeHealthMonitor::SetHeartbeatTimeoutMs(uint32_t timeoutMs) {
    std::lock_guard<std::mutex> lock(monitorMutex_);
    heartbeatTimeoutMs_ = timeoutMs;
}

uint32_t NodeHealthMonitor::GetHeartbeatTimeoutMs() const {
    std::lock_guard<std::mutex> lock(monitorMutex_);
    return heartbeatTimeoutMs_;
}

void NodeHealthMonitor::Reset() {
    std::lock_guard<std::mutex> lock(monitorMutex_);
    nodeRecords_.clear();
}

} // namespace aegis::coordinator::orchestration
