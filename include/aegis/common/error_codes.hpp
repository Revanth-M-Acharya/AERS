/**
 * @file error_codes.hpp
 * @brief Standardized 16-bit error codes for SMART AEGIS.
 */

#ifndef AEGIS_COMMON_ERROR_CODES_HPP
#define AEGIS_COMMON_ERROR_CODES_HPP

#include <cstdint>
#include <string_view>

namespace aegis {
namespace common {

enum class ErrorCode : uint16_t {
    SUCCESS = 0x0000,
    ERR_GPS_TIMEOUT = 0x0001,
    ERR_MQTT_DISCONNECTED = 0x0002,
    ERR_INVALID_MESSAGE = 0x0003,
    ERR_TRAFFIC_OFFLINE = 0x0004,
    ERR_VITALS_FAILURE = 0x0005,
    ERR_PREEMPT_REJECTED = 0x0006,
    ERR_AUTH_TOKEN_EXPIRED = 0x0007,
    ERR_REPLAY_ATTACK_DET = 0x0008,
    ERR_WDT_STARVATION = 0x0009,
    ERR_ROUTE_BLOCKED = 0x000A,
    ERR_DUAL_GREEN_ABORT = 0x000B,
    ERR_STORAGE_FULL = 0x000C
};

constexpr std::string_view ToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::SUCCESS: return "SUCCESS";
        case ErrorCode::ERR_GPS_TIMEOUT: return "ERR_001_GPS_TIMEOUT";
        case ErrorCode::ERR_MQTT_DISCONNECTED: return "ERR_002_MQTT_DISCONNECTED";
        case ErrorCode::ERR_INVALID_MESSAGE: return "ERR_003_INVALID_MESSAGE";
        case ErrorCode::ERR_TRAFFIC_OFFLINE: return "ERR_004_TRAFFIC_OFFLINE";
        case ErrorCode::ERR_VITALS_FAILURE: return "ERR_005_VITALS_FAILURE";
        case ErrorCode::ERR_PREEMPT_REJECTED: return "ERR_006_PREEMPT_REJECTED";
        case ErrorCode::ERR_AUTH_TOKEN_EXPIRED: return "ERR_007_AUTH_TOKEN_EXPIRED";
        case ErrorCode::ERR_REPLAY_ATTACK_DET: return "ERR_008_REPLAY_ATTACK_DET";
        case ErrorCode::ERR_WDT_STARVATION: return "ERR_009_WDT_STARVATION";
        case ErrorCode::ERR_ROUTE_BLOCKED: return "ERR_010_ROUTE_BLOCKED";
        case ErrorCode::ERR_DUAL_GREEN_ABORT: return "ERR_011_DUAL_GREEN_ABORT";
        case ErrorCode::ERR_STORAGE_FULL: return "ERR_012_STORAGE_FULL";
        default: return "UNKNOWN_ERROR";
    }
}

} // namespace common
} // namespace aegis

#endif // AEGIS_COMMON_ERROR_CODES_HPP
