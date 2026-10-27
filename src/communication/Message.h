/**
 * @file Message.h
 * @brief Lightweight packet structure for AERS distributed communication.
 * 
 * Defines message types, metadata headers, and binary framing serialization
 * and deserialization for inter-node communication between Central Coordinator,
 * Ambulance OBU nodes, and Raspberry Pi intersection nodes.
 */

#ifndef AEGIS_COMMUNICATION_MESSAGE_H
#define AEGIS_COMMUNICATION_MESSAGE_H

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <optional>

namespace aegis {
namespace communication {

/**
 * @brief Supported communication message types in AERS.
 */
enum class MessageType : uint8_t {
    REGISTER      = 1,  ///< Node registration with Central Coordinator
    HEARTBEAT     = 2,  ///< Liveness beacon and keepalive ping
    SIGNAL_STATUS = 3,  ///< Intersection signal phase and queue status
    COMMAND       = 4,  ///< Control command (preemption trigger, phase override)
    ACK           = 5,  ///< Message receipt acknowledgement
    ERROR         = 6   ///< Error alert and fault notification
};

/**
 * @brief Convert MessageType enum to human-readable string.
 * @param type Message type enum.
 * @return String representation.
 */
const char* MessageTypeToString(MessageType type) noexcept;

/**
 * @brief Convert string name to MessageType enum.
 * @param str String representation (e.g., "REGISTER", "HEARTBEAT").
 * @return MessageType enum value or MessageType::ERROR if unknown.
 */
MessageType StringToMessageType(const std::string& str) noexcept;

/**
 * @brief Lightweight packet structure for network serialization.
 *
 * Wire Framing Format:
 * [Magic: 2B (0xAE, 0x47)]
 * [Total Packet Length: 4B Big-Endian]
 * [Message Type: 1B]
 * [Timestamp: 8B Big-Endian (epoch milliseconds)]
 * [Sender ID Length: 2B Big-Endian]
 * [Payload Length: 4B Big-Endian]
 * [Checksum: 2B Big-Endian (CRC-16-CCITT)]
 * [Sender ID: N bytes]
 * [Payload: M bytes]
 *
 * Fixed Header Size = 23 bytes.
 */
class Message {
public:
    static constexpr uint8_t MAGIC_BYTE_0 = 0xAE;
    static constexpr uint8_t MAGIC_BYTE_1 = 0x47;
    static constexpr size_t HEADER_SIZE = 23;
    static constexpr size_t MAX_PACKET_SIZE = 10 * 1024 * 1024; // 10 MB sanity cap

    /**
     * @brief Default constructor creating an empty HEARTBEAT message.
     */
    Message();

    /**
     * @brief Parameterized constructor.
     * @param type Message type.
     * @param senderId Identifier of the transmitting node (e.g. "COORD_01", "RSU_INT_04").
     * @param payload Application payload string or serialized JSON/binary.
     * @param timestamp Unix epoch timestamp in milliseconds (0 = auto-set to current UTC).
     */
    Message(MessageType type,
            std::string senderId,
            std::string payload = "",
            uint64_t timestamp = 0);

    // Default copy and move semantics
    Message(const Message&) = default;
    Message& operator=(const Message&) = default;
    Message(Message&&) noexcept = default;
    Message& operator=(Message&&) noexcept = default;
    ~Message() = default;

    // =========================================================================
    // Getters & Setters
    // =========================================================================

    MessageType GetType() const noexcept { return m_type; }
    void SetType(MessageType type) noexcept { m_type = type; }

    const std::string& GetSenderId() const noexcept { return m_senderId; }
    void SetSenderId(std::string senderId) { m_senderId = std::move(senderId); }

    uint64_t GetTimestamp() const noexcept { return m_timestamp; }
    void SetTimestamp(uint64_t timestamp) noexcept { m_timestamp = timestamp; }

    const std::string& GetPayload() const noexcept { return m_payload; }
    void SetPayload(std::string payload) { m_payload = std::move(payload); }

    // =========================================================================
    // Serialization & Deserialization
    // =========================================================================

    /**
     * @brief Serialize message to a framed binary buffer ready for TCP transmission.
     * @return Byte buffer containing framed packet with header, payload, and CRC.
     */
    std::vector<uint8_t> Serialize() const;

    /**
     * @brief Deserialize a single framed message from a binary buffer.
     * @param data Pointer to raw data.
     * @param length Number of bytes available.
     * @param outMsg Output message instance if successfully parsed.
     * @return true if packet is valid and parsed; false otherwise.
     */
    static bool Deserialize(const uint8_t* data, size_t length, Message& outMsg);

    /**
     * @brief Convenience overload for vector buffer.
     */
    static bool Deserialize(const std::vector<uint8_t>& buffer, Message& outMsg);

    /**
     * @brief Streaming stream parser: extracts next complete message from an accumulator buffer.
     *
     * Consumes valid packet bytes from the front of @p streamBuffer. Discards leading
     * unaligned bytes if magic bytes do not match. If a complete packet is not yet
     * received, leaves buffer intact and returns false.
     *
     * @param streamBuffer Accumulated TCP stream buffer.
     * @param outMsg Output message parsed.
     * @return true if a complete message was extracted and removed from buffer; false if more data is needed.
     */
    static bool ParseNext(std::vector<uint8_t>& streamBuffer, Message& outMsg);

    /**
     * @brief Serialize message metadata and payload to JSON string.
     * @return Formatted JSON string.
     */
    std::string ToJson() const;

    /**
     * @brief Deserialize message from JSON string.
     * @param jsonString Input JSON string.
     * @param outMsg Output message parsed.
     * @return true if successfully parsed; false on malformed JSON.
     */
    static bool FromJson(const std::string& jsonString, Message& outMsg);

    /**
     * @brief Generate a concise diagnostic string representation.
     */
    std::string ToString() const;

    /**
     * @brief Compute CRC-16-CCITT checksum for data integrity verification.
     * @param data Pointer to input data.
     * @param length Length of data in bytes.
     * @return 16-bit CRC checksum.
     */
    static uint16_t ComputeCRC16(const uint8_t* data, size_t length) noexcept;

    /**
     * @brief Get current system clock time in milliseconds since epoch.
     */
    static uint64_t CurrentTimestampMs() noexcept;

private:
    MessageType m_type{MessageType::HEARTBEAT};
    std::string m_senderId{};
    uint64_t m_timestamp{0};
    std::string m_payload{};
};

} // namespace communication
} // namespace aegis

namespace communication = aegis::communication;

#endif // AEGIS_COMMUNICATION_MESSAGE_H
