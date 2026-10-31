/**
 * @file MissionTimeline.hpp
 * @brief Ordered historical event stream and chronological timeline for emergency missions.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

namespace aegis::digital_twin {

struct TimelineEntry {
    uint64_t timestampMonoMs{0};
    std::string missionId;
    std::string entityId;      // e.g. "AMB_01", "INT_B", "HOSP_CITY_TRAUMA"
    std::string eventType;     // "MISSION_CREATED", "CORRIDOR_PLANNED", "PREEMPTION_ACTIVE", "INCIDENT_DETECTED", "HOSPITAL_ALERT", "HANDOVER_COMPLETE"
    std::string description;
    std::string severity;      // "INFO", "WARNING", "ALERT", "CRITICAL"
};

class MissionTimeline {
public:
    MissionTimeline() = default;
    ~MissionTimeline() = default;

    void RecordEvent(const TimelineEntry& entry);
    void RecordEvent(const std::string& missionId, const std::string& entityId, const std::string& eventType, const std::string& description, const std::string& severity = "INFO");

    std::vector<TimelineEntry> GetAllEvents() const;
    std::vector<TimelineEntry> GetEventsForMission(const std::string& missionId) const;
    std::vector<TimelineEntry> FilterByType(const std::string& eventType) const;
    std::vector<TimelineEntry> FilterByTimeRange(uint64_t fromMonoMs, uint64_t toMonoMs) const;

    size_t GetEventCount() const;
    void Clear();

    std::string ToJson() const;
    std::string ToAsciiTimeline(size_t maxEntries = 20) const;

private:
    mutable std::mutex mutex_;
    std::vector<TimelineEntry> entries_;
};

} // namespace aegis::digital_twin

