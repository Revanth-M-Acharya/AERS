/**
 * @file EventLogger.cpp
 * @brief Implementation of SMART AEGIS Central Event Logging System.
 */

#include "logger/EventLogger.h"
#include <chrono>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <ctime>

namespace aegis {
namespace logger {

static std::string EscapeJsonString(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 8);
    for (char c : input) {
        switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:   out += c; break;
        }
    }
    return out;
}

std::string EventTypeToString(EventType type) {
    switch (type) {
        case EventType::PATIENT_CREATED:           return "PATIENT_CREATED";
        case EventType::AMBULANCE_REGISTERED:      return "AMBULANCE_REGISTERED";
        case EventType::PRIORITY_CALCULATED:       return "PRIORITY_CALCULATED";
        case EventType::CONFLICT_DETECTED:         return "CONFLICT_DETECTED";
        case EventType::CONFLICT_RESOLVED:         return "CONFLICT_RESOLVED";
        case EventType::GREEN_CORRIDOR_ACTIVATED:  return "GREEN_CORRIDOR_ACTIVATED";
        case EventType::GREEN_CORRIDOR_COMPLETED:  return "GREEN_CORRIDOR_COMPLETED";
        case EventType::MANUAL_OVERRIDE:           return "MANUAL_OVERRIDE";
        case EventType::NODE_FAILURE:              return "NODE_FAILURE";
        case EventType::NODE_RECOVERY:             return "NODE_RECOVERY";
        case EventType::HEARTBEAT:                 return "HEARTBEAT";
        case EventType::REGISTRATION:              return "REGISTRATION";
        case EventType::TRAFFIC_COMMAND:           return "TRAFFIC_COMMAND";
        default:                                   return "UNKNOWN";
    }
}

EventType StringToEventType(const std::string& str) {
    if (str == "PATIENT_CREATED")          return EventType::PATIENT_CREATED;
    if (str == "AMBULANCE_REGISTERED")     return EventType::AMBULANCE_REGISTERED;
    if (str == "PRIORITY_CALCULATED")      return EventType::PRIORITY_CALCULATED;
    if (str == "CONFLICT_DETECTED")        return EventType::CONFLICT_DETECTED;
    if (str == "CONFLICT_RESOLVED")        return EventType::CONFLICT_RESOLVED;
    if (str == "GREEN_CORRIDOR_ACTIVATED") return EventType::GREEN_CORRIDOR_ACTIVATED;
    if (str == "GREEN_CORRIDOR_COMPLETED") return EventType::GREEN_CORRIDOR_COMPLETED;
    if (str == "MANUAL_OVERRIDE")          return EventType::MANUAL_OVERRIDE;
    if (str == "NODE_FAILURE")             return EventType::NODE_FAILURE;
    if (str == "NODE_RECOVERY")            return EventType::NODE_RECOVERY;
    if (str == "HEARTBEAT")                return EventType::HEARTBEAT;
    if (str == "REGISTRATION")             return EventType::REGISTRATION;
    if (str == "TRAFFIC_COMMAND")          return EventType::TRAFFIC_COMMAND;
    return EventType::HEARTBEAT;
}

std::string EventSeverityToString(EventSeverity severity) {
    switch (severity) {
        case EventSeverity::INFO:     return "INFO";
        case EventSeverity::WARNING:  return "WARNING";
        case EventSeverity::ERROR:    return "ERROR";
        case EventSeverity::CRITICAL: return "CRITICAL";
        default:                      return "INFO";
    }
}

EventSeverity StringToEventSeverity(const std::string& str) {
    if (str == "CRITICAL") return EventSeverity::CRITICAL;
    if (str == "ERROR")    return EventSeverity::ERROR;
    if (str == "WARNING")  return EventSeverity::WARNING;
    return EventSeverity::INFO;
}

std::string SystemEvent::ToJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"eventId\":\"" << EscapeJsonString(eventId) << "\","
        << "\"timestampEpochMs\":" << timestampEpochMs << ","
        << "\"timestampIso8601\":\"" << EscapeJsonString(timestampIso8601) << "\","
        << "\"type\":\"" << EventTypeToString(type) << "\","
        << "\"severity\":\"" << EventSeverityToString(severity) << "\","
        << "\"description\":\"" << EscapeJsonString(description) << "\","
        << "\"relatedAmbulance\":\"" << EscapeJsonString(relatedAmbulance) << "\","
        << "\"relatedPatient\":\"" << EscapeJsonString(relatedPatient) << "\","
        << "\"relatedIntersection\":\"" << EscapeJsonString(relatedIntersection) << "\","
        << "\"metadata\":{";

    size_t idx = 0;
    for (const auto& [k, v] : metadata) {
        oss << "\"" << EscapeJsonString(k) << "\":\"" << EscapeJsonString(v) << "\"";
        if (++idx < metadata.size()) oss << ",";
    }
    oss << "}}";
    return oss.str();
}

std::string SystemEvent::ToString() const {
    std::ostringstream oss;
    oss << "[" << (timestampIso8601.empty() ? std::to_string(timestampEpochMs) : timestampIso8601) << "] "
        << "[" << EventSeverityToString(severity) << "] "
        << "[" << EventTypeToString(type) << "] "
        << "[" << eventId << "] "
        << description;

    if (!relatedAmbulance.empty()) oss << " | Amb: " << relatedAmbulance;
    if (!relatedPatient.empty()) oss << " | Patient: " << relatedPatient;
    if (!relatedIntersection.empty()) oss << " | Junction: " << relatedIntersection;

    return oss.str();
}

EventLogger::EventLogger() = default;

EventLogger& EventLogger::Instance() {
    static EventLogger s_instance;
    return s_instance;
}

std::string EventLogger::FormatIso8601(uint64_t epochMs) {
    std::time_t sec = static_cast<std::time_t>(epochMs / 1000);
    uint32_t millis = static_cast<uint32_t>(epochMs % 1000);
    std::tm tmBuf{};
#if defined(_WIN32)
    gmtime_s(&tmBuf, &sec);
#else
    gmtime_r(&sec, &tmBuf);
#endif
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
                  tmBuf.tm_year + 1900, tmBuf.tm_mon + 1, tmBuf.tm_mday,
                  tmBuf.tm_hour, tmBuf.tm_min, tmBuf.tm_sec, millis);
    return std::string(buf);
}

std::string EventLogger::GenerateEventId() {
    std::ostringstream oss;
    oss << "EVT-" << (++m_counter);
    return oss.str();
}

void EventLogger::LogEvent(SystemEvent event) {
    std::vector<EventListener> listenersCopy;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (event.timestampEpochMs == 0) {
            event.timestampEpochMs = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count());
        }
        if (event.timestampIso8601.empty()) {
            event.timestampIso8601 = FormatIso8601(event.timestampEpochMs);
        }
        if (event.eventId.empty()) {
            event.eventId = GenerateEventId();
        }

        m_events.push_back(event);
        listenersCopy = m_listeners;
    }

    for (const auto& listener : listenersCopy) {
        if (listener) {
            listener(event);
        }
    }
}

void EventLogger::Log(EventType type,
                      EventSeverity severity,
                      std::string description,
                      std::string relatedAmbulance,
                      std::string relatedPatient,
                      std::string relatedIntersection,
                      std::unordered_map<std::string, std::string> metadata) {
    SystemEvent evt;
    evt.type = type;
    evt.severity = severity;
    evt.description = std::move(description);
    evt.relatedAmbulance = std::move(relatedAmbulance);
    evt.relatedPatient = std::move(relatedPatient);
    evt.relatedIntersection = std::move(relatedIntersection);
    evt.metadata = std::move(metadata);
    LogEvent(std::move(evt));
}

void EventLogger::LogPatientCreated(const std::string& patientId,
                                   const std::string& description,
                                   EventSeverity severity,
                                   const std::string& ambulanceId) {
    Log(EventType::PATIENT_CREATED, severity, description, ambulanceId, patientId, "");
}

void EventLogger::LogAmbulanceRegistered(const std::string& ambulanceId,
                                        const std::string& description,
                                        EventSeverity severity,
                                        const std::string& intersectionId) {
    Log(EventType::AMBULANCE_REGISTERED, severity, description, ambulanceId, "", intersectionId);
}

void EventLogger::LogPriorityCalculated(const std::string& ambulanceId,
                                       const std::string& patientId,
                                       const std::string& description,
                                       EventSeverity severity) {
    Log(EventType::PRIORITY_CALCULATED, severity, description, ambulanceId, patientId, "");
}

void EventLogger::LogConflictDetected(const std::string& intersectionId,
                                     const std::string& description,
                                     const std::string& primaryAmbulanceId,
                                     EventSeverity severity) {
    Log(EventType::CONFLICT_DETECTED, severity, description, primaryAmbulanceId, "", intersectionId);
}

void EventLogger::LogConflictResolved(const std::string& intersectionId,
                                     const std::string& winnerAmbulanceId,
                                     const std::string& description,
                                     EventSeverity severity) {
    Log(EventType::CONFLICT_RESOLVED, severity, description, winnerAmbulanceId, "", intersectionId);
}

void EventLogger::LogGreenCorridorActivated(const std::string& corridorId,
                                           const std::string& ambulanceId,
                                           const std::string& intersectionId,
                                           const std::string& description,
                                           EventSeverity severity) {
    std::unordered_map<std::string, std::string> meta{{"corridorId", corridorId}};
    Log(EventType::GREEN_CORRIDOR_ACTIVATED, severity, description, ambulanceId, "", intersectionId, meta);
}

void EventLogger::LogGreenCorridorCompleted(const std::string& corridorId,
                                           const std::string& ambulanceId,
                                           const std::string& description,
                                           EventSeverity severity) {
    std::unordered_map<std::string, std::string> meta{{"corridorId", corridorId}};
    Log(EventType::GREEN_CORRIDOR_COMPLETED, severity, description, ambulanceId, "", "", meta);
}

void EventLogger::LogManualOverride(const std::string& description,
                                   const std::string& ambulanceId,
                                   const std::string& intersectionId,
                                   EventSeverity severity) {
    Log(EventType::MANUAL_OVERRIDE, severity, description, ambulanceId, "", intersectionId);
}

void EventLogger::LogNodeFailure(const std::string& intersectionId,
                                const std::string& description,
                                EventSeverity severity) {
    Log(EventType::NODE_FAILURE, severity, description, "", "", intersectionId);
}

void EventLogger::LogNodeRecovery(const std::string& intersectionId,
                                 const std::string& description,
                                 EventSeverity severity) {
    Log(EventType::NODE_RECOVERY, severity, description, "", "", intersectionId);
}

void EventLogger::LogHeartbeat(const std::string& nodeId,
                             const std::string& description,
                             EventSeverity severity) {
    Log(EventType::HEARTBEAT, severity, description, "", "", nodeId);
}

void EventLogger::LogRegistration(const std::string& nodeId,
                                 const std::string& description,
                                 EventSeverity severity) {
    Log(EventType::REGISTRATION, severity, description, "", "", nodeId);
}

void EventLogger::LogTrafficCommand(const std::string& intersectionId,
                                   const std::string& description,
                                   const std::string& ambulanceId,
                                   EventSeverity severity) {
    Log(EventType::TRAFFIC_COMMAND, severity, description, ambulanceId, "", intersectionId);
}

std::vector<SystemEvent> EventLogger::GetAllEvents() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_events;
}

std::vector<SystemEvent> EventLogger::GetEventsByType(EventType type) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<SystemEvent> results;
    for (const auto& ev : m_events) {
        if (ev.type == type) {
            results.push_back(ev);
        }
    }
    return results;
}

std::vector<SystemEvent> EventLogger::GetEventsBySeverity(EventSeverity severity) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<SystemEvent> results;
    for (const auto& ev : m_events) {
        if (ev.severity == severity) {
            results.push_back(ev);
        }
    }
    return results;
}

std::vector<SystemEvent> EventLogger::GetEventsForAmbulance(const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<SystemEvent> results;
    for (const auto& ev : m_events) {
        if (ev.relatedAmbulance == ambulanceId) {
            results.push_back(ev);
        }
    }
    return results;
}

std::vector<SystemEvent> EventLogger::GetEventsForPatient(const std::string& patientId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<SystemEvent> results;
    for (const auto& ev : m_events) {
        if (ev.relatedPatient == patientId) {
            results.push_back(ev);
        }
    }
    return results;
}

std::vector<SystemEvent> EventLogger::GetEventsForIntersection(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<SystemEvent> results;
    for (const auto& ev : m_events) {
        if (ev.relatedIntersection == intersectionId) {
            results.push_back(ev);
        }
    }
    return results;
}

std::vector<SystemEvent> EventLogger::GetEventsInTimeRange(uint64_t startEpochMs, uint64_t endEpochMs) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<SystemEvent> results;
    for (const auto& ev : m_events) {
        if (ev.timestampEpochMs >= startEpochMs && ev.timestampEpochMs <= endEpochMs) {
            results.push_back(ev);
        }
    }
    return results;
}

size_t EventLogger::GetEventCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_events.size();
}

void EventLogger::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_events.clear();
}

std::string EventLogger::ExportEventsToJson(const std::vector<SystemEvent>& events) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < events.size(); ++i) {
        oss << events[i].ToJson();
        if (i + 1 < events.size()) oss << ",";
    }
    oss << "]";
    return oss.str();
}

std::string EventLogger::ExportJson() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return ExportEventsToJson(m_events);
}

bool EventLogger::ExportJsonToFile(const std::string& filePath) const {
    std::string jsonStr = ExportJson();
    std::ofstream outFile(filePath, std::ios::out | std::ios::trunc);
    if (!outFile.is_open()) {
        return false;
    }
    outFile << jsonStr;
    outFile.close();
    return true;
}

void EventLogger::RegisterListener(EventListener listener) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listeners.push_back(std::move(listener));
}

void EventLogger::ClearListeners() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listeners.clear();
}

} // namespace logger
} // namespace aegis
