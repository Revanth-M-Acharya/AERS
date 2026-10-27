/**
 * @file ConflictDetectionEngine.h
 * @brief Multi-Ambulance Spatial-Temporal Conflict Detection Engine for SMART AEGIS.
 */

#ifndef AEGIS_SCHEDULER_CONFLICT_DETECTION_ENGINE_H
#define AEGIS_SCHEDULER_CONFLICT_DETECTION_ENGINE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <functional>
#include <optional>
#include <cstdint>

#include "ambulance/AmbulanceProfile.h"
#include "scheduler/ConflictDetector.h"
#include "logger/EventLogger.h"

namespace aegis {
namespace scheduler {

/**
 * @brief Structured event representing a detected junction contention.
 */
struct ConflictEvent {
    std::string eventId{""};
    std::string intersectionId{""};
    std::vector<std::string> conflictingAmbulanceIds{};
    std::vector<AmbulanceArrival> conflictingArrivals{};
    uint32_t timeWindowSeconds{10};
    uint32_t earliestEtaSeconds{0};
    uint32_t latestEtaSeconds{0};
    uint32_t timeDeltaSeconds{0};
    uint64_t timestampEpochMs{0};
    std::string description{""};
    logger::EventSeverity severity{logger::EventSeverity::WARNING};

    size_t GetConflictCount() const noexcept { return conflictingAmbulanceIds.size(); }
    bool HasAmbulance(const std::string& ambId) const;
    std::string ToString() const;
    std::string ToJson() const;
};

/**
 * @brief Engine responsible for continuous junction arrival tracking, time-window contention detection,
 * and conflict event dispatching.
 */
class ConflictDetectionEngine {
public:
    using ConflictListener = std::function<void(const ConflictEvent&)>;
    static constexpr uint32_t DEFAULT_CONFLICT_WINDOW_SECONDS = 10;

    ConflictDetectionEngine();
    explicit ConflictDetectionEngine(uint32_t conflictWindowSeconds);
    ~ConflictDetectionEngine() = default;

    // Window Configuration
    uint32_t GetConflictWindowSeconds() const noexcept;
    void SetConflictWindowSeconds(uint32_t seconds) noexcept;

    // Arrival Registration
    void RegisterArrival(const AmbulanceArrival& arrival);
    void RegisterArrival(const std::string& ambulanceId,
                         const std::string& intersectionId,
                         uint32_t etaSeconds,
                         double distanceMeters = 0.0,
                         const ambulance::AmbulanceProfile& profile = ambulance::AmbulanceProfile{});
    void ClearArrivals();

    // Conflict Detection & Event Generation
    std::vector<ConflictEvent> DetectConflicts();
    std::vector<ConflictEvent> DetectConflicts(const std::vector<ambulance::AmbulanceProfile>& ambulances,
                                              uint32_t windowSeconds = DEFAULT_CONFLICT_WINDOW_SECONDS);

    // Queries
    std::vector<ConflictEvent> GetActiveConflicts() const;
    std::optional<ConflictEvent> GetConflictForIntersection(const std::string& intersectionId) const;
    bool HasConflictAt(const std::string& intersectionId) const;
    std::vector<ConflictEvent> GetConflictHistory() const;
    size_t GetTotalArrivalCount() const;

    // Listener Callbacks
    void AddConflictListener(ConflictListener listener);
    void ClearListeners();

    // Event Logging Flag
    void EnableEventLogging(bool enable) noexcept { m_enableLogging = enable; }
    bool IsEventLoggingEnabled() const noexcept { return m_enableLogging; }

    void Clear();

private:
    static uint64_t CurrentTimeMillis();
    ConflictEvent BuildConflictEvent(const IntersectionConflict& conflict);

    mutable std::mutex m_mutex;
    ConflictDetector m_detector;
    uint32_t m_conflictWindowSeconds{DEFAULT_CONFLICT_WINDOW_SECONDS};
    std::vector<ConflictEvent> m_activeConflicts{};
    std::vector<ConflictEvent> m_conflictHistory{};
    std::vector<ConflictListener> m_listeners{};
    bool m_enableLogging{true};
    uint64_t m_eventSequence{0};
};

} // namespace scheduler
} // namespace aegis

#endif // AEGIS_SCHEDULER_CONFLICT_DETECTION_ENGINE_H
