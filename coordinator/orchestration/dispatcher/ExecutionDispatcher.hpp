/**
 * @file ExecutionDispatcher.hpp
 * @brief Reliable, sequenced dispatching of immutable execution plans to distributed nodes.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/intelligence/execution/IExecutionPlanGenerator.hpp"
#include "common/protocol/Packet.hpp"
#include "coordinator/orchestration/event_bus/IEventBus.hpp"
#include "coordinator/orchestration/state/CoordinatorStateManager.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <mutex>
#include <memory>
#include <cstdint>

namespace aegis::coordinator::orchestration {

using NodeSender = std::function<bool(const std::string& nodeId, const protocol::Packet& packet)>;

struct DispatchedCommand {
    std::string planId;
    uint32_t stageIndex{0};
    std::string nodeId;
    std::string command;
    uint64_t activationTimeMonoMs{0};
    uint32_t holdDurationMs{0};
    uint64_t dispatchedAtMonoMs{0};
    uint32_t retryCount{0};
    bool acknowledged{false};
    bool failed{false};
    uint32_t sequenceNumber{0};
    uint16_t planChecksum{0};
};

class ExecutionDispatcher {
public:
    ExecutionDispatcher(NodeSender sender,
                        std::shared_ptr<IEventBus> eventBus = nullptr,
                        std::shared_ptr<CoordinatorStateManager> stateManager = nullptr,
                        uint32_t maxRetries = 3,
                        uint32_t dispatchTimeoutMs = 2000,
                        uint32_t retryDelayMs = 500);
    ~ExecutionDispatcher() = default;

    /**
     * @brief Dispatch all stages of an immutable execution plan in strict sequence.
     * Validates CRC16 before transmitting.
     */
    bool DispatchPlan(const intelligence::ExecutionPlan& plan, uint64_t currentMonoMs = 0);

    /**
     * @brief Process an incoming ACK packet from a distributed node.
     * Validates packet CRC16 and rejects duplicates.
     */
    bool ProcessAckPacket(const std::string& nodeId, const protocol::Packet& packet, uint64_t currentMonoMs = 0);

    /**
     * @brief Process ACK by explicit sequence number and plan checksum.
     */
    bool ProcessAck(const std::string& nodeId, uint32_t sequenceNumber, uint16_t checksum, uint64_t currentMonoMs = 0);

    /**
     * @brief Check unacknowledged commands for timeout and perform retries up to maxRetries.
     */
    void CheckTimeoutsAndRetry(uint64_t currentMonoMs = 0);

    bool IsPlanFullyAcknowledged(const std::string& planId) const;
    bool HasPlanDispatchFailed(const std::string& planId) const;
    size_t GetPendingAckCount(const std::string& planId) const;
    size_t GetDispatchedCount() const;
    uint32_t GetRetryDelayMs() const { return retryDelayMs_; }

    void SetSender(NodeSender sender);
    void Reset();

private:
    NodeSender sender_;
    std::shared_ptr<IEventBus> eventBus_;
    std::shared_ptr<CoordinatorStateManager> stateManager_;
    uint32_t maxRetries_{3};
    uint32_t dispatchTimeoutMs_{2000};
    uint32_t retryDelayMs_{500};

    mutable std::mutex dispatcherMutex_;
    uint32_t nextSequenceNumber_{1};
    std::unordered_map<std::string, std::vector<DispatchedCommand>> dispatchedPlans_;
    std::unordered_map<std::string, bool> planFailed_;
    std::unordered_set<uint32_t> processedAckSequences_;
};

} // namespace aegis::coordinator::orchestration
