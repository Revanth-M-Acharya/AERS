/**
 * @file MissionTimeline.cpp
 * @brief Implementation of MissionTimeline chronological recorder.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "digital_twin/MissionTimeline.hpp"
#include "common/utils/TimeUtils.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace aegis::digital_twin {

void MissionTimeline::RecordEvent(const TimelineEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.push_back(entry);
    if (entries_.size() > 500) {
        entries_.erase(entries_.begin());
    }
}

void MissionTimeline::RecordEvent(
    const std::string& missionId,
    const std::string& entityId,
    const std::string& eventType,
    const std::string& description,
    const std::string& severity) {

    TimelineEntry entry;
    entry.timestampMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    entry.missionId = missionId;
    entry.entityId = entityId;
    entry.eventType = eventType;
    entry.description = description;
    entry.severity = severity;
    RecordEvent(entry);
}

std::vector<TimelineEntry> MissionTimeline::GetAllEvents() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_;
}

std::vector<TimelineEntry> MissionTimeline::GetEventsForMission(const std::string& missionId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TimelineEntry> list;
    for (const auto& e : entries_) {
        if (e.missionId == missionId) list.push_back(e);
    }
    return list;
}

std::vector<TimelineEntry> MissionTimeline::FilterByType(const std::string& eventType) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TimelineEntry> list;
    for (const auto& e : entries_) {
        if (e.eventType == eventType) list.push_back(e);
    }
    return list;
}

std::vector<TimelineEntry> MissionTimeline::FilterByTimeRange(uint64_t fromMonoMs, uint64_t toMonoMs) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TimelineEntry> list;
    for (const auto& e : entries_) {
        if (e.timestampMonoMs >= fromMonoMs && e.timestampMonoMs <= toMonoMs) {
            list.push_back(e);
        }
    }
    return list;
}

size_t MissionTimeline::GetEventCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
}

void MissionTimeline::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
}

std::string MissionTimeline::ToJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream ss;
    ss << "{\"timeline\":[";
    for (size_t i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        ss << "{\"ts\":" << e.timestampMonoMs
           << ",\"mission\":\"" << e.missionId << "\""
           << ",\"entity\":\"" << e.entityId << "\""
           << ",\"type\":\"" << e.eventType << "\""
           << ",\"sev\":\"" << e.severity << "\""
           << ",\"desc\":\"" << e.description << "\"}"
           << (i + 1 < entries_.size() ? "," : "");
    }
    ss << "]}";
    return ss.str();
}

std::string MissionTimeline::ToAsciiTimeline(size_t maxEntries) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream ss;
    ss << "================================================================================\n"
       << " MISSION CHRONOLOGICAL TIMELINE\n"
       << "================================================================================\n";

    if (entries_.empty()) {
        ss << "   (No timeline events recorded)\n";
    } else {
        size_t startIdx = (entries_.size() > maxEntries) ? (entries_.size() - maxEntries) : 0;
        for (size_t i = startIdx; i < entries_.size(); ++i) {
            const auto& e = entries_[i];
            ss << "   [" << std::setw(10) << e.timestampMonoMs << "ms] "
               << "[" << std::setw(8) << e.severity << "] "
               << "{" << e.missionId << "} "
               << "(" << e.entityId << ") "
               << e.eventType << " -> " << e.description << "\n";
        }
    }
    ss << "================================================================================\n";
    return ss.str();
}

} // namespace aegis::digital_twin

