/**
 * @file HospitalNotificationManager.hpp
 * @brief Telemetry messaging and clinical inbound alert manager for hospitals.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/orchestration/event_bus/IEventBus.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <cstdint>

namespace aegis::coordinator::hospital {

enum class HospitalNotificationType {
    INCOMING_PATIENT,
    ETA_UPDATE,
    PRIORITY_UPDATE,
    ROUTE_CHANGED,
    CRITICAL_ALERT,
    MISSION_CANCELLED,
    MISSION_COMPLETED
};

inline std::string HospitalNotificationTypeToString(HospitalNotificationType type) {
    switch (type) {
        case HospitalNotificationType::INCOMING_PATIENT:  return "INCOMING_PATIENT";
        case HospitalNotificationType::ETA_UPDATE:         return "ETA_UPDATE";
        case HospitalNotificationType::PRIORITY_UPDATE:    return "PRIORITY_UPDATE";
        case HospitalNotificationType::ROUTE_CHANGED:      return "ROUTE_CHANGED";
        case HospitalNotificationType::CRITICAL_ALERT:     return "CRITICAL_ALERT";
        case HospitalNotificationType::MISSION_CANCELLED:  return "MISSION_CANCELLED";
        case HospitalNotificationType::MISSION_COMPLETED:  return "MISSION_COMPLETED";
        default:                                           return "UNKNOWN";
    }
}

struct HospitalNotification {
    std::string notificationId;
    HospitalNotificationType type{HospitalNotificationType::INCOMING_PATIENT};
    std::string emergencyId;
    std::string hospitalId;
    std::string ambulanceId;
    std::string patientCondition;
    double etaSeconds{0.0};
    uint32_t priorityLevel{1};
    std::string requiredDepartment;
    uint64_t expectedArrivalTimestampMonoMs{0};
    std::string details;
    uint64_t sentTimestampMonoMs{0};
};

class HospitalNotificationManager {
public:
    explicit HospitalNotificationManager(std::shared_ptr<orchestration::IEventBus> eventBus = nullptr);
    ~HospitalNotificationManager() = default;

    /**
     * @brief Dispatch notification to hospital.
     */
    std::string SendNotification(const HospitalNotification& notification, uint64_t monoMs = 0);

    std::vector<HospitalNotification> GetNotificationsForHospital(const std::string& hospitalId) const;
    std::vector<HospitalNotification> GetNotificationsForEmergency(const std::string& emergencyId) const;
    size_t GetTotalNotificationCount() const;

    void Reset();

private:
    std::shared_ptr<orchestration::IEventBus> eventBus_;
    mutable std::mutex mutex_;
    uint64_t nextId_{1};
    std::unordered_map<std::string, std::vector<HospitalNotification>> notificationsByHospital_;
    std::unordered_map<std::string, std::vector<HospitalNotification>> notificationsByEmergency_;
};

} // namespace aegis::coordinator::hospital

