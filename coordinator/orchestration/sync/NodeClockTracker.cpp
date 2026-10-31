/**
 * @file NodeClockTracker.cpp
 * @brief Implementation of distributed clock tracking and offset calculation.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/orchestration/sync/NodeClockTracker.hpp"
#include "common/utils/TimeUtils.hpp"
#include <cmath>

namespace aegis::coordinator::orchestration {

NodeClockTracker::NodeClockTracker(std::shared_ptr<CoordinatorStateManager> stateManager,
                                   std::shared_ptr<IEventBus> eventBus,
                                   uint32_t driftThresholdMs)
    : stateManager_(std::move(stateManager)),
      eventBus_(std::move(eventBus)),
      driftThresholdMs_(driftThresholdMs) {}

void NodeClockTracker::RecordPtpSample(const std::string& nodeId,
                                       uint64_t t1ClientSend,
                                       uint64_t t2ServerRecv,
                                       uint64_t t3ServerSend,
                                       uint64_t t4ClientRecv) {
    if (nodeId.empty()) return;

    // Standard PTP / NTP calculation:
    // Offset = ((t2 - t1) + (t3 - t4)) / 2
    // RTT = (t4 - t1) - (t3 - t2)
    int64_t diff1 = static_cast<int64_t>(t2ServerRecv) - static_cast<int64_t>(t1ClientSend);
    int64_t diff2 = static_cast<int64_t>(t3ServerSend) - static_cast<int64_t>(t4ClientRecv);
    int64_t offset = (diff1 + diff2) / 2;

    int64_t rttVal = (static_cast<int64_t>(t4ClientRecv) - static_cast<int64_t>(t1ClientSend)) -
                     (static_cast<int64_t>(t3ServerSend) - static_cast<int64_t>(t2ServerRecv));
    uint32_t rtt = (rttVal > 0) ? static_cast<uint32_t>(rttVal) : 0;

    ClockSyncSample sample;
    sample.nodeId = nodeId;
    sample.clockOffsetMs = offset;
    sample.roundTripTimeMs = rtt;
    sample.sampleTimeMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    sample.valid = true;

    {
        std::lock_guard<std::mutex> lock(trackerMutex_);
        samples_[nodeId] = sample;
    }

    if (stateManager_) {
        NodeSyncRecord sync;
        sync.nodeId = nodeId;
        sync.clockOffsetMs = offset;
        sync.roundTripTimeMs = rtt;
        sync.lastSyncMonoMs = sample.sampleTimeMonoMs;
        sync.isSynchronized = (std::abs(offset) <= static_cast<int64_t>(driftThresholdMs_));
        stateManager_->UpdateNodeSync(sync);
    }
}

void NodeClockTracker::RecordPingSample(const std::string& nodeId,
                                        uint64_t pingSendMonoMs,
                                        uint64_t ackRecvMonoMs,
                                        uint64_t nodeReportedTimestampMs) {
    if (nodeId.empty()) return;

    uint32_t rtt = (ackRecvMonoMs >= pingSendMonoMs) ? static_cast<uint32_t>(ackRecvMonoMs - pingSendMonoMs) : 0;
    int64_t offset = 0;
    if (nodeReportedTimestampMs > 0) {
        uint64_t estimatedNodeTimeWhenAckReceived = nodeReportedTimestampMs + (rtt / 2);
        offset = static_cast<int64_t>(ackRecvMonoMs) - static_cast<int64_t>(estimatedNodeTimeWhenAckReceived);
    }

    ClockSyncSample sample;
    sample.nodeId = nodeId;
    sample.clockOffsetMs = offset;
    sample.roundTripTimeMs = rtt;
    sample.sampleTimeMonoMs = ackRecvMonoMs;
    sample.valid = true;

    {
        std::lock_guard<std::mutex> lock(trackerMutex_);
        samples_[nodeId] = sample;
    }

    if (stateManager_) {
        NodeSyncRecord sync;
        sync.nodeId = nodeId;
        sync.clockOffsetMs = offset;
        sync.roundTripTimeMs = rtt;
        sync.lastSyncMonoMs = ackRecvMonoMs;
        sync.isSynchronized = (std::abs(offset) <= static_cast<int64_t>(driftThresholdMs_));
        stateManager_->UpdateNodeSync(sync);
    }
}

int64_t NodeClockTracker::GetClockOffsetMs(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(trackerMutex_);
    auto it = samples_.find(nodeId);
    if (it != samples_.end() && it->second.valid) {
        return it->second.clockOffsetMs;
    }
    return 0;
}

uint32_t NodeClockTracker::GetRoundTripTimeMs(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(trackerMutex_);
    auto it = samples_.find(nodeId);
    if (it != samples_.end() && it->second.valid) {
        return it->second.roundTripTimeMs;
    }
    return 0;
}

bool NodeClockTracker::IsClockDriftDetected(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(trackerMutex_);
    auto it = samples_.find(nodeId);
    if (it != samples_.end() && it->second.valid) {
        return std::abs(it->second.clockOffsetMs) > static_cast<int64_t>(driftThresholdMs_);
    }
    return false;
}

bool NodeClockTracker::IsNodeSynchronized(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(trackerMutex_);
    auto it = samples_.find(nodeId);
    if (it != samples_.end() && it->second.valid) {
        return std::abs(it->second.clockOffsetMs) <= static_cast<int64_t>(driftThresholdMs_);
    }
    return false;
}

void NodeClockTracker::SetDriftThresholdMs(uint32_t thresholdMs) {
    std::lock_guard<std::mutex> lock(trackerMutex_);
    driftThresholdMs_ = thresholdMs;
}

uint32_t NodeClockTracker::GetDriftThresholdMs() const {
    std::lock_guard<std::mutex> lock(trackerMutex_);
    return driftThresholdMs_;
}

void NodeClockTracker::Reset() {
    std::lock_guard<std::mutex> lock(trackerMutex_);
    samples_.clear();
}

} // namespace aegis::coordinator::orchestration

