/**
 * @file MessageSecurityGuard.hpp
 * @brief Message-level security guard enforcing sequence monotonicity, replay protection, and checksum integrity.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "common/protocol/Packet.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <cstdint>

namespace aegis::production {

enum class SecurityVerificationResult {
    PASSED,
    REPLAY_ATTACK_DETECTED,
    OUT_OF_ORDER_SEQUENCE,
    TIMESTAMP_SKEW_EXCEEDED,
    INTEGRITY_CHECK_FAILED
};

inline std::string SecurityResultToString(SecurityVerificationResult res) {
    switch (res) {
        case SecurityVerificationResult::PASSED:                  return "PASSED";
        case SecurityVerificationResult::REPLAY_ATTACK_DETECTED:  return "REPLAY_ATTACK_DETECTED";
        case SecurityVerificationResult::OUT_OF_ORDER_SEQUENCE:   return "OUT_OF_ORDER_SEQUENCE";
        case SecurityVerificationResult::TIMESTAMP_SKEW_EXCEEDED: return "TIMESTAMP_SKEW_EXCEEDED";
        case SecurityVerificationResult::INTEGRITY_CHECK_FAILED:  return "INTEGRITY_CHECK_FAILED";
        default:                                                  return "UNKNOWN";
    }
}

class MessageSecurityGuard {
public:
    explicit MessageSecurityGuard(uint64_t maxSkewMs = 5000);
    ~MessageSecurityGuard() = default;

    /**
     * @brief Verify incoming message security against replay, timestamp skew, and sequence integrity.
     */
    SecurityVerificationResult VerifyMessage(const std::string& senderId, uint64_t sequenceNumber, uint64_t packetTimestampMs, uint64_t currentSystemTimeMs = 0);

    uint64_t GetReplayAttacksDetected() const;
    uint64_t GetSkewViolations() const;
    void Reset();

private:
    mutable std::mutex mutex_;
    uint64_t maxSkewMs_{5000};
    std::unordered_map<std::string, uint64_t> lastSequence_;
    std::unordered_map<std::string, std::unordered_set<uint64_t>> seenSequences_;
    uint64_t replayAttacksDetected_{0};
    uint64_t skewViolations_{0};
};

} // namespace aegis::production

