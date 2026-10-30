/**
 * @file message.hpp
 * @brief QNX Network Communication Layer - Structured Message Protocol.
 *
 * Implements packet framing, wire serialization/deserialization, and CRC16
 * checksum verification for inter-node communication between Central Coordinator
 * and edge Traffic Nodes on Raspberry Pi 4 running QNX SDP 8.0.
 */

#ifndef AEGIS_NETWORK_MESSAGE_HPP
#define AEGIS_NETWORK_MESSAGE_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

namespace aegis {
namespace network {

/**
 * @brief Supported communication message types in SMART AEGIS QNX Network Layer.
 */
enum class MessageType : uint8_t {
    UNKNOWN       = 0,
    REGISTER      = 1,  ///< Node registration with Coordinator
    HEARTBEAT     = 2,  ///< 1-second liveness keepalive ping
    STATUS        = 3,  ///< Node health and operational metrics
    TRAFFIC_STATE = 4,  ///< Signal phase / queue occupancy telemetry
    COMMAND       = 5,  ///< Preemption corridor / phase control command
    ACK           = 6,  ///< Message receipt acknowledgement
    ERROR         = 7   ///< Alert and fault notification
};

/**
 * @brief Convert MessageType enum to human-readable string.
 */
const char* MessageTypeToString(MessageType type) noexcept;

/**
 * @brief Convert string representation to MessageType enum.
 */
MessageType StringToMessageType(const std::string& str) noexcept;

/**
 * @brief Lightweight packet structure with wire framing and CRC16 checksum.
 *
 * Wire Framing Format:
 * [Magic Bytes: 2B (0xA5, 0x5A)]
 * [Message Type: 1B]
 * [Timestamp: 8B Big-Endian (epoch milliseconds)]
 * [Node ID Length: 2B Big-Endian]
 * [Payload Length: 4B Big-Endian]
 * [Checksum: 2B Big-Endian (CRC16-CCITT)]
 * [Node ID: N bytes]
 * [Payload: M bytes]
 *
 * Fixed Header Size = 19 bytes. Total packet length = 19 + N + M.
 */
class Message {
public:
    static constexpr uint8_t MAGIC_BYTE_0 = 0xA5;
    static constexpr uint8_t MAGIC_BYTE_1 = 0x5A;
    static constexpr size_t FIXED_HEADER_SIZE = 19;
    static constexpr size_t MAX_PACKET_SIZE = 10 * 1024 * 1024; // 10 MB sanity cap

    Message();
    Message(MessageType type,
            std::string nodeId,
            std::string payload = "",
            uint64_t timestamp = 0);

    ~Message() = default;
    Message(const Message&) = default;
    Message& operator=(const Message&) = default;
    Message(Message&&) noexcept = default;
    Message& operator=(Message&&) noexcept = default;

    // Getters
    MessageType GetType() const noexcept { return type_; }
    MessageType get_type() const noexcept { return type_; }

    const std::string& GetNodeId() const noexcept { return nodeId_; }
    const std::string& get_node_id() const noexcept { return nodeId_; }

    const std::string& GetPayload() const noexcept { return payload_; }
    const std::string& get_payload() const noexcept { return payload_; }

    uint64_t GetTimestamp() const noexcept { return timestamp_; }
    uint64_t get_timestamp() const noexcept { return timestamp_; }

    uint32_t GetPayloadLength() const noexcept {
        return static_cast<uint32_t>(payload_.size());
    }
    uint32_t get_payload_length() const noexcept {
        return static_cast<uint32_t>(payload_.size());
    }

    uint16_t GetChecksum() const noexcept { return checksum_; }
    uint16_t get_checksum() const noexcept { return checksum_; }

    // Setters
    void SetType(MessageType type) noexcept { type_ = type; }
    void set_type(MessageType type) noexcept { type_ = type; }

    void SetNodeId(const std::string& nodeId) { nodeId_ = nodeId; }
    void set_node_id(const std::string& nodeId) { nodeId_ = nodeId; }

    void SetPayload(const std::string& payload) { payload_ = payload; }
    void set_payload(const std::string& payload) { payload_ = payload; }

    void SetTimestamp(uint64_t timestamp) noexcept { timestamp_ = timestamp; }
    void set_timestamp(uint64_t timestamp) noexcept { timestamp_ = timestamp; }

    void SetChecksum(uint16_t checksum) noexcept { checksum_ = checksum; }
    void set_checksum(uint16_t checksum) noexcept { checksum_ = checksum; }

    // Serialization & Deserialization
    std::vector<uint8_t> Serialize() const;
    std::vector<uint8_t> serialize() const { return Serialize(); }

    static bool Deserialize(const uint8_t* data, size_t length, Message& outMsg, size_t& bytesConsumed);
    static bool Deserialize(const std::vector<uint8_t>& buffer, Message& outMsg, size_t& bytesConsumed);
    static bool deserialize(const std::vector<uint8_t>& buffer, Message& outMsg, size_t& bytesConsumed) {
        return Deserialize(buffer, outMsg, bytesConsumed);
    }

    // Checksum & Validation
    uint16_t ComputeChecksum() const;
    uint16_t compute_checksum() const { return ComputeChecksum(); }

    bool VerifyChecksum() const;
    bool verify_checksum() const { return VerifyChecksum(); }

    bool IsValid() const;
    bool is_valid() const { return IsValid(); }

    // Display & Logging
    std::string ToString() const;
    std::string to_string() const { return ToString(); }

    std::string GetFormattedLog() const;
    std::string get_formatted_log() const { return GetFormattedLog(); }

    // Convenience Factory Methods
    static Message CreateRegister(const std::string& nodeId, const std::string& payload = "");
    static Message CreateHeartbeat(const std::string& nodeId);
    static Message CreateStatus(const std::string& nodeId, const std::string& statusPayload);
    static Message CreateTrafficState(const std::string& nodeId, const std::string& statePayload);
    static Message CreateCommand(const std::string& nodeId, const std::string& commandPayload);
    static Message CreateAck(const std::string& nodeId, const std::string& ackPayload = "OK");
    static Message CreateError(const std::string& nodeId, const std::string& errorPayload);

    // CRC16 Calculation Helper (Polynomial 0x1021)
    static uint16_t CalculateCRC16(const uint8_t* data, size_t length) noexcept;

private:
    static uint64_t GetCurrentTimeMs() noexcept;

    MessageType type_{MessageType::UNKNOWN};
    std::string nodeId_{};
    std::string payload_{};
    uint64_t timestamp_{0};
    uint16_t checksum_{0};
};

} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_MESSAGE_HPP

