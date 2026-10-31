/**
 * @file ProtocolTypes.hpp
 * @brief Wire protocol message types and status enums for SMART AEGIS.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <cstdint>
#include <string>

namespace aegis::protocol {

/**
 * @brief Strongly typed message enumeration as specified in SMART AEGIS protocol specification.
 */
enum class MessageType : uint16_t {
    UNKNOWN         = 0x0000,

    // Traffic Node -> Coordinator
    HELLO           = 0x0001,
    HEARTBEAT       = 0x0002,
    STATUS          = 0x0003,
    ACK             = 0x0004,

    // Coordinator -> Traffic Node
    GREEN           = 0x0010,
    YELLOW          = 0x0011,
    RED             = 0x0012,
    ALL_RED         = 0x0013,
    NORMAL_MODE     = 0x0020,
    EMERGENCY_MODE  = 0x0021,
    PING            = 0x0030
};

/**
 * @brief Convert MessageType enum to printable uppercase string.
 */
inline std::string MessageTypeToString(MessageType type) {
    switch (type) {
        case MessageType::HELLO:          return "HELLO";
        case MessageType::HEARTBEAT:      return "HEARTBEAT";
        case MessageType::STATUS:         return "STATUS";
        case MessageType::ACK:            return "ACK";
        case MessageType::GREEN:          return "GREEN";
        case MessageType::YELLOW:         return "YELLOW";
        case MessageType::RED:            return "RED";
        case MessageType::ALL_RED:        return "ALL_RED";
        case MessageType::NORMAL_MODE:    return "NORMAL_MODE";
        case MessageType::EMERGENCY_MODE: return "EMERGENCY_MODE";
        case MessageType::PING:           return "PING";
        case MessageType::UNKNOWN:
        default:                          return "UNKNOWN";
    }
}

/**
 * @brief Parse string into MessageType enum.
 */
inline MessageType StringToMessageType(const std::string& str) {
    if (str == "HELLO")          return MessageType::HELLO;
    if (str == "HEARTBEAT")      return MessageType::HEARTBEAT;
    if (str == "STATUS")         return MessageType::STATUS;
    if (str == "ACK")            return MessageType::ACK;
    if (str == "GREEN")          return MessageType::GREEN;
    if (str == "YELLOW")         return MessageType::YELLOW;
    if (str == "RED")            return MessageType::RED;
    if (str == "ALL_RED")        return MessageType::ALL_RED;
    if (str == "NORMAL_MODE")    return MessageType::NORMAL_MODE;
    if (str == "EMERGENCY_MODE") return MessageType::EMERGENCY_MODE;
    if (str == "PING")           return MessageType::PING;
    return MessageType::UNKNOWN;
}

} // namespace aegis::protocol

