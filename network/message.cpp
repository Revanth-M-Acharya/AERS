/**
 * @file message.cpp
 * @brief QNX Network Communication Layer - Structured Message Protocol Implementation.
 */

#include "message.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace aegis {
namespace network {

namespace {

inline void WriteUint16BE(uint8_t* dest, uint16_t val) noexcept {
    dest[0] = static_cast<uint8_t>((val >> 8) & 0xFF);
    dest[1] = static_cast<uint8_t>(val & 0xFF);
}

inline uint16_t ReadUint16BE(const uint8_t* src) noexcept {
    return static_cast<uint16_t>((static_cast<uint16_t>(src[0]) << 8) |
                                  static_cast<uint16_t>(src[1]));
}

inline void WriteUint32BE(uint8_t* dest, uint32_t val) noexcept {
    dest[0] = static_cast<uint8_t>((val >> 24) & 0xFF);
    dest[1] = static_cast<uint8_t>((val >> 16) & 0xFF);
    dest[2] = static_cast<uint8_t>((val >> 8) & 0xFF);
    dest[3] = static_cast<uint8_t>(val & 0xFF);
}

inline uint32_t ReadUint32BE(const uint8_t* src) noexcept {
    return (static_cast<uint32_t>(src[0]) << 24) |
           (static_cast<uint32_t>(src[1]) << 16) |
           (static_cast<uint32_t>(src[2]) << 8)  |
           static_cast<uint32_t>(src[3]);
}

inline void WriteUint64BE(uint8_t* dest, uint64_t val) noexcept {
    for (int i = 7; i >= 0; --i) {
        dest[7 - i] = static_cast<uint8_t>((val >> (i * 8)) & 0xFF);
    }
}

inline uint64_t ReadUint64BE(const uint8_t* src) noexcept {
    uint64_t val = 0;
    for (int i = 0; i < 8; ++i) {
        val = (val << 8) | static_cast<uint64_t>(src[i]);
    }
    return val;
}

} // anonymous namespace

const char* MessageTypeToString(MessageType type) noexcept {
    switch (type) {
        case MessageType::REGISTER:      return "REGISTER";
        case MessageType::HEARTBEAT:     return "HEARTBEAT";
        case MessageType::STATUS:        return "STATUS";
        case MessageType::TRAFFIC_STATE: return "TRAFFIC_STATE";
        case MessageType::COMMAND:       return "COMMAND";
        case MessageType::ACK:           return "ACK";
        case MessageType::ERROR:         return "ERROR";
        case MessageType::UNKNOWN:
        default:                         return "UNKNOWN";
    }
}

MessageType StringToMessageType(const std::string& str) noexcept {
    if (str == "REGISTER")      return MessageType::REGISTER;
    if (str == "HEARTBEAT")     return MessageType::HEARTBEAT;
    if (str == "STATUS")        return MessageType::STATUS;
    if (str == "TRAFFIC_STATE") return MessageType::TRAFFIC_STATE;
    if (str == "COMMAND")       return MessageType::COMMAND;
    if (str == "ACK")           return MessageType::ACK;
    if (str == "ERROR")         return MessageType::ERROR;
    return MessageType::UNKNOWN;
}

uint64_t Message::GetCurrentTimeMs() noexcept {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

uint16_t Message::CalculateCRC16(const uint8_t* data, size_t length) noexcept {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
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

Message::Message()
    : type_(MessageType::UNKNOWN),
      nodeId_(""),
      payload_(""),
      timestamp_(GetCurrentTimeMs()),
      checksum_(0) {
}

Message::Message(MessageType type,
                 std::string nodeId,
                 std::string payload,
                 uint64_t timestamp)
    : type_(type),
      nodeId_(std::move(nodeId)),
      payload_(std::move(payload)),
      timestamp_(timestamp == 0 ? GetCurrentTimeMs() : timestamp),
      checksum_(0) {
    checksum_ = ComputeChecksum();
}

uint16_t Message::ComputeChecksum() const {
    // Checksum covers Type (1B) + Timestamp (8B) + NodeID Length (2B) + Payload Length (4B) + NodeID + Payload
    const uint16_t nodeLen = static_cast<uint16_t>(nodeId_.size());
    const uint32_t payloadLen = static_cast<uint32_t>(payload_.size());
    const size_t bufferSize = 1 + 8 + 2 + 4 + nodeLen + payloadLen;

    std::vector<uint8_t> buffer(bufferSize);
    buffer[0] = static_cast<uint8_t>(type_);
    WriteUint64BE(&buffer[1], timestamp_);
    WriteUint16BE(&buffer[9], nodeLen);
    WriteUint32BE(&buffer[11], payloadLen);

    if (nodeLen > 0) {
        std::copy(nodeId_.begin(), nodeId_.end(), buffer.begin() + 15);
    }
    if (payloadLen > 0) {
        std::copy(payload_.begin(), payload_.end(), buffer.begin() + 15 + nodeLen);
    }

    return CalculateCRC16(buffer.data(), buffer.size());
}

bool Message::VerifyChecksum() const {
    return (checksum_ == ComputeChecksum());
}

bool Message::IsValid() const {
    if (type_ == MessageType::UNKNOWN) {
        return false;
    }
    if (nodeId_.empty()) {
        return false;
    }
    return VerifyChecksum();
}

std::vector<uint8_t> Message::Serialize() const {
    const uint16_t nodeLen = static_cast<uint16_t>(nodeId_.size());
    const uint32_t payloadLen = static_cast<uint32_t>(payload_.size());
    const uint16_t crc = ComputeChecksum();

    const size_t totalSize = FIXED_HEADER_SIZE + nodeLen + payloadLen;
    std::vector<uint8_t> buffer(totalSize);

    // [Magic Bytes: 2B (0xA5, 0x5A)]
    buffer[0] = MAGIC_BYTE_0;
    buffer[1] = MAGIC_BYTE_1;

    // [Message Type: 1B]
    buffer[2] = static_cast<uint8_t>(type_);

    // [Timestamp: 8B]
    WriteUint64BE(&buffer[3], timestamp_);

    // [Node ID Length: 2B]
    WriteUint16BE(&buffer[11], nodeLen);

    // [Payload Length: 4B]
    WriteUint32BE(&buffer[13], payloadLen);

    // [Checksum: 2B]
    WriteUint16BE(&buffer[17], crc);

    // [Node ID: N bytes]
    if (nodeLen > 0) {
        std::copy(nodeId_.begin(), nodeId_.end(), buffer.begin() + FIXED_HEADER_SIZE);
    }

    // [Payload: M bytes]
    if (payloadLen > 0) {
        std::copy(payload_.begin(), payload_.end(), buffer.begin() + FIXED_HEADER_SIZE + nodeLen);
    }

    return buffer;
}

bool Message::Deserialize(const uint8_t* data, size_t length, Message& outMsg, size_t& bytesConsumed) {
    bytesConsumed = 0;
    if (data == nullptr || length < FIXED_HEADER_SIZE) {
        return false;
    }

    // Scan for magic bytes
    size_t offset = 0;
    while (offset + 1 < length) {
        if (data[offset] == MAGIC_BYTE_0 && data[offset + 1] == MAGIC_BYTE_1) {
            break;
        }
        offset++;
    }

    if (offset > 0) {
        // Discard leading garbage before magic bytes
        bytesConsumed = offset;
        return false;
    }

    if (length < FIXED_HEADER_SIZE) {
        return false;
    }

    const uint8_t rawType = data[2];
    const uint64_t timestamp = ReadUint64BE(&data[3]);
    const uint16_t nodeLen = ReadUint16BE(&data[11]);
    const uint32_t payloadLen = ReadUint32BE(&data[13]);
    const uint16_t receivedCrc = ReadUint16BE(&data[17]);

    const size_t totalPacketLength = FIXED_HEADER_SIZE + nodeLen + payloadLen;
    if (totalPacketLength > MAX_PACKET_SIZE) {
        // Exceeds sanity size limit, consume magic bytes to discard corrupted frame
        bytesConsumed = 2;
        return false;
    }

    if (length < totalPacketLength) {
        // Incomplete packet in stream, wait for remaining bytes
        return false;
    }

    // Extract dynamic fields
    std::string nodeId(reinterpret_cast<const char*>(data + FIXED_HEADER_SIZE), nodeLen);
    std::string payload(reinterpret_cast<const char*>(data + FIXED_HEADER_SIZE + nodeLen), payloadLen);

    // Verify Checksum
    Message tempMsg;
    tempMsg.type_ = static_cast<MessageType>(rawType);
    tempMsg.timestamp_ = timestamp;
    tempMsg.nodeId_ = nodeId;
    tempMsg.payload_ = payload;

    const uint16_t calculatedCrc = tempMsg.ComputeChecksum();
    if (calculatedCrc != receivedCrc) {
        // Checksum mismatch, advance past magic bytes to resynchronize stream
        bytesConsumed = 2;
        return false;
    }

    // Successful deserialization
    outMsg.type_ = static_cast<MessageType>(rawType);
    outMsg.timestamp_ = timestamp;
    outMsg.nodeId_ = std::move(nodeId);
    outMsg.payload_ = std::move(payload);
    outMsg.checksum_ = receivedCrc;

    bytesConsumed = totalPacketLength;
    return true;
}

bool Message::Deserialize(const std::vector<uint8_t>& buffer, Message& outMsg, size_t& bytesConsumed) {
    return Deserialize(buffer.data(), buffer.size(), outMsg, bytesConsumed);
}

std::string Message::ToString() const {
    std::ostringstream ss;
    ss << "Message{type=" << MessageTypeToString(type_)
       << ", nodeId=\"" << nodeId_ << "\""
       << ", len=" << payload_.size()
       << ", timestamp=" << timestamp_
       << ", crc=0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << checksum_
       << "}";
    return ss.str();
}

std::string Message::GetFormattedLog() const {
    std::ostringstream ss;
    ss << "Type: " << MessageTypeToString(type_)
       << ", Node: " << nodeId_
       << ", Len: " << payload_.size()
       << ", Checksum: 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << checksum_;
    return ss.str();
}

Message Message::CreateRegister(const std::string& nodeId, const std::string& payload) {
    return Message(MessageType::REGISTER, nodeId, payload);
}

Message Message::CreateHeartbeat(const std::string& nodeId) {
    return Message(MessageType::HEARTBEAT, nodeId, "HEARTBEAT");
}

Message Message::CreateStatus(const std::string& nodeId, const std::string& statusPayload) {
    return Message(MessageType::STATUS, nodeId, statusPayload);
}

Message Message::CreateTrafficState(const std::string& nodeId, const std::string& statePayload) {
    return Message(MessageType::TRAFFIC_STATE, nodeId, statePayload);
}

Message Message::CreateCommand(const std::string& nodeId, const std::string& commandPayload) {
    return Message(MessageType::COMMAND, nodeId, commandPayload);
}

Message Message::CreateAck(const std::string& nodeId, const std::string& ackPayload) {
    return Message(MessageType::ACK, nodeId, ackPayload);
}

Message Message::CreateError(const std::string& nodeId, const std::string& errorPayload) {
    return Message(MessageType::ERROR, nodeId, errorPayload);
}

} // namespace network
} // namespace aegis

