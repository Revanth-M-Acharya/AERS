/**
 * @file Packet.hpp
 * @brief Binary packet framing and serialization for SMART AEGIS wire protocol.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "common/protocol/ProtocolTypes.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace aegis::protocol {

// 2-byte magic delimiter: 'A' (0x41), 'E' (0x45)
inline constexpr uint8_t PACKET_MAGIC_0 = 0x41;
inline constexpr uint8_t PACKET_MAGIC_1 = 0x45;
inline constexpr size_t NODE_ID_MAX_LEN = 32;
inline constexpr size_t PACKET_HEADER_SIZE = 50; // 2(magic) + 2(type) + 32(node) + 8(time) + 4(len) + 2(crc)

class Packet {
public:
    Packet();
    Packet(MessageType type, const std::string& nodeId, const std::string& payload = "", uint64_t timestamp = 0);
    ~Packet() = default;

    // Getters
    MessageType GetType() const { return type_; }
    const std::string& GetNodeId() const { return nodeId_; }
    uint64_t GetTimestamp() const { return timestamp_; }
    const std::string& GetPayload() const { return payload_; }
    size_t GetPayloadSize() const { return payload_.size(); }

    // Setters
    void SetType(MessageType type) { type_ = type; }
    void SetNodeId(const std::string& nodeId);
    void SetTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }
    void SetPayload(const std::string& payload) { payload_ = payload; }

    /**
     * @brief Serialize packet into wire-format byte vector.
     */
    std::vector<uint8_t> Serialize() const;

    /**
     * @brief Attempt to deserialize a packet from incoming raw byte stream.
     * @param buffer Raw buffer pointer.
     * @param length Number of bytes in buffer.
     * @param outPacket Result packet if successful.
     * @param bytesConsumed Number of bytes read/consumed from the buffer.
     * @return true if complete valid packet was extracted; false if partial or invalid.
     */
    static bool Deserialize(const uint8_t* buffer, size_t length, Packet& outPacket, size_t& bytesConsumed);

    /**
     * @brief Compute CRC16-CCITT checksum over specified data buffer.
     */
    static uint16_t ComputeCRC16(const uint8_t* data, size_t length);

private:
    MessageType type_{MessageType::UNKNOWN};
    std::string nodeId_;
    uint64_t timestamp_{0};
    std::string payload_;
};

} // namespace aegis::protocol

