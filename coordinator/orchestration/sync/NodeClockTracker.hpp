/**
 * @file NodeClockTracker.hpp
 * @brief Tracks distributed node clock offsets, network latency, and clock drift.
 *
 * Implements software-level 4-timestamp synchronization (using standard NTP/PTP-style
 * offset and delay estimation formulas: Offset = ((t2 - t1) + (t3 - t4)) / 2, RTT = (t4 - t1) - (t3 - t2))
 * as well as 2-way application ping-pong time tracking over monotonic clocks.
 * Note: This provides application-layer distributed timeline tracking and drift detection;
 * it is not a hardware IEEE 1588 PTP physical-layer PHY timestamping driver.
 *
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/orchestration/state/CoordinatorStateManager.hpp"
#include "coordinator/orchestration/event_bus/IEventBus.hpp"
#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <cstdint>

namespace aegis::coordinator::orchestration {

struct ClockSyncSample {
    std::string nodeId;
    int64_t clockOffsetMs{0};
    uint32_t roundTripTimeMs{0};
    uint64_t sampleTimeMonoMs{0};
    bool valid{false};
};

class NodeClockTracker {
public:
    explicit NodeClockTracker(std::shared_ptr<CoordinatorStateManager> stateManager = nullptr,
                              std::shared_ptr<IEventBus> eventBus = nullptr,
                              uint32_t driftThresholdMs = 500);
    ~NodeClockTracker() = default;

    /**
     * @brief Record 4-timestamp PTP/NTP style sync sample.
     * @param nodeId Traffic Node identifier.
     * @param t1ClientSend Client transmit time (remote clock).
     * @param t2ServerRecv Server receive time (local clock).
     * @param t3ServerSend Server response transmit time (local clock).
     * @param t4ClientRecv Client response receive time (remote clock).
     */
    void RecordPtpSample(const std::string& nodeId,
                         uint64_t t1ClientSend,
                         uint64_t t2ServerRecv,
                         uint64_t t3ServerSend,
                         uint64_t t4ClientRecv);

    /**
     * @brief Simplified 2-way ping-pong sync sample from Coordinator perspective.
     * @param nodeId Node identifier.
     * @param pingSendMonoMs Time coordinator sent ping.
     * @param ackRecvMonoMs Time coordinator received ack.
     * @param nodeReportedTimestampMs Remote timestamp reported in packet payload.
     */
    void RecordPingSample(const std::string& nodeId,
                          uint64_t pingSendMonoMs,
                          uint64_t ackRecvMonoMs,
                          uint64_t nodeReportedTimestampMs = 0);

    int64_t GetClockOffsetMs(const std::string& nodeId) const;
    uint32_t GetRoundTripTimeMs(const std::string& nodeId) const;
    bool IsClockDriftDetected(const std::string& nodeId) const;
    bool IsNodeSynchronized(const std::string& nodeId) const;

    void SetDriftThresholdMs(uint32_t thresholdMs);
    uint32_t GetDriftThresholdMs() const;

    void Reset();

private:
    std::shared_ptr<CoordinatorStateManager> stateManager_;
    std::shared_ptr<IEventBus> eventBus_;
    uint32_t driftThresholdMs_{500};

    mutable std::mutex trackerMutex_;
    std::unordered_map<std::string, ClockSyncSample> samples_;
};

} // namespace aegis::coordinator::orchestration

