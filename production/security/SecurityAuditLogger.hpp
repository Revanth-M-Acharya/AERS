/**
 * @file SecurityAuditLogger.hpp
 * @brief Tamper-evident structured security audit logger for SMART AEGIS.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

namespace aegis::production {

enum class SecurityEventType {
    AUTH_SUCCESS,
    AUTH_FAILURE,
    REPLAY_ATTACK_DETECTED,
    TIMESTAMP_SKEW_DETECTED,
    UNAUTHORIZED_NODE_REJECTED,
    CONFIG_TAMPERING_DETECTED,
    PRIVILEGE_VIOLATION
};

inline std::string SecurityEventTypeToString(SecurityEventType type) {
    switch (type) {
        case SecurityEventType::AUTH_SUCCESS:                return "AUTH_SUCCESS";
        case SecurityEventType::AUTH_FAILURE:                return "AUTH_FAILURE";
        case SecurityEventType::REPLAY_ATTACK_DETECTED:      return "REPLAY_ATTACK_DETECTED";
        case SecurityEventType::TIMESTAMP_SKEW_DETECTED:     return "TIMESTAMP_SKEW_DETECTED";
        case SecurityEventType::UNAUTHORIZED_NODE_REJECTED:  return "UNAUTHORIZED_NODE_REJECTED";
        case SecurityEventType::CONFIG_TAMPERING_DETECTED:   return "CONFIG_TAMPERING_DETECTED";
        case SecurityEventType::PRIVILEGE_VIOLATION:         return "PRIVILEGE_VIOLATION";
        default:                                             return "UNKNOWN";
    }
}

struct SecurityAuditRecord {
    uint64_t recordId{0};
    uint64_t timestampMs{0};
    SecurityEventType type{SecurityEventType::AUTH_SUCCESS};
    std::string sourceNode;
    std::string ipAddress;
    std::string description;
    uint32_t entryChecksum{0};
};

class SecurityAuditLogger {
public:
    explicit SecurityAuditLogger(size_t maxRecords = 1000);
    ~SecurityAuditLogger() = default;

    void LogEvent(SecurityEventType type, const std::string& sourceNode, const std::string& ip, const std::string& description);

    std::vector<SecurityAuditRecord> GetRecentRecords(size_t limit = 100) const;
    size_t GetTotalRecords() const;
    uint32_t GetViolationCount() const;

    void Clear();
    std::string ExportJson() const;

private:
    static uint32_t CalculateChecksum(uint64_t id, uint64_t time, SecurityEventType type, const std::string& node, const std::string& desc);

    mutable std::mutex mutex_;
    size_t maxRecords_{1000};
    uint64_t nextRecordId_{1};
    std::vector<SecurityAuditRecord> records_;
    uint32_t violationCount_{0};
};

} // namespace aegis::production

