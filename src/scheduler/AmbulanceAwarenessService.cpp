/**
 * @file AmbulanceAwarenessService.cpp
 * @brief Implementation of AmbulanceAwarenessService peer notification generation and dispatching.
 */

#include "scheduler/AmbulanceAwarenessService.h"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace aegis {
namespace scheduler {

std::string AwarenessNotification::ToString() const {
    std::ostringstream oss;
    oss << "AwarenessNotification [" << notificationId << "] for " << targetAmbulanceId << "\n"
        << "  Approaching Unit : " << approachingAmbulanceId << " at " << intersectionId << "\n"
        << "  Priority         : " << approachingPriorityString << " (Score: " << approachingScore << ")\n"
        << "  ETA              : " << approachingEtaSeconds << "s\n"
        << "  Conflict Status  : " << conflictStatus << "\n"
        << "  Advisory Message : " << advisoryMessage;
    return oss.str();
}

std::string AwarenessNotification::ToJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"notification_id\":\"" << notificationId << "\","
        << "\"target_ambulance_id\":\"" << targetAmbulanceId << "\","
        << "\"approaching_ambulance_id\":\"" << approachingAmbulanceId << "\","
        << "\"intersection_id\":\"" << intersectionId << "\","
        << "\"approaching_priority\":\"" << approachingPriorityString << "\","
        << "\"approaching_score\":" << std::fixed << std::setprecision(2) << approachingScore << ","
        << "\"approaching_eta_seconds\":" << approachingEtaSeconds << ","
        << "\"conflict_status\":\"" << conflictStatus << "\","
        << "\"advisory_message\":\"" << advisoryMessage << "\","
        << "\"timestamp_epoch_ms\":" << timestampEpochMs << "}";
    return oss.str();
}

uint64_t AmbulanceAwarenessService::CurrentTimeMillis() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

std::string AmbulanceAwarenessService::GenerateNotificationId() {
    ++m_notificationSequence;
    std::ostringstream oss;
    oss << "AWARE-" << m_notificationSequence;
    return oss.str();
}

std::vector<AwarenessNotification> AmbulanceAwarenessService::GenerateAwarenessForConflict(
    const ConflictEvent& conflict,
    const std::string& conflictStatus) {

    return GenerateAwarenessFromArrivals(conflict.intersectionId, conflict.conflictingArrivals, conflictStatus);
}

std::vector<AwarenessNotification> AmbulanceAwarenessService::GenerateAwarenessFromArrivals(
    const std::string& intersectionId,
    const std::vector<AmbulanceArrival>& arrivals,
    const std::string& conflictStatus) {

    std::vector<AwarenessNotification> generated;
    std::vector<NotificationListener> listenersCopy;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        uint64_t now = CurrentTimeMillis();

        for (size_t i = 0; i < arrivals.size(); ++i) {
            const auto& target = arrivals[i];

            for (size_t j = 0; j < arrivals.size(); ++j) {
                if (i == j) continue;
                const auto& peer = arrivals[j];

                AwarenessNotification notif;
                notif.notificationId = GenerateNotificationId();
                notif.targetAmbulanceId = target.ambulanceId;
                notif.approachingAmbulanceId = peer.ambulanceId;
                notif.intersectionId = intersectionId;
                notif.approachingPriority = peer.profile.GetPriority();
                notif.approachingPriorityString = medical::PriorityLevelToString(peer.profile.GetPriority());
                notif.approachingScore = peer.profile.GetPriorityScore();
                if (notif.approachingScore <= 0.0 && peer.profile.HasPatient()) {
                    notif.approachingScore = medical::PriorityEngine::CalculateEmergencyScore(*peer.profile.GetPatient());
                }
                notif.approachingEtaSeconds = peer.etaSeconds;
                notif.conflictStatus = conflictStatus;
                notif.timestampEpochMs = now;

                std::ostringstream msgOss;
                msgOss << "ALERT: Emergency vehicle [" << peer.ambulanceId << "] (Priority: "
                       << notif.approachingPriorityString << ", ETA: " << peer.etaSeconds
                       << "s) approaching " << intersectionId << ". Conflict Status: " << conflictStatus;
                notif.advisoryMessage = msgOss.str();

                m_ambulanceNotifications[target.ambulanceId].push_back(notif);
                m_allNotifications.push_back(notif);
                generated.push_back(notif);
            }
        }
        listenersCopy = m_listeners;
    }

    for (const auto& notif : generated) {
        for (const auto& listener : listenersCopy) {
            if (listener) {
                listener(notif);
            }
        }
    }

    return generated;
}

void AmbulanceAwarenessService::PostNotification(AwarenessNotification notification) {
    std::vector<NotificationListener> listenersCopy;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (notification.notificationId.empty()) {
            notification.notificationId = GenerateNotificationId();
        }
        if (notification.timestampEpochMs == 0) {
            notification.timestampEpochMs = CurrentTimeMillis();
        }
        m_ambulanceNotifications[notification.targetAmbulanceId].push_back(notification);
        m_allNotifications.push_back(notification);
        listenersCopy = m_listeners;
    }

    for (const auto& listener : listenersCopy) {
        if (listener) {
            listener(notification);
        }
    }
}

std::vector<AwarenessNotification> AmbulanceAwarenessService::GetNotificationsForAmbulance(
    const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_ambulanceNotifications.find(ambulanceId);
    if (it != m_ambulanceNotifications.end()) {
        return it->second;
    }
    return {};
}

std::optional<AwarenessNotification> AmbulanceAwarenessService::GetLatestNotification(
    const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_ambulanceNotifications.find(ambulanceId);
    if (it != m_ambulanceNotifications.end() && !it->second.empty()) {
        return it->second.back();
    }
    return std::nullopt;
}

std::vector<AwarenessNotification> AmbulanceAwarenessService::GetAllNotifications() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_allNotifications;
}

size_t AmbulanceAwarenessService::GetNotificationCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_allNotifications.size();
}

bool AmbulanceAwarenessService::HasNotificationsFor(const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_ambulanceNotifications.find(ambulanceId);
    return it != m_ambulanceNotifications.end() && !it->second.empty();
}

void AmbulanceAwarenessService::AddListener(NotificationListener listener) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listeners.push_back(std::move(listener));
}

void AmbulanceAwarenessService::ClearListeners() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listeners.clear();
}

void AmbulanceAwarenessService::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_ambulanceNotifications.clear();
    m_allNotifications.clear();
}

} // namespace scheduler
} // namespace aegis
