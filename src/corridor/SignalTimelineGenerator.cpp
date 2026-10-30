/**
 * @file SignalTimelineGenerator.cpp
 * @brief Implementation of Phase 3 Signal Timeline Generator & Replay Engine for SMART AEGIS.
 */

#include "aegis/corridor/SignalTimelineGenerator.hpp"
#include "aegis/logger/logger.hpp"

#include <algorithm>
#include <iostream>

namespace aegis {
namespace corridor {

[[maybe_unused]] static void LogInfo(const std::string& msg) {
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::INFO, "TIMELINE_GEN", msg);
}

[[maybe_unused]] static void LogWarn(const std::string& msg) {
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::WARN, "TIMELINE_GEN", msg);
}

[[maybe_unused]] static void LogError(const std::string& msg) {
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::ERR, "TIMELINE_GEN", msg);
}

uint64_t SignalTimelineGenerator::GetCurrentTimeUs() const noexcept {
    return aegis::common::GetMonotonicMicroseconds();
}

CorridorTimelineSchedule SignalTimelineGenerator::GenerateTimeline(const std::string& corridorId,
                                                                  const std::string& ambulanceId,
                                                                  const std::vector<std::string>& intersections,
                                                                  const std::vector<double>& etaOffsetsSec,
                                                                  double greenDurationSec,
                                                                  double yellowDurationSec,
                                                                  double allRedDurationSec,
                                                                  uint64_t baseTimeUs) {
    CorridorTimelineSchedule schedule;
    schedule.scheduleId = "SCHED_" + corridorId;
    schedule.corridorId = corridorId;
    schedule.ambulanceId = ambulanceId;
    schedule.baseTimestampUs = (baseTimeUs > 0) ? baseTimeUs : GetCurrentTimeUs();
    schedule.isValid = false;

    if (intersections.empty() || intersections.size() != etaOffsetsSec.size()) {
        return schedule;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    uint64_t yellowUs = static_cast<uint64_t>(yellowDurationSec * 1000000.0);
    uint64_t allRedUs = static_cast<uint64_t>(allRedDurationSec * 1000000.0);
    uint64_t greenUs = static_cast<uint64_t>(greenDurationSec * 1000000.0);

    uint64_t maxEndUs = schedule.baseTimestampUs;

    for (size_t i = 0; i < intersections.size(); ++i) {
        const std::string& intersectionId = intersections[i];
        double etaSec = etaOffsetsSec[i];
        uint64_t etaOffsetUs = static_cast<uint64_t>(etaSec * 1000000.0);
        uint64_t etaTimestampUs = schedule.baseTimestampUs + etaOffsetUs;

        // Ensure green window opens before arrival: buffer of 3.0s prior to ETA
        uint64_t preBufferUs = 3000000ULL;
        uint64_t greenStartUs = (etaTimestampUs >= preBufferUs) ? (etaTimestampUs - preBufferUs) : etaTimestampUs;

        // Sequence for intersection i:
        // 1. PRE_CLEARANCE_YELLOW (Yellow to clear opposing green)
        uint64_t preYellowStartUs = (greenStartUs >= (yellowUs + allRedUs)) ? (greenStartUs - (yellowUs + allRedUs)) : 0;
        SignalTimelineEvent evPreYellow;
        evPreYellow.eventId = m_nextEventId++;
        evPreYellow.intersectionId = intersectionId;
        evPreYellow.phase = SignalPhaseType::PRE_CLEARANCE_YELLOW;
        evPreYellow.commandString = "YELLOW_PRE_CLEAR";
        evPreYellow.scheduledStartUs = preYellowStartUs;
        evPreYellow.durationUs = yellowUs;
        evPreYellow.scheduledEndUs = preYellowStartUs + yellowUs;
        schedule.events.push_back(evPreYellow);

        // 2. ALL_RED_HOLD (Safety clear before preemption green)
        uint64_t allRedHoldStartUs = evPreYellow.scheduledEndUs;
        SignalTimelineEvent evAllRedHold;
        evAllRedHold.eventId = m_nextEventId++;
        evAllRedHold.intersectionId = intersectionId;
        evAllRedHold.phase = SignalPhaseType::ALL_RED_HOLD;
        evAllRedHold.commandString = "ALL_RED_HOLD";
        evAllRedHold.scheduledStartUs = allRedHoldStartUs;
        evAllRedHold.durationUs = allRedUs;
        evAllRedHold.scheduledEndUs = allRedHoldStartUs + allRedUs;
        schedule.events.push_back(evAllRedHold);

        // 3. EMERGENCY_GREEN
        SignalTimelineEvent evGreen;
        evGreen.eventId = m_nextEventId++;
        evGreen.intersectionId = intersectionId;
        evGreen.phase = SignalPhaseType::EMERGENCY_GREEN;
        evGreen.commandString = "ROAD_A_GREEN"; // Preempt green for emergency direction
        evGreen.scheduledStartUs = greenStartUs;
        evGreen.durationUs = greenUs;
        evGreen.scheduledEndUs = greenStartUs + greenUs;
        schedule.events.push_back(evGreen);

        // 4. POST_YELLOW_TRANSITION
        uint64_t postYellowStartUs = evGreen.scheduledEndUs;
        SignalTimelineEvent evPostYellow;
        evPostYellow.eventId = m_nextEventId++;
        evPostYellow.intersectionId = intersectionId;
        evPostYellow.phase = SignalPhaseType::POST_YELLOW_TRANSITION;
        evPostYellow.commandString = "YELLOW_POST_CLEAR";
        evPostYellow.scheduledStartUs = postYellowStartUs;
        evPostYellow.durationUs = yellowUs;
        evPostYellow.scheduledEndUs = postYellowStartUs + yellowUs;
        schedule.events.push_back(evPostYellow);

        // 5. POST_ALL_RED_CLEARANCE
        uint64_t postAllRedStartUs = evPostYellow.scheduledEndUs;
        SignalTimelineEvent evPostAllRed;
        evPostAllRed.eventId = m_nextEventId++;
        evPostAllRed.intersectionId = intersectionId;
        evPostAllRed.phase = SignalPhaseType::POST_ALL_RED_CLEARANCE;
        evPostAllRed.commandString = "ALL_RED_CLEAR";
        evPostAllRed.scheduledStartUs = postAllRedStartUs;
        evPostAllRed.durationUs = allRedUs;
        evPostAllRed.scheduledEndUs = postAllRedStartUs + allRedUs;
        schedule.events.push_back(evPostAllRed);

        // 6. RESTORED_NOMINAL
        uint64_t restoreStartUs = evPostAllRed.scheduledEndUs;
        SignalTimelineEvent evRestore;
        evRestore.eventId = m_nextEventId++;
        evRestore.intersectionId = intersectionId;
        evRestore.phase = SignalPhaseType::RESTORED_NOMINAL;
        evRestore.commandString = "RESTORE_CYCLIC";
        evRestore.scheduledStartUs = restoreStartUs;
        evRestore.durationUs = 1000000ULL; // 1 second pulse
        evRestore.scheduledEndUs = restoreStartUs + evRestore.durationUs;
        schedule.events.push_back(evRestore);

        if (evRestore.scheduledEndUs > maxEndUs) {
            maxEndUs = evRestore.scheduledEndUs;
        }
    }

    // Sort events deterministically by scheduled start time
    std::sort(schedule.events.begin(), schedule.events.end(),
              [](const SignalTimelineEvent& a, const SignalTimelineEvent& b) {
                  if (a.scheduledStartUs != b.scheduledStartUs) {
                      return a.scheduledStartUs < b.scheduledStartUs;
                  }
                  return a.eventId < b.eventId;
              });

    schedule.totalScheduleDurationSec = static_cast<double>(maxEndUs - schedule.baseTimestampUs) / 1000000.0;
    schedule.isValid = true;

    m_schedules[schedule.scheduleId] = schedule;
    LogInfo("Generated schedule " + schedule.scheduleId +
            " (Events: " + std::to_string(schedule.events.size()) +
            ", Duration: " + std::to_string(schedule.totalScheduleDurationSec) + "s)");
    return schedule;
}

bool SignalTimelineGenerator::RecordExecution(const std::string& scheduleId,
                                             uint64_t eventId,
                                             uint64_t executionTimeUs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_schedules.find(scheduleId);
    if (it == m_schedules.end()) {
        return false;
    }

    for (auto& ev : it->second.events) {
        if (ev.eventId == eventId) {
            ev.isExecuted = true;
            ev.actualExecutionStartUs = executionTimeUs;
            ev.driftUs = static_cast<int64_t>(executionTimeUs) - static_cast<int64_t>(ev.scheduledStartUs);
            return true;
        }
    }
    return false;
}

bool SignalTimelineGenerator::ValidateTimeline(const CorridorTimelineSchedule& schedule,
                                              std::string& failureReason) const {
    if (schedule.events.empty()) {
        failureReason = "Timeline schedule contains zero events.";
        return false;
    }

    for (size_t i = 0; i < schedule.events.size(); ++i) {
        const auto& ev = schedule.events[i];
        if (ev.scheduledEndUs < ev.scheduledStartUs) {
            failureReason = "Event " + std::to_string(ev.eventId) + " has negative duration.";
            return false;
        }
        if (ev.durationUs == 0) {
            failureReason = "Event " + std::to_string(ev.eventId) + " has zero duration.";
            return false;
        }
    }
    return true;
}

bool SignalTimelineGenerator::ReplayTimeline(const std::string& scheduleId, ReplayStepCallback onStep) {
    if (!onStep) return false;

    CorridorTimelineSchedule schedCopy;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_schedules.find(scheduleId);
        if (it == m_schedules.end()) {
            return false;
        }
        schedCopy = it->second;
    }

    for (auto& ev : schedCopy.events) {
        if (!ev.isExecuted) {
            ev.isExecuted = true;
            ev.actualExecutionStartUs = ev.scheduledStartUs;
            ev.driftUs = 0;
        }
        onStep(ev);
    }
    return true;
}

bool SignalTimelineGenerator::GetSchedule(const std::string& scheduleId,
                                         CorridorTimelineSchedule& outSchedule) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_schedules.find(scheduleId);
    if (it != m_schedules.end()) {
        outSchedule = it->second;
        return true;
    }
    return false;
}

std::string SignalTimelineGenerator::ExportToJson(const std::string& scheduleId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_schedules.find(scheduleId);
    if (it == m_schedules.end()) {
        return "{}";
    }

    const auto& s = it->second;
    std::ostringstream oss;
    oss << "{"
        << "\"scheduleId\":\"" << s.scheduleId << "\","
        << "\"corridorId\":\"" << s.corridorId << "\","
        << "\"ambulanceId\":\"" << s.ambulanceId << "\","
        << "\"baseTimestampUs\":" << s.baseTimestampUs << ","
        << "\"totalDurationSec\":" << std::fixed << std::setprecision(2) << s.totalScheduleDurationSec << ","
        << "\"events\":[";

    for (size_t i = 0; i < s.events.size(); ++i) {
        oss << s.events[i].ToJson();
        if (i + 1 < s.events.size()) oss << ",";
    }
    oss << "]}";
    return oss.str();
}

std::string SignalTimelineGenerator::ExportToAsciiTable(const std::string& scheduleId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_schedules.find(scheduleId);
    if (it == m_schedules.end()) {
        return "Schedule " + scheduleId + " not found.";
    }

    const auto& s = it->second;
    std::ostringstream oss;
    oss << "=========================================================================\n"
        << " SMART AEGIS - SIGNAL TIMELINE SCHEDULE: " << s.scheduleId << "\n"
        << "=========================================================================\n"
        << " Corridor ID     : " << s.corridorId << "\n"
        << " Ambulance       : " << s.ambulanceId << "\n"
        << " Total Duration  : " << std::fixed << std::setprecision(1) << s.totalScheduleDurationSec << "s\n"
        << " Total Events    : " << s.events.size() << "\n"
        << "-------------------------------------------------------------------------\n"
        << " Event | Intersection | Phase                  | Offset(s) | Dur(s) | Cmd\n"
        << "-------------------------------------------------------------------------\n";

    for (const auto& ev : s.events) {
        double offsetSec = static_cast<double>(ev.scheduledStartUs - s.baseTimestampUs) / 1000000.0;
        double durSec = static_cast<double>(ev.durationUs) / 1000000.0;

        oss << " " << std::setw(5) << ev.eventId << " | "
            << std::left << std::setw(12) << ev.intersectionId.substr(0, 12) << " | "
            << std::setw(22) << SignalPhaseTypeToString(ev.phase) << " | "
            << std::right << std::fixed << std::setprecision(2) << std::setw(8) << offsetSec << "s | "
            << std::setw(5) << durSec << "s | "
            << std::left << ev.commandString << "\n";
    }
    oss << "=========================================================================\n";
    return oss.str();
}

void SignalTimelineGenerator::ClearAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_schedules.clear();
}

} // namespace corridor
} // namespace aegis
