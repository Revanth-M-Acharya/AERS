/**
 * @file AegisProtocol.hpp
 * @brief Formal TCP message framing, JSON format, commands, and CRC16 checksum for SMART AEGIS.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <cstdint>

namespace aegis::protocol {

namespace Commands {
    inline constexpr std::string_view INITIALIZE         = "INITIALIZE";
    inline constexpr std::string_view PING               = "PING";
    inline constexpr std::string_view HEALTH_CHECK       = "HEALTH_CHECK";
    inline constexpr std::string_view STATUS             = "STATUS";
    inline constexpr std::string_view BLUE_ON            = "BLUE_ON";
    inline constexpr std::string_view BLUE_OFF           = "BLUE_OFF";
    inline constexpr std::string_view RED_ON             = "RED_ON";
    inline constexpr std::string_view RED_OFF            = "RED_OFF";
    inline constexpr std::string_view RESET              = "RESET";
    inline constexpr std::string_view INTERSECTION_READY = "INTERSECTION_READY";
    inline constexpr std::string_view ACK                = "ACK";
    inline constexpr std::string_view ERROR              = "ERROR";
    inline constexpr std::string_view SHUTDOWN           = "SHUTDOWN";
}

struct AegisMessage {
    std::string timestamp;         ///< ISO 8601 UTC timestamp string
    int intersection{0};          ///< Target/Source intersection numerical ID (1, 2, or 3)
    std::string command;           ///< Command verb (e.g., BLUE_ON, ACK, PING)
    uint64_t sequence{0};          ///< Strictly monotonic sequence counter
    std::string status{"OK"};      ///< "OK", "ERROR", "BUSY"
    std::string details{""};       ///< Diagnostic text or error message
    uint16_t checksum{0};          ///< 16-bit CRC checksum

    std::string ToJson() const;
    static std::optional<AegisMessage> FromJson(std::string_view jsonStr);
    static uint16_t CalculateChecksum(std::string_view data);
    static std::string CurrentIsoTimestamp();
};

} // namespace aegis::protocol
