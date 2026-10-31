/**
 * @file ReliableChannel.cpp
 * @brief Implementation of reliable communication channel.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "production/communication/ReliableChannel.hpp"
#include "common/utils/TimeUtils.hpp"
#include <sstream>

namespace aegis::production {

ReliableChannel::ReliableChannel(size_t maxQueueSize, QueueOverflowPolicy policy)
    : maxQueueSize_(maxQueueSize), policy_(policy) {}

uint64_t ReliableChannel::EnqueueOutbound(const protocol::Packet& packet, uint32_t maxRetries, uint32_t baseRetryMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (pendingAck_.size() >= maxQueueSize_) {
        stats_.queueOverflowDrops++;
        if (policy_ == QueueOverflowPolicy::REJECT_NEW) {
            return 0;
        }
        // DROP_OLDEST: Remove smallest sequence number
        auto oldestIt = pendingAck_.begin();
        for (auto it = pendingAck_.begin(); it != pendingAck_.end(); ++it) {
            if (it->first < oldestIt->first) {
                oldestIt = it;
            }
        }
        if (oldestIt != pendingAck_.end()) {
            pendingAck_.erase(oldestIt);
        }
    }

    uint64_t seq = nextOutSequence_++;
    QueuedMessage msg;
    msg.sequenceNumber = seq;
    msg.packet = packet;
    msg.attempt = 1;
    msg.maxRetries = maxRetries;
    msg.retryDelayMs = (baseRetryMs == 0) ? 200 : baseRetryMs;
    msg.nextRetryMonoMs = common::TimeUtils::GetMonotonicTimeMs() + msg.retryDelayMs;

    pendingAck_[seq] = msg;
    stats_.packetsTx++;
    return seq;
}

bool ReliableChannel::IngestInbound(const std::string& senderId, uint64_t sequenceNumber) {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.packetsRx++;

    auto& seen = seenSequences_[senderId];
    if (seen.find(sequenceNumber) != seen.end()) {
        stats_.duplicatesDetected++;
        return false; // Duplicate rejected
    }

    auto lastIt = lastReceivedSequence_.find(senderId);
    if (lastIt != lastReceivedSequence_.end()) {
        if (sequenceNumber < lastIt->second) {
            stats_.outOfOrderDetected++;
        } else {
            lastIt->second = sequenceNumber;
        }
    } else {
        lastReceivedSequence_[senderId] = sequenceNumber;
    }

    seen.insert(sequenceNumber);
    // Prune seen set window if exceeding 1000 items
    if (seen.size() > 1000) {
        uint64_t minSeq = sequenceNumber;
        for (auto s : seen) {
            if (s < minSeq) minSeq = s;
        }
        seen.erase(minSeq);
    }

    return true;
}

bool ReliableChannel::Acknowledge(uint64_t sequenceNumber) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = pendingAck_.find(sequenceNumber);
    if (it != pendingAck_.end()) {
        pendingAck_.erase(it);
        return true;
    }
    return false;
}

std::vector<QueuedMessage> ReliableChannel::ProcessRetries() {
    uint64_t now = common::TimeUtils::GetMonotonicTimeMs();
    std::vector<QueuedMessage> toRetry;

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = pendingAck_.begin();
    while (it != pendingAck_.end()) {
        if (now >= it->second.nextRetryMonoMs) {
            if (it->second.attempt < it->second.maxRetries) {
                it->second.attempt++;
                stats_.retriesAttempted++;
                toRetry.push_back(it->second);
                // Exponential backoff
                it->second.retryDelayMs *= 2;
                it->second.nextRetryMonoMs = now + it->second.retryDelayMs;
                ++it;
            } else {
                // Exceeded max retries: drop from pending
                it = pendingAck_.erase(it);
            }
        } else {
            ++it;
        }
    }

    return toRetry;
}

ReliableChannelStats ReliableChannel::GetStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

size_t ReliableChannel::GetPendingAckCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pendingAck_.size();
}

void ReliableChannel::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    nextOutSequence_ = 1;
    lastReceivedSequence_.clear();
    seenSequences_.clear();
    pendingAck_.clear();
    stats_ = ReliableChannelStats{};
}

std::string ReliableChannel::ToJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream ss;
    ss << "{\n"
       << "  \"packets_tx\": " << stats_.packetsTx << ",\n"
       << "  \"packets_rx\": " << stats_.packetsRx << ",\n"
       << "  \"duplicates_detected\": " << stats_.duplicatesDetected << ",\n"
       << "  \"out_of_order_detected\": " << stats_.outOfOrderDetected << ",\n"
       << "  \"queue_overflow_drops\": " << stats_.queueOverflowDrops << ",\n"
       << "  \"retries_attempted\": " << stats_.retriesAttempted << ",\n"
       << "  \"pending_ack_count\": " << pendingAck_.size() << "\n"
       << "}";
    return ss.str();
}

} // namespace aegis::production

