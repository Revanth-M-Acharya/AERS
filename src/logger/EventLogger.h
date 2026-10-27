/**
 * @file EventLogger.h
 * @brief Comprehensive Event Logging System for SMART AEGIS.
 */

#ifndef AEGIS_LOGGER_EVENT_LOGGER_H
#define AEGIS_LOGGER_EVENT_LOGGER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <functional>
#include <cstdint>

namespace aegis {
namespace logger {

/**
 * @brief Comprehensive enumeration of all significant system events in SMART AEGIS.
 */
enum class EventType {
    PATIENT_CREATED,
    AMBULANCE_REGISTERED,
    PRIORITY_CALCULATED,
    CONFLICT_DETECTED,
    CONFLICT_RESOLVED,
    GREEN_CORRIDOR_ACTIVATED,
    GREEN_CORRIDOR_COMPLETED,
    MANUAL_OVERRIDE,
    NODE_FAILURE,
    NODE_RECOVERY,
    HEARTBEAT,
    REGISTRATION,
    TRAFFIC_COMMAND
};

std::string EventTypeToString(EventType type);
EventType StringToEventType(const std::string& str);

/**
 * @brief Categorical severity classification for audit and alerting.
 */
enum class EventSeverity {
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

std::string EventSeverityToString(EventSeverity severity);
EventSeverity StringToEventSeverity(const std::string& str);

/**
 * @brief Immutable structured event entry capturing all relevant context.
 */
struct SystemEvent {
    std::string eventId{""};
    uint64_t timestampEpochMs{0};
    std::string timestampIso8601{""};
    EventType type{EventType::HEARTBEAT};
    EventSeverity severity{EventSeverity::INFO};
    std::string description{""};
    std::string relatedAmbulance{""};
    std::string relatedPatient{""};
    std::string relatedIntersection{""};
    std::unordered_map<std::string, std::string> metadata{};

    std::string ToJson() const;
    std::string ToString() const;
};

/**
 * @brief Thread-safe Central Event Logging Engine.
 */
class EventLogger {
public:
    using EventListener = std::function<void(const SystemEvent&)>;

    EventLogger();
    ~EventLogger() = default;

    static EventLogger& Instance();

    // Core Logging
    void LogEvent(SystemEvent event);

    void Log(EventType type,
             EventSeverity severity,
             std::string description,
             std::string relatedAmbulance = "",
             std::string relatedPatient = "",
             std::string relatedIntersection = "",
             std::unordered_map<std::string, std::string> metadata = {});

    // Explicit convenience helpers for each significant event type
    void LogPatientCreated(const std::string& patientId,
                           const std::string& description,
                           EventSeverity severity = EventSeverity::INFO,
                           const std::string& ambulanceId = "");

    void LogAmbulanceRegistered(const std::string& ambulanceId,
                                const std::string& description,
                                EventSeverity severity = EventSeverity::INFO,
                                const std::string& intersectionId = "");

    void LogPriorityCalculated(const std::string& ambulanceId,
                               const std::string& patientId,
                               const std::string& description,
                               EventSeverity severity = EventSeverity::INFO);

    void LogConflictDetected(const std::string& intersectionId,
                             const std::string& description,
                             const std::string& primaryAmbulanceId = "",
                             EventSeverity severity = EventSeverity::WARNING);

    void LogConflictResolved(const std::string& intersectionId,
                             const std::string& winnerAmbulanceId,
                             const std::string& description,
                             EventSeverity severity = EventSeverity::INFO);

    void LogGreenCorridorActivated(const std::string& corridorId,
                                   const std::string& ambulanceId,
                                   const std::string& intersectionId,
                                   const std::string& description,
                                   EventSeverity severity = EventSeverity::INFO);

    void LogGreenCorridorCompleted(const std::string& corridorId,
                                   const std::string& ambulanceId,
                                   const std::string& description,
                                   EventSeverity severity = EventSeverity::INFO);

    void LogManualOverride(const std::string& description,
                           const std::string& ambulanceId = "",
                           const std::string& intersectionId = "",
                           EventSeverity severity = EventSeverity::WARNING);

    void LogNodeFailure(const std::string& intersectionId,
                        const std::string& description,
                        EventSeverity severity = EventSeverity::ERROR);

    void LogNodeRecovery(const std::string& intersectionId,
                         const std::string& description,
                         EventSeverity severity = EventSeverity::INFO);

    void LogHeartbeat(const std::string& nodeId,
                     const std::string& description = "Heartbeat received",
                     EventSeverity severity = EventSeverity::INFO);

    void LogRegistration(const std::string& nodeId,
                         const std::string& description = "Node registered",
                         EventSeverity severity = EventSeverity::INFO);

    void LogTrafficCommand(const std::string& intersectionId,
                           const std::string& description,
                           const std::string& ambulanceId = "",
                           EventSeverity severity = EventSeverity::INFO);

    // Queries & Filtering
    std::vector<SystemEvent> GetAllEvents() const;
    std::vector<SystemEvent> GetEventsByType(EventType type) const;
    std::vector<SystemEvent> GetEventsBySeverity(EventSeverity severity) const;
    std::vector<SystemEvent> GetEventsForAmbulance(const std::string& ambulanceId) const;
    std::vector<SystemEvent> GetEventsForPatient(const std::string& patientId) const;
    std::vector<SystemEvent> GetEventsForIntersection(const std::string& intersectionId) const;
    std::vector<SystemEvent> GetEventsInTimeRange(uint64_t startEpochMs, uint64_t endEpochMs) const;

    size_t GetEventCount() const;
    void Clear();

    // Exporting JSON
    std::string ExportJson() const;
    static std::string ExportEventsToJson(const std::vector<SystemEvent>& events);
    bool ExportJsonToFile(const std::string& filePath) const;

    // Listeners & Observers
    void RegisterListener(EventListener listener);
    void ClearListeners();

private:
    std::string GenerateEventId();
    static std::string FormatIso8601(uint64_t epochMs);

    mutable std::mutex m_mutex;
    std::vector<SystemEvent> m_events{};
    std::vector<EventListener> m_listeners{};
    uint64_t m_counter{0};
};

} // namespace logger
} // namespace aegis

#endif // AEGIS_LOGGER_EVENT_LOGGER_H
