/**
 * @file MessageSecurityGuard.cpp
 * @brief Implementation of message security guard.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "production/security/MessageSecurityGuard.hpp"
#include "common/utils/TimeUtils.hpp"

namespace aegis::production {

MessageSecurityGuard::MessageSecurityGuard(uint64_t maxSkewMs)
    : maxSkewMs_(maxSkewMs) {}

SecurityVerificationResult MessageSecurityGuard::VerifyMessage(
    const std::string& senderId,
    uint64_t sequenceNumber,
    uint64_t packetTimestampMs,
    uint64_t currentSystemTimeMs) {

    std::lock_guard<std::mutex> lock(mutex_);

    // 1. Timestamp skew check
    if (maxSkewMs_ > 0 && packetTimestampMs > 0) {
        uint64_t now = (currentSystemTimeMs != 0) ? currentSystemTimeMs : common::TimeUtils::GetCurrentTimestampMs();
        uint64_t diff = (now >= packetTimestampMs) ? (now - packetTimestampMs) : (packetTimestampMs - now);
        if (diff > maxSkewMs_) {
            skewViolations_++;
            return SecurityVerificationResult::TIMESTAMP_SKEW_EXCEEDED;
        }
    }

    // 2. Replay check
    auto& seen = seenSequences_[senderId];
    if (seen.find(sequenceNumber) != seen.end()) {
        replayAttacksDetected_++;
        return SecurityVerificationResult::REPLAY_ATTACK_DETECTED;
    }

    // 3. Monotonicity check
    auto lastIt = lastSequence_.find(senderId);
    if (lastIt != lastSequence_.end()) {
        if (sequenceNumber <= lastIt->second) {
            seen.insert(sequenceNumber);
            return SecurityVerificationResult::OUT_OF_ORDER_SEQUENCE;
        }
        lastIt->second = sequenceNumber;
    } else {
        lastSequence_[senderId] = sequenceNumber;
    }

    seen.insert(sequenceNumber);
    if (seen.size() > 500) {
        // Prune oldest sequence
        uint64_t minS = sequenceNumber;
        for (auto s : seen) {
            if (s < minS) minS = s;
        }
        seen.erase(minS);
    }

    return SecurityVerificationResult::PASSED;
}

uint64_t MessageSecurityGuard::GetReplayAttacksDetected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return replayAttacksDetected_;
}

uint64_t MessageSecurityGuard::GetSkewViolations() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return skewViolations_;
}

void MessageSecurityGuard::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    lastSequence_.clear();
    seenSequences_.clear();
    replayAttacksDetected_ = 0;
    skewViolations_ = 0;
}

} // namespace aegis::production

