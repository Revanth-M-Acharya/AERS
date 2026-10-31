/**
 * @file PacketValidator.cpp
 * @brief Implementation of packet validation rules.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "production/communication/PacketValidator.hpp"
#include "common/utils/TimeUtils.hpp"
#include <cmath>

namespace aegis::production {

PacketValidator::PacketValidator(uint64_t maxTimestampSkewMs)
    : maxTimestampSkewMs_(maxTimestampSkewMs) {}

PacketValidationResult PacketValidator::Validate(const protocol::Packet& packet, uint64_t currentSystemTimeMs) const {
    if (packet.GetType() == protocol::MessageType::UNKNOWN) {
        return PacketValidationResult::UNKNOWN_MESSAGE_TYPE;
    }

    if (packet.GetNodeId().empty() || packet.GetNodeId().size() > protocol::NODE_ID_MAX_LEN) {
        return PacketValidationResult::INVALID_NODE_ID;
    }

    if (packet.GetPayloadSize() > 65535) {
        return PacketValidationResult::INVALID_PAYLOAD_LENGTH;
    }

    if (maxTimestampSkewMs_ > 0) {
        uint64_t now = (currentSystemTimeMs != 0) ? currentSystemTimeMs : common::TimeUtils::GetCurrentTimestampMs();
        uint64_t pktTime = packet.GetTimestamp();
        if (pktTime > 0) {
            uint64_t diff = (now >= pktTime) ? (now - pktTime) : (pktTime - now);
            if (diff > maxTimestampSkewMs_) {
                return PacketValidationResult::TIMESTAMP_SKEW_EXCESSIVE;
            }
        }
    }

    return PacketValidationResult::VALID;
}

PacketValidationResult PacketValidator::ValidateRawBuffer(const uint8_t* buffer, size_t length) const {
    if (!buffer || length < protocol::PACKET_HEADER_SIZE) {
        return PacketValidationResult::INVALID_PAYLOAD_LENGTH;
    }

    if (buffer[0] != protocol::PACKET_MAGIC_0 || buffer[1] != protocol::PACKET_MAGIC_1) {
        return PacketValidationResult::INVALID_MAGIC;
    }

    return PacketValidationResult::VALID;
}

void PacketValidator::SetMaxTimestampSkewMs(uint64_t skewMs) {
    maxTimestampSkewMs_ = skewMs;
}

uint64_t PacketValidator::GetMaxTimestampSkewMs() const {
    return maxTimestampSkewMs_;
}

} // namespace aegis::production

