/**
 * @file Message.cpp
 * @brief Implementation of packet serialization, framing, and checksum logic.
 */

#include "Message.h"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <algorithm>

namespace aegis {
namespace communication {

namespace {

// Byte writing helpers with explicit big-endian ordering
inline void WriteUint16BE(std::vector<uint8_t>& buf, uint16_t val) {
    buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
}

inline void WriteUint32BE(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
}

inline void WriteUint64BE(std::vector<uint8_t>& buf, uint64_t val) {
    for (int i = 7; i >= 0; --i) {
        buf.push_back(static_cast<uint8_t>((val >> (static_cast<unsigned>(i) * 8U)) & 0xFF));
    }
}

inline uint16_t ReadUint16BE(const uint8_t* data) noexcept {
    return static_cast<uint16_t>(
        (static_cast<uint16_t>(data[0]) << 8) |
        static_cast<uint16_t>(data[1])
    );
}

inline uint32_t ReadUint32BE(const uint8_t* data) noexcept {
    return (static_cast<uint32_t>(data[0]) << 24) |
           (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8)  |
           static_cast<uint32_t>(data[3]);
}

inline uint64_t ReadUint64BE(const uint8_t* data) noexcept {
    uint64_t val = 0;
    for (size_t i = 0; i < 8; ++i) {
        val = (val << 8) | static_cast<uint64_t>(data[i]);
    }
    return val;
}

// Simple JSON string escape
std::string EscapeJsonString(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 16);
    for (char c : input) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    std::ostringstream ss;
                    ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                    out += ss.str();
                } else {
                    out.push_back(c);
                }
                break;
        }
    }
    return out;
}

// Simple JSON string unescape
std::string UnescapeJsonString(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\\' && i + 1 < input.size()) {
            char next = input[++i];
            switch (next) {
                case '"':  out.push_back('"');  break;
                case '\\': out.push_back('\\'); break;
                case '/':  out.push_back('/');  break;
                case 'b':  out.push_back('\b'); break;
                case 'f':  out.push_back('\f'); break;
                case 'n':  out.push_back('\n'); break;
                case 'r':  out.push_back('\r'); break;
                case 't':  out.push_back('\t'); break;
                default:   out.push_back(next); break;
            }
        } else {
            out.push_back(input[i]);
        }
    }
    return out;
}

// Simple JSON key extractor helper
bool ExtractJsonKeyValue(const std::string& json, const std::string& key, std::string& outVal, bool isString) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return false;

    size_t colonPos = json.find(':', keyPos + searchKey.size());
    if (colonPos == std::string::npos) return false;

    size_t start = colonPos + 1;
    while (start < json.size() && (json[start] == ' ' || json[start] == '\t' || json[start] == '\r' || json[start] == '\n')) {
        start++;
    }
    if (start >= json.size()) return false;

    if (isString) {
        if (json[start] != '"') return false;
        start++;
        size_t end = start;
        bool inEscape = false;
        while (end < json.size()) {
            if (inEscape) {
                inEscape = false;
            } else if (json[end] == '\\') {
                inEscape = true;
            } else if (json[end] == '"') {
                break;
            }
            end++;
        }
        if (end >= json.size()) return false;
        outVal = UnescapeJsonString(json.substr(start, end - start));
        return true;
    } else {
        size_t end = start;
        while (end < json.size() && json[end] != ',' && json[end] != '}' && json[end] != ' ' && json[end] != '\n' && json[end] != '\r') {
            end++;
        }
        outVal = json.substr(start, end - start);
        return true;
    }
}

} // anonymous namespace

const char* MessageTypeToString(MessageType type) noexcept {
    switch (type) {
        case MessageType::REGISTER:      return "REGISTER";
        case MessageType::HEARTBEAT:     return "HEARTBEAT";
        case MessageType::SIGNAL_STATUS: return "SIGNAL_STATUS";
        case MessageType::COMMAND:       return "COMMAND";
        case MessageType::ACK:           return "ACK";
        case MessageType::ERROR:         return "ERROR";
        default:                         return "UNKNOWN";
    }
}

MessageType StringToMessageType(const std::string& str) noexcept {
    if (str == "REGISTER")      return MessageType::REGISTER;
    if (str == "HEARTBEAT")     return MessageType::HEARTBEAT;
    if (str == "SIGNAL_STATUS") return MessageType::SIGNAL_STATUS;
    if (str == "COMMAND")       return MessageType::COMMAND;
    if (str == "ACK")           return MessageType::ACK;
    if (str == "ERROR")         return MessageType::ERROR;
    return MessageType::ERROR;
}

uint64_t Message::CurrentTimestampMs() noexcept {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

uint16_t Message::ComputeCRC16(const uint8_t* data, size_t length) noexcept {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= static_cast<uint16_t>(static_cast<uint16_t>(data[i]) << 8);
        for (int j = 0; j < 8; ++j) {
            if ((crc & 0x8000) != 0) {
                crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
            } else {
                crc = static_cast<uint16_t>(crc << 1);
            }
        }
    }
    return crc;
}

Message::Message()
    : m_type(MessageType::HEARTBEAT)
    , m_senderId("")
    , m_timestamp(CurrentTimestampMs())
    , m_payload("") {
}

Message::Message(MessageType type,
                 std::string senderId,
                 std::string payload,
                 uint64_t timestamp)
    : m_type(type)
    , m_senderId(std::move(senderId))
    , m_timestamp(timestamp == 0 ? CurrentTimestampMs() : timestamp)
    , m_payload(std::move(payload)) {
}

std::vector<uint8_t> Message::Serialize() const {
    const uint16_t senderLen = static_cast<uint16_t>(std::min<size_t>(m_senderId.size(), 0xFFFF));
    const uint32_t payloadLen = static_cast<uint32_t>(m_payload.size());
    const uint32_t totalLength = static_cast<uint32_t>(HEADER_SIZE + senderLen + payloadLen);

    std::vector<uint8_t> buffer;
    buffer.reserve(totalLength);

    // 1. Magic bytes (2B)
    buffer.push_back(MAGIC_BYTE_0);
    buffer.push_back(MAGIC_BYTE_1);

    // 2. Total Packet Length (4B)
    WriteUint32BE(buffer, totalLength);

    // 3. Message Type (1B)
    buffer.push_back(static_cast<uint8_t>(m_type));

    // 4. Timestamp (8B)
    WriteUint64BE(buffer, m_timestamp);

    // 5. Sender ID Length (2B)
    WriteUint16BE(buffer, senderLen);

    // 6. Payload Length (4B)
    WriteUint32BE(buffer, payloadLen);

    // 7. Checksum (2B) placeholder
    size_t crcOffset = buffer.size();
    WriteUint16BE(buffer, 0x0000);

    // 8. Sender ID bytes
    if (senderLen > 0) {
        buffer.insert(buffer.end(), m_senderId.data(), m_senderId.data() + senderLen);
    }

    // 9. Payload bytes
    if (payloadLen > 0) {
        buffer.insert(buffer.end(), m_payload.data(), m_payload.data() + payloadLen);
    }

    // Calculate CRC over bytes 6 through end (excluding magic, totalLength, and CRC field itself)
    // Protected bytes: MessageType(1) + Timestamp(8) + SenderLen(2) + PayloadLen(4) + SenderID + Payload
    std::vector<uint8_t> crcInput;
    crcInput.reserve(15 + senderLen + payloadLen);
    crcInput.push_back(buffer[6]); // MessageType
    crcInput.insert(crcInput.end(), buffer.begin() + 7, buffer.begin() + 15); // Timestamp
    crcInput.insert(crcInput.end(), buffer.begin() + 15, buffer.begin() + 17); // SenderLen
    crcInput.insert(crcInput.end(), buffer.begin() + 17, buffer.begin() + 21); // PayloadLen
    if (senderLen > 0) {
        crcInput.insert(crcInput.end(), buffer.begin() + HEADER_SIZE, buffer.begin() + HEADER_SIZE + senderLen);
    }
    if (payloadLen > 0) {
        crcInput.insert(crcInput.end(), buffer.begin() + HEADER_SIZE + senderLen, buffer.end());
    }

    uint16_t checksum = ComputeCRC16(crcInput.data(), crcInput.size());
    buffer[crcOffset] = static_cast<uint8_t>((checksum >> 8) & 0xFF);
    buffer[crcOffset + 1] = static_cast<uint8_t>(checksum & 0xFF);

    return buffer;
}

bool Message::Deserialize(const uint8_t* data, size_t length, Message& outMsg) {
    if (data == nullptr || length < HEADER_SIZE || length > MAX_PACKET_SIZE) {
        return false;
    }

    // Check magic bytes
    if (data[0] != MAGIC_BYTE_0 || data[1] != MAGIC_BYTE_1) {
        return false;
    }

    uint32_t packetLen = ReadUint32BE(data + 2);
    if (packetLen != length) {
        return false;
    }

    uint8_t typeVal = data[6];
    uint64_t timestamp = ReadUint64BE(data + 7);
    uint16_t senderLen = ReadUint16BE(data + 15);
    uint32_t payloadLen = ReadUint32BE(data + 17);
    uint16_t rxChecksum = ReadUint16BE(data + 21);

    if (HEADER_SIZE + senderLen + payloadLen != length) {
        return false;
    }

    // Verify CRC
    std::vector<uint8_t> crcInput;
    crcInput.reserve(15 + senderLen + payloadLen);
    crcInput.push_back(typeVal);
    crcInput.insert(crcInput.end(), data + 7, data + 15);
    crcInput.insert(crcInput.end(), data + 15, data + 17);
    crcInput.insert(crcInput.end(), data + 17, data + 21);
    if (senderLen > 0) {
        crcInput.insert(crcInput.end(), data + HEADER_SIZE, data + HEADER_SIZE + senderLen);
    }
    if (payloadLen > 0) {
        crcInput.insert(crcInput.end(), data + HEADER_SIZE + senderLen, data + length);
    }

    uint16_t calculatedCrc = ComputeCRC16(crcInput.data(), crcInput.size());
    if (rxChecksum != calculatedCrc) {
        return false;
    }

    std::string senderId(reinterpret_cast<const char*>(data + HEADER_SIZE), senderLen);
    std::string payload(reinterpret_cast<const char*>(data + HEADER_SIZE + senderLen), payloadLen);

    outMsg.m_type = static_cast<MessageType>(typeVal);
    outMsg.m_timestamp = timestamp;
    outMsg.m_senderId = std::move(senderId);
    outMsg.m_payload = std::move(payload);

    return true;
}

bool Message::Deserialize(const std::vector<uint8_t>& buffer, Message& outMsg) {
    return Deserialize(buffer.data(), buffer.size(), outMsg);
}

bool Message::ParseNext(std::vector<uint8_t>& streamBuffer, Message& outMsg) {
    while (streamBuffer.size() >= 2) {
        // Find magic bytes
        if (streamBuffer[0] == MAGIC_BYTE_0 && streamBuffer[1] == MAGIC_BYTE_1) {
            break;
        }
        // Discard 1 byte to find alignment
        streamBuffer.erase(streamBuffer.begin());
    }

    if (streamBuffer.size() < 6) {
        return false; // Need at least magic + totalLength to know packet length
    }

    uint32_t totalLength = ReadUint32BE(streamBuffer.data() + 2);
    if (totalLength < HEADER_SIZE || totalLength > MAX_PACKET_SIZE) {
        // Corrupted packet length, discard magic bytes to re-sync
        streamBuffer.erase(streamBuffer.begin(), streamBuffer.begin() + 2);
        return false;
    }

    if (streamBuffer.size() < totalLength) {
        return false; // Incomplete packet, wait for further TCP fragments
    }

    bool success = Deserialize(streamBuffer.data(), totalLength, outMsg);
    if (success) {
        streamBuffer.erase(streamBuffer.begin(), streamBuffer.begin() + totalLength);
        return true;
    }

    // Packet checksum failed, discard magic to scan for next potential packet
    streamBuffer.erase(streamBuffer.begin(), streamBuffer.begin() + 2);
    return false;
}

std::string Message::ToJson() const {
    std::ostringstream ss;
    ss << "{"
       << "\"type\":\"" << MessageTypeToString(m_type) << "\","
       << "\"senderId\":\"" << EscapeJsonString(m_senderId) << "\","
       << "\"timestamp\":" << m_timestamp << ","
       << "\"payload\":\"" << EscapeJsonString(m_payload) << "\""
       << "}";
    return ss.str();
}

bool Message::FromJson(const std::string& jsonString, Message& outMsg) {
    std::string typeStr;
    std::string senderIdStr;
    std::string tsStr;
    std::string payloadStr;

    if (!ExtractJsonKeyValue(jsonString, "type", typeStr, true)) return false;
    if (!ExtractJsonKeyValue(jsonString, "senderId", senderIdStr, true)) return false;
    if (!ExtractJsonKeyValue(jsonString, "timestamp", tsStr, false)) return false;
    ExtractJsonKeyValue(jsonString, "payload", payloadStr, true); // payload can be empty

    outMsg.m_type = StringToMessageType(typeStr);
    outMsg.m_senderId = senderIdStr;
    try {
        outMsg.m_timestamp = std::stoull(tsStr);
    } catch (...) {
        outMsg.m_timestamp = CurrentTimestampMs();
    }
    outMsg.m_payload = payloadStr;
    return true;
}

std::string Message::ToString() const {
    std::ostringstream ss;
    ss << "[Message type=" << MessageTypeToString(m_type)
       << " sender=" << (m_senderId.empty() ? "(none)" : m_senderId)
       << " ts=" << m_timestamp
       << " payloadBytes=" << m_payload.size() << "]";
    return ss.str();
}

} // namespace communication
} // namespace aegis
