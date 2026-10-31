/**
 * @file ReliableChannel.hpp
 * @brief Communication reliability engine providing deduplication, sequence ordering, and bounded queues.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "common/protocol/Packet.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <mutex>
#include <cstdint>
#include <functional>

namespace aegis::production {

enum class QueueOverflowPolicy {
    DROP_OLDEST,
    REJECT_NEW
};

struct ReliableChannelStats {
    uint64_t packetsTx{0};
    uint64_t packetsRx{0};
    uint64_t duplicatesDetected{0};
    uint64_t outOfOrderDetected{0};
    uint64_t queueOverflowDrops{0};
    uint64_t retriesAttempted{0};
};

struct QueuedMessage {
    uint64_t sequenceNumber{0};
    protocol::Packet packet;
    uint32_t attempt{0};
    uint32_t maxRetries{3};
    uint64_t nextRetryMonoMs{0};
    uint32_t retryDelayMs{200};
};

class ReliableChannel {
public:
    explicit ReliableChannel(size_t maxQueueSize = 1000, QueueOverflowPolicy policy = QueueOverflowPolicy::DROP_OLDEST);
    ~ReliableChannel() = default;

    /**
     * @brief Enqueue an outbound packet for reliable delivery.
     * @return Assigned monotonic sequence number, or 0 if rejected due to queue overflow.
     */
    uint64_t EnqueueOutbound(const protocol::Packet& packet, uint32_t maxRetries = 3, uint32_t baseRetryMs = 200);

    /**
     * @brief Check whether an incoming sequence number is valid or a duplicate.
     * @return true if packet is novel and accepted; false if duplicate or invalid.
     */
    bool IngestInbound(const std::string& senderId, uint64_t sequenceNumber);

    /**
     * @brief Acknowledge receipt of an outbound packet by sequence number.
     */
    bool Acknowledge(uint64_t sequenceNumber);

    /**
     * @brief Process retry timeouts for pending unacknowledged packets.
     * @return List of packets requiring retransmission.
     */
    std::vector<QueuedMessage> ProcessRetries();

    ReliableChannelStats GetStats() const;
    size_t GetPendingAckCount() const;
    void Reset();

    std::string ToJson() const;

private:
    mutable std::mutex mutex_;
    size_t maxQueueSize_{1000};
    QueueOverflowPolicy policy_{QueueOverflowPolicy::DROP_OLDEST};
    uint64_t nextOutSequence_{1};

    // Sender -> Highest received sequence number
    std::unordered_map<std::string, uint64_t> lastReceivedSequence_;
    // Sender -> Recently seen sequence numbers (within sliding window)
    std::unordered_map<std::string, std::unordered_set<uint64_t>> seenSequences_;

    // Pending ACK table: sequenceNumber -> QueuedMessage
    std::unordered_map<uint64_t, QueuedMessage> pendingAck_;

    ReliableChannelStats stats_;
};

} // namespace aegis::production

