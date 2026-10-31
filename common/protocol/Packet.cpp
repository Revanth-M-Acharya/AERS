/**
 * @file Packet.cpp
 * @brief Implementation of binary packet framing and serialization.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "common/protocol/Packet.hpp"
#include "common/utils/TimeUtils.hpp"
#include <cstring>
#include <algorithm>

namespace aegis::protocol {

namespace {

// Standard CRC16-CCITT polynomial 0x1021
uint16_t CalculateCRC16(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= static_cast<uint16_t>(data[i] << 8);
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x8000) {
                crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
            } else {
                crc = static_cast<uint16_t>(crc << 1);
            }
        }
    }
    return crc;
}

void WriteUint16BE(uint8_t* dest, uint16_t val) {
    dest[0] = static_cast<uint8_t>((val >> 8) & 0xFF);
    dest[1] = static_cast<uint8_t>(val & 0xFF);
}

void WriteUint32BE(uint8_t* dest, uint32_t val) {
    dest[0] = static_cast<uint8_t>((val >> 24) & 0xFF);
    dest[1] = static_cast<uint8_t>((val >> 16) & 0xFF);
    dest[2] = static_cast<uint8_t>((val >> 8) & 0xFF);
    dest[3] = static_cast<uint8_t>(val & 0xFF);
}

void WriteUint64BE(uint8_t* dest, uint64_t val) {
    for (int i = 7; i >= 0; --i) {
        dest[i] = static_cast<uint8_t>(val & 0xFF);
        val >>= 8;
    }
}

uint16_t ReadUint16BE(const uint8_t* src) {
    return static_cast<uint16_t>((static_cast<uint16_t>(src[0]) << 8) |
                                 static_cast<uint16_t>(src[1]));
}

uint32_t ReadUint32BE(const uint8_t* src) {
    return (static_cast<uint32_t>(src[0]) << 24) |
           (static_cast<uint32_t>(src[1]) << 16) |
           (static_cast<uint32_t>(src[2]) << 8)  |
           (static_cast<uint32_t>(src[3]));
}

uint64_t ReadUint64BE(const uint8_t* src) {
    uint64_t val = 0;
    for (int i = 0; i < 8; ++i) {
        val = (val << 8) | static_cast<uint64_t>(src[i]);
    }
    return val;
}

} // namespace

Packet::Packet()
    : type_(MessageType::UNKNOWN),
      nodeId_(""),
      timestamp_(common::TimeUtils::GetCurrentTimestampMs()),
      payload_("") {}

Packet::Packet(MessageType type, const std::string& nodeId, const std::string& payload, uint64_t timestamp)
    : type_(type),
      nodeId_(nodeId.substr(0, NODE_ID_MAX_LEN - 1)),
      timestamp_(timestamp == 0 ? common::TimeUtils::GetCurrentTimestampMs() : timestamp),
      payload_(payload) {}

void Packet::SetNodeId(const std::string& nodeId) {
    nodeId_ = nodeId.substr(0, NODE_ID_MAX_LEN - 1);
}

uint16_t Packet::ComputeCRC16(const uint8_t* data, size_t length) {
    return CalculateCRC16(data, length);
}

std::vector<uint8_t> Packet::Serialize() const {
    uint32_t payloadLen = static_cast<uint32_t>(payload_.size());
    size_t totalSize = PACKET_HEADER_SIZE + payloadLen;
    std::vector<uint8_t> buffer(totalSize, 0);

    // Magic Bytes (0..1)
    buffer[0] = PACKET_MAGIC_0;
    buffer[1] = PACKET_MAGIC_1;

    // Message Type (2..3)
    WriteUint16BE(&buffer[2], static_cast<uint16_t>(type_));

    // Node ID (4..35) - 32 bytes fixed, null-padded
    size_t copyLen = std::min(nodeId_.size(), NODE_ID_MAX_LEN - 1);
    std::memcpy(&buffer[4], nodeId_.data(), copyLen);
    buffer[4 + copyLen] = '\0';

    // Timestamp (36..43)
    WriteUint64BE(&buffer[36], timestamp_);

    // Payload Length (44..47)
    WriteUint32BE(&buffer[44], payloadLen);

    // Copy Payload (50..N)
    if (payloadLen > 0) {
        std::memcpy(&buffer[PACKET_HEADER_SIZE], payload_.data(), payloadLen);
    }

    // Compute CRC16 over Header (bytes 0..47) + Payload (bytes 50..N)
    // We compute over entire packet except the 2 CRC bytes at 48..49.
    // Temporary vector for CRC input
    std::vector<uint8_t> crcInput;
    crcInput.reserve(48 + payloadLen);
    crcInput.insert(crcInput.end(), buffer.begin(), buffer.begin() + 48);
    if (payloadLen > 0) {
        crcInput.insert(crcInput.end(), buffer.begin() + static_cast<std::ptrdiff_t>(PACKET_HEADER_SIZE), buffer.end());
    }

    uint16_t crc = CalculateCRC16(crcInput.data(), crcInput.size());
    WriteUint16BE(&buffer[48], crc);

    return buffer;
}

bool Packet::Deserialize(const uint8_t* buffer, size_t length, Packet& outPacket, size_t& bytesConsumed) {
    bytesConsumed = 0;
    if (length < PACKET_HEADER_SIZE) {
        return false; // Incomplete header
    }

    // Scan for magic bytes to handle noise/framing offset
    size_t offset = 0;
    while (offset + 1 < length &&
           (buffer[offset] != PACKET_MAGIC_0 || buffer[offset + 1] != PACKET_MAGIC_1)) {
        ++offset;
    }

    if (offset > 0) {
        // Discard leading garbage before magic bytes
        bytesConsumed = offset;
        return false;
    }

    if (length < PACKET_HEADER_SIZE) {
        return false;
    }

    // Parse Header
    uint16_t typeRaw = ReadUint16BE(&buffer[2]);
    auto type = static_cast<MessageType>(typeRaw);

    char nodeBuf[NODE_ID_MAX_LEN + 1]{};
    std::memcpy(nodeBuf, &buffer[4], NODE_ID_MAX_LEN);
    nodeBuf[NODE_ID_MAX_LEN] = '\0';
    std::string nodeId(nodeBuf);

    uint64_t timestamp = ReadUint64BE(&buffer[36]);
    uint32_t payloadLen = ReadUint32BE(&buffer[44]);
    uint16_t expectedCrc = ReadUint16BE(&buffer[48]);

    // Check if entire packet body has arrived
    size_t fullPacketSize = PACKET_HEADER_SIZE + payloadLen;
    if (length < fullPacketSize) {
        return false; // Wait for remaining bytes in stream
    }

    // Verify CRC
    std::vector<uint8_t> crcInput;
    crcInput.reserve(48 + payloadLen);
    crcInput.insert(crcInput.end(), buffer, buffer + 48);
    if (payloadLen > 0) {
        crcInput.insert(crcInput.end(), buffer + PACKET_HEADER_SIZE, buffer + fullPacketSize);
    }

    uint16_t calculatedCrc = CalculateCRC16(crcInput.data(), crcInput.size());
    if (calculatedCrc != expectedCrc) {
        // Corrupted packet! Skip magic bytes and search next
        bytesConsumed = 2;
        return false;
    }

    // Extract payload
    std::string payload;
    if (payloadLen > 0) {
        payload.assign(reinterpret_cast<const char*>(buffer + PACKET_HEADER_SIZE), payloadLen);
    }

    outPacket.SetType(type);
    outPacket.SetNodeId(nodeId);
    outPacket.SetTimestamp(timestamp);
    outPacket.SetPayload(payload);

    bytesConsumed = fullPacketSize;
    return true;
}

} // namespace aegis::protocol

