/**
 * @file SecurityAuditLogger.cpp
 * @brief Implementation of tamper-evident security audit logger.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "production/security/SecurityAuditLogger.hpp"
#include "common/utils/TimeUtils.hpp"
#include <sstream>

namespace aegis::production {

SecurityAuditLogger::SecurityAuditLogger(size_t maxRecords)
    : maxRecords_(maxRecords) {}

uint32_t SecurityAuditLogger::CalculateChecksum(
    uint64_t id,
    uint64_t time,
    SecurityEventType type,
    const std::string& node,
    const std::string& desc) {

    uint32_t hash = 2166136261u;
    hash = (hash ^ static_cast<uint32_t>(id)) * 16777619u;
    hash = (hash ^ static_cast<uint32_t>(time)) * 16777619u;
    hash = (hash ^ static_cast<uint32_t>(type)) * 16777619u;
    for (char c : node) {
        hash = (hash ^ static_cast<uint32_t>(c)) * 16777619u;
    }
    for (char c : desc) {
        hash = (hash ^ static_cast<uint32_t>(c)) * 16777619u;
    }
    return hash;
}

void SecurityAuditLogger::LogEvent(
    SecurityEventType type,
    const std::string& sourceNode,
    const std::string& ip,
    const std::string& description) {

    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t id = nextRecordId_++;
    uint64_t time = common::TimeUtils::GetCurrentTimestampMs();

    SecurityAuditRecord rec;
    rec.recordId = id;
    rec.timestampMs = time;
    rec.type = type;
    rec.sourceNode = sourceNode;
    rec.ipAddress = ip;
    rec.description = description;
    rec.entryChecksum = CalculateChecksum(id, time, type, sourceNode, description);

    if (type != SecurityEventType::AUTH_SUCCESS) {
        violationCount_++;
    }

    records_.push_back(rec);
    if (records_.size() > maxRecords_) {
        records_.erase(records_.begin());
    }
}

std::vector<SecurityAuditRecord> SecurityAuditLogger::GetRecentRecords(size_t limit) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (limit >= records_.size()) {
        return records_;
    }
    return std::vector<SecurityAuditRecord>(records_.end() - static_cast<std::ptrdiff_t>(limit), records_.end());
}

size_t SecurityAuditLogger::GetTotalRecords() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return records_.size();
}

uint32_t SecurityAuditLogger::GetViolationCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return violationCount_;
}

void SecurityAuditLogger::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    records_.clear();
    violationCount_ = 0;
}

std::string SecurityAuditLogger::ExportJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream ss;
    ss << "{\n"
       << "  \"total_records\": " << records_.size() << ",\n"
       << "  \"total_violations\": " << violationCount_ << ",\n"
       << "  \"records\": [\n";

    bool first = true;
    for (const auto& r : records_) {
        if (!first) ss << ",\n";
        first = false;
        ss << "    {\n"
           << "      \"id\": " << r.recordId << ",\n"
           << "      \"time_ms\": " << r.timestampMs << ",\n"
           << "      \"type\": \"" << SecurityEventTypeToString(r.type) << "\",\n"
           << "      \"source\": \"" << r.sourceNode << "\",\n"
           << "      \"ip\": \"" << r.ipAddress << "\",\n"
           << "      \"desc\": \"" << r.description << "\",\n"
           << "      \"checksum\": " << r.entryChecksum << "\n"
           << "    }";
    }
    ss << "\n  ]\n}";
    return ss.str();
}

} // namespace aegis::production

