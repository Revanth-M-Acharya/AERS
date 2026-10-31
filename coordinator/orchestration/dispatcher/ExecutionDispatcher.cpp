/**
 * @file ExecutionDispatcher.cpp
 * @brief Implementation of reliable execution plan dispatching, retries, and ACK tracking.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/orchestration/dispatcher/ExecutionDispatcher.hpp"
#include "common/utils/TimeUtils.hpp"
#include <sstream>

namespace aegis::coordinator::orchestration {

ExecutionDispatcher::ExecutionDispatcher(NodeSender sender,
                                         std::shared_ptr<IEventBus> eventBus,
                                         std::shared_ptr<CoordinatorStateManager> stateManager,
                                         uint32_t maxRetries,
                                         uint32_t dispatchTimeoutMs,
                                         uint32_t retryDelayMs)
    : sender_(std::move(sender)),
      eventBus_(std::move(eventBus)),
      stateManager_(std::move(stateManager)),
      maxRetries_(maxRetries),
      dispatchTimeoutMs_(dispatchTimeoutMs),
      retryDelayMs_(retryDelayMs) {}

void ExecutionDispatcher::SetSender(NodeSender sender) {
    std::lock_guard<std::mutex> lock(dispatcherMutex_);
    sender_ = std::move(sender);
}

bool ExecutionDispatcher::DispatchPlan(const intelligence::ExecutionPlan& plan, uint64_t currentMonoMs) {
    if (plan.planId.empty()) return false;
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    // 1. Strict CRC16 validation of the plan
    if (!plan.VerifyChecksum()) {
        if (eventBus_) {
            eventBus_->Publish(Event(EventType::TrafficIncident, plan.planId, "",
                                     "Plan CRC16 Checksum verification failed; rejecting dispatch",
                                     currentMonoMs));
        }
        return false;
    }

    std::vector<DispatchedCommand> commands;
    uint32_t stageIdx = 0;

    for (const auto& item : plan.signalCommands) {
        DispatchedCommand cmd;
        cmd.planId = plan.planId;
        cmd.stageIndex = stageIdx++;
        cmd.nodeId = item.intersectionId;
        cmd.command = item.signalCommand;
        cmd.activationTimeMonoMs = item.activationTimeMonoMs;
        cmd.holdDurationMs = item.holdDurationMs;
        cmd.dispatchedAtMonoMs = currentMonoMs;
        cmd.retryCount = 0;
        cmd.acknowledged = false;
        cmd.failed = false;
        cmd.planChecksum = plan.checksum;

        {
            std::lock_guard<std::mutex> lock(dispatcherMutex_);
            cmd.sequenceNumber = nextSequenceNumber_++;
        }

        commands.push_back(cmd);
    }

    {
        std::lock_guard<std::mutex> lock(dispatcherMutex_);
        dispatchedPlans_[plan.planId] = commands;
        planFailed_[plan.planId] = false;
    }

    // Transmit commands
    bool allSent = true;
    for (const auto& cmd : commands) {
        if (sender_) {
            // Encode payload: SEQ=<seq>;PLAN=<planId>;CMD=<cmd>;ACT=<act>;HOLD=<hold>;CRC=<crc>
            std::ostringstream oss;
            oss << "SEQ=" << cmd.sequenceNumber
                << ";PLAN=" << cmd.planId
                << ";CMD=" << cmd.command
                << ";ACT=" << cmd.activationTimeMonoMs
                << ";HOLD=" << cmd.holdDurationMs
                << ";CRC=" << cmd.planChecksum;

            protocol::Packet pkt(protocol::MessageType::EMERGENCY_MODE, cmd.nodeId, oss.str(), currentMonoMs);
            bool ok = sender_(cmd.nodeId, pkt);
            if (!ok) {
                allSent = false;
            }
        }
    }

    if (stateManager_) {
        stateManager_->SetActiveExecutionPlan(plan.planId, plan);
    }

    if (eventBus_) {
        eventBus_->Publish(Event(EventType::ExecutionStarted, plan.planId, plan.ambulanceId,
                                 "Dispatched " + std::to_string(commands.size()) + " stages",
                                 currentMonoMs));
    }

    return allSent;
}

bool ExecutionDispatcher::ProcessAckPacket(const std::string& nodeId,
                                           const protocol::Packet& packet,
                                           uint64_t currentMonoMs) {
    if (packet.GetType() != protocol::MessageType::ACK) {
        return false;
    }

    // Parse payload: SEQ=<num>;CRC=<num>
    const std::string& payload = packet.GetPayload();
    uint32_t seq = 0;
    uint16_t crc = 0;

    size_t seqPos = payload.find("SEQ=");
    if (seqPos != std::string::npos) {
        size_t endPos = payload.find(';', seqPos);
        std::string seqStr = (endPos == std::string::npos) ?
            payload.substr(seqPos + 4) : payload.substr(seqPos + 4, endPos - (seqPos + 4));
        try { seq = static_cast<uint32_t>(std::stoul(seqStr)); } catch (...) {}
    }

    size_t crcPos = payload.find("CRC=");
    if (crcPos != std::string::npos) {
        std::string crcStr = payload.substr(crcPos + 4);
        try { crc = static_cast<uint16_t>(std::stoul(crcStr)); } catch (...) {}
    }

    return ProcessAck(nodeId, seq, crc, currentMonoMs);
}

bool ExecutionDispatcher::ProcessAck(const std::string& nodeId,
                                     uint32_t sequenceNumber,
                                     uint16_t checksum,
                                     uint64_t currentMonoMs) {
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    std::lock_guard<std::mutex> lock(dispatcherMutex_);

    // Duplicate rejection
    if (processedAckSequences_.find(sequenceNumber) != processedAckSequences_.end()) {
        return false; // Duplicate packet rejected
    }

    bool matched = false;
    std::string matchedPlanId;

    for (auto& [planId, cmds] : dispatchedPlans_) {
        for (auto& cmd : cmds) {
            if (cmd.sequenceNumber == sequenceNumber && cmd.nodeId == nodeId) {
                if (checksum != 0 && cmd.planChecksum != 0 && cmd.planChecksum != checksum) {
                    return false; // Checksum mismatch
                }
                cmd.acknowledged = true;
                matched = true;
                matchedPlanId = planId;
                break;
            }
        }
        if (matched) break;
    }

    if (matched) {
        processedAckSequences_.insert(sequenceNumber);
    }

    return matched;
}

void ExecutionDispatcher::CheckTimeoutsAndRetry(uint64_t currentMonoMs) {
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    std::vector<DispatchedCommand> toRetry;
    std::vector<std::pair<std::string, std::string>> failedCommands; // planId, nodeId

    {
        std::lock_guard<std::mutex> lock(dispatcherMutex_);
        for (auto& [planId, cmds] : dispatchedPlans_) {
            for (auto& cmd : cmds) {
                if (!cmd.acknowledged && !cmd.failed) {
                    if (currentMonoMs > cmd.dispatchedAtMonoMs &&
                        (currentMonoMs - cmd.dispatchedAtMonoMs) >= dispatchTimeoutMs_) {
                        if (cmd.retryCount < maxRetries_) {
                            cmd.retryCount++;
                            cmd.dispatchedAtMonoMs = currentMonoMs;
                            toRetry.push_back(cmd);
                        } else {
                            cmd.failed = true;
                            planFailed_[planId] = true;
                            failedCommands.push_back({planId, cmd.nodeId});
                        }
                    }
                }
            }
        }
    }

    // Execute retries
    for (const auto& cmd : toRetry) {
        if (sender_) {
            std::ostringstream oss;
            oss << "SEQ=" << cmd.sequenceNumber
                << ";PLAN=" << cmd.planId
                << ";CMD=" << cmd.command
                << ";ACT=" << cmd.activationTimeMonoMs
                << ";HOLD=" << cmd.holdDurationMs
                << ";CRC=" << cmd.planChecksum
                << ";RETRY=" << cmd.retryCount;

            protocol::Packet pkt(protocol::MessageType::EMERGENCY_MODE, cmd.nodeId, oss.str(), currentMonoMs);
            sender_(cmd.nodeId, pkt);
        }
    }

    // Alert on failure
    for (const auto& [pId, nId] : failedCommands) {
        if (eventBus_) {
            eventBus_->Publish(Event(EventType::TrafficIncident, pId, nId,
                                     "Dispatch retry limit exceeded for node " + nId, currentMonoMs));
        }
    }
}

bool ExecutionDispatcher::IsPlanFullyAcknowledged(const std::string& planId) const {
    std::lock_guard<std::mutex> lock(dispatcherMutex_);
    auto it = dispatchedPlans_.find(planId);
    if (it == dispatchedPlans_.end()) return false;
    if (it->second.empty()) return false;

    for (const auto& cmd : it->second) {
        if (!cmd.acknowledged) return false;
    }
    return true;
}

bool ExecutionDispatcher::HasPlanDispatchFailed(const std::string& planId) const {
    std::lock_guard<std::mutex> lock(dispatcherMutex_);
    auto it = planFailed_.find(planId);
    if (it != planFailed_.end()) {
        return it->second;
    }
    return false;
}

size_t ExecutionDispatcher::GetPendingAckCount(const std::string& planId) const {
    std::lock_guard<std::mutex> lock(dispatcherMutex_);
    auto it = dispatchedPlans_.find(planId);
    if (it == dispatchedPlans_.end()) return 0;

    size_t pending = 0;
    for (const auto& cmd : it->second) {
        if (!cmd.acknowledged && !cmd.failed) {
            pending++;
        }
    }
    return pending;
}

size_t ExecutionDispatcher::GetDispatchedCount() const {
    std::lock_guard<std::mutex> lock(dispatcherMutex_);
    size_t count = 0;
    for (const auto& [_, cmds] : dispatchedPlans_) {
        count += cmds.size();
    }
    return count;
}

void ExecutionDispatcher::Reset() {
    std::lock_guard<std::mutex> lock(dispatcherMutex_);
    dispatchedPlans_.clear();
    planFailed_.clear();
    processedAckSequences_.clear();
    nextSequenceNumber_ = 1;
}

} // namespace aegis::coordinator::orchestration

