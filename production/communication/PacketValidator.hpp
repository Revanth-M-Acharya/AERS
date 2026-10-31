/**
 * @file PacketValidator.hpp
 * @brief Wire packet integrity, format, and timing validation.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "common/protocol/Packet.hpp"
#include <string>
#include <cstdint>

namespace aegis::production {

enum class PacketValidationResult {
    VALID,
    INVALID_MAGIC,
    INVALID_CRC,
    INVALID_NODE_ID,
    INVALID_PAYLOAD_LENGTH,
    TIMESTAMP_SKEW_EXCESSIVE,
    UNKNOWN_MESSAGE_TYPE
};

inline std::string ValidationResultToString(PacketValidationResult res) {
    switch (res) {
        case PacketValidationResult::VALID:                    return "VALID";
        case PacketValidationResult::INVALID_MAGIC:            return "INVALID_MAGIC";
        case PacketValidationResult::INVALID_CRC:              return "INVALID_CRC";
        case PacketValidationResult::INVALID_NODE_ID:          return "INVALID_NODE_ID";
        case PacketValidationResult::INVALID_PAYLOAD_LENGTH:   return "INVALID_PAYLOAD_LENGTH";
        case PacketValidationResult::TIMESTAMP_SKEW_EXCESSIVE: return "TIMESTAMP_SKEW_EXCESSIVE";
        case PacketValidationResult::UNKNOWN_MESSAGE_TYPE:     return "UNKNOWN_MESSAGE_TYPE";
        default:                                               return "UNKNOWN";
    }
}

class PacketValidator {
public:
    explicit PacketValidator(uint64_t maxTimestampSkewMs = 10000);
    ~PacketValidator() = default;

    /**
     * @brief Validate an incoming protocol packet.
     */
    PacketValidationResult Validate(const protocol::Packet& packet, uint64_t currentSystemTimeMs = 0) const;

    /**
     * @brief Validate raw byte buffer framing before deserialization.
     */
    PacketValidationResult ValidateRawBuffer(const uint8_t* buffer, size_t length) const;

    void SetMaxTimestampSkewMs(uint64_t skewMs);
    uint64_t GetMaxTimestampSkewMs() const;

private:
    uint64_t maxTimestampSkewMs_{10000};
};

} // namespace aegis::production

