/**
 * @file AegisProtocol.cpp
 * @brief Implementation of AegisProtocol message serialization and CRC16.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "protocol/AegisProtocol.hpp"
#include "aegis/common/json_helper.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace aegis::protocol {

// Standard CRC-16-CCITT (polynomial 0x1021)
uint16_t AegisMessage::CalculateChecksum(std::string_view data) {
    uint16_t crc = 0xFFFF;
    for (char c : data) {
        crc ^= static_cast<uint16_t>(static_cast<uint8_t>(c) << 8);
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x8000) {
                crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
            } else {
                crc = static_cast<uint16_t>(crc << 1);
            }
        }
    }
    return crc;
}

std::string AegisMessage::CurrentIsoTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuf{};
#if defined(_WIN32)
    gmtime_s(&tmBuf, &tt);
#else
    gmtime_r(&tt, &tmBuf);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tmBuf, "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

std::string AegisMessage::ToJson() const {
    std::ostringstream ss;
    ss << "{"
       << "\"timestamp\":\"" << (timestamp.empty() ? CurrentIsoTimestamp() : timestamp) << "\","
       << "\"intersection\":" << intersection << ","
       << "\"command\":\"" << command << "\","
       << "\"sequence\":" << sequence << ","
       << "\"status\":\"" << status << "\"";

    if (!details.empty()) {
        ss << ",\"details\":\"" << common::JsonHelper::EscapeString(details) << "\"";
    }

    // Compute checksum over the payload preceding the checksum field
    std::string preliminary = ss.str();
    uint16_t computed = CalculateChecksum(preliminary);
    ss << ",\"checksum\":" << (checksum != 0 ? checksum : computed) << "}\n";
    return ss.str();
}

std::optional<AegisMessage> AegisMessage::FromJson(std::string_view jsonStr) {
    if (jsonStr.empty()) return std::nullopt;

    AegisMessage msg;
    auto ts = common::JsonHelper::ExtractString(jsonStr, "timestamp");
    if (ts.has_value()) msg.timestamp = *ts;

    auto inter = common::JsonHelper::ExtractInt64(jsonStr, "intersection");
    if (!inter.has_value()) return std::nullopt;
    msg.intersection = static_cast<int>(*inter);

    auto cmd = common::JsonHelper::ExtractString(jsonStr, "command");
    if (!cmd.has_value()) return std::nullopt;
    msg.command = *cmd;

    auto seq = common::JsonHelper::ExtractInt64(jsonStr, "sequence");
    msg.sequence = seq.has_value() ? static_cast<uint64_t>(*seq) : 0;

    auto st = common::JsonHelper::ExtractString(jsonStr, "status");
    msg.status = st.value_or("OK");

    auto det = common::JsonHelper::ExtractString(jsonStr, "details");
    if (det.has_value()) msg.details = *det;

    auto chk = common::JsonHelper::ExtractInt64(jsonStr, "checksum");
    msg.checksum = chk.has_value() ? static_cast<uint16_t>(*chk) : 0;

    return msg;
}

} // namespace aegis::protocol
