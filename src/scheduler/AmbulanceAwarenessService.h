/**
 * @file AmbulanceAwarenessService.h
 * @brief Inter-ambulance awareness and proximity notification service for SMART AEGIS.
 */

#ifndef AEGIS_SCHEDULER_AMBULANCE_AWARENESS_SERVICE_H
#define AEGIS_SCHEDULER_AMBULANCE_AWARENESS_SERVICE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <optional>
#include <cstdint>

#include "ambulance/AmbulanceProfile.h"
#include "scheduler/ConflictDetectionEngine.h"

namespace aegis {
namespace scheduler {

/**
 * @brief Telemetry notification sent to an ambulance advising them of another approaching vehicle.
 */
struct AwarenessNotification {
    std::string notificationId{""};
    std::string targetAmbulanceId{""};       ///< Vehicle receiving the advisory
    std::string approachingAmbulanceId{""};  ///< Conflicting/approaching peer vehicle
    std::string intersectionId{""};          ///< Common junction of contention
    medical::PriorityLevel approachingPriority{medical::PriorityLevel::LOW};
    std::string approachingPriorityString{"LOW"};
    double approachingScore{0.0};
    uint32_t approachingEtaSeconds{0};
    std::string conflictStatus{"CONFLICT_DETECTED"};
    std::string advisoryMessage{""};
    uint64_t timestampEpochMs{0};

    std::string ToString() const;
    std::string ToJson() const;
};

/**
 * @brief Real-time service notifying ambulances whenever another emergency vehicle approaches the same junction.
 */
class AmbulanceAwarenessService {
public:
    using NotificationListener = std::function<void(const AwarenessNotification&)>;

    AmbulanceAwarenessService() = default;
    ~AmbulanceAwarenessService() = default;

    // Notification Generation
    std::vector<AwarenessNotification> GenerateAwarenessForConflict(
        const ConflictEvent& conflict,
        const std::string& conflictStatus = "CONFLICT_DETECTED");

    std::vector<AwarenessNotification> GenerateAwarenessFromArrivals(
        const std::string& intersectionId,
        const std::vector<AmbulanceArrival>& arrivals,
        const std::string& conflictStatus = "CONFLICT_DETECTED");

    // Direct Notification Dispatch
    void PostNotification(AwarenessNotification notification);

    // Queries
    std::vector<AwarenessNotification> GetNotificationsForAmbulance(const std::string& ambulanceId) const;
    std::optional<AwarenessNotification> GetLatestNotification(const std::string& ambulanceId) const;
    std::vector<AwarenessNotification> GetAllNotifications() const;
    size_t GetNotificationCount() const;
    bool HasNotificationsFor(const std::string& ambulanceId) const;

    // Observer Hooks
    void AddListener(NotificationListener listener);
    void ClearListeners();

    // Reset
    void Clear();

private:
    static uint64_t CurrentTimeMillis();
    std::string GenerateNotificationId();

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::vector<AwarenessNotification>> m_ambulanceNotifications{};
    std::vector<AwarenessNotification> m_allNotifications{};
    std::vector<NotificationListener> m_listeners{};
    uint64_t m_notificationSequence{0};
};

} // namespace scheduler
} // namespace aegis

#endif // AEGIS_SCHEDULER_AMBULANCE_AWARENESS_SERVICE_H
