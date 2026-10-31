/**
 * @file HospitalNotificationManager.cpp
 * @brief Implementation of hospital notification routing and history.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/hospital/HospitalNotificationManager.hpp"
#include "common/utils/TimeUtils.hpp"

namespace aegis::coordinator::hospital {

HospitalNotificationManager::HospitalNotificationManager(std::shared_ptr<orchestration::IEventBus> eventBus)
    : eventBus_(std::move(eventBus)) {}

std::string HospitalNotificationManager::SendNotification(const HospitalNotification& notification, uint64_t monoMs) {
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();

    HospitalNotification notif = notification;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (notif.notificationId.empty()) {
            notif.notificationId = "NOTIF-" + std::to_string(nextId_++);
        }
        notif.sentTimestampMonoMs = monoMs;
        notificationsByHospital_[notif.hospitalId].push_back(notif);
        notificationsByEmergency_[notif.emergencyId].push_back(notif);
    }

    if (eventBus_) {
        eventBus_->Publish(orchestration::Event(
            orchestration::EventType::HospitalUpdated,
            notif.hospitalId,
            notif.emergencyId,
            "Type=" + HospitalNotificationTypeToString(notif.type) + ";ETA=" + std::to_string(notif.etaSeconds),
            monoMs
        ));
    }

    return notif.notificationId;
}

std::vector<HospitalNotification> HospitalNotificationManager::GetNotificationsForHospital(const std::string& hospitalId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = notificationsByHospital_.find(hospitalId);
    if (it != notificationsByHospital_.end()) return it->second;
    return {};
}

std::vector<HospitalNotification> HospitalNotificationManager::GetNotificationsForEmergency(const std::string& emergencyId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = notificationsByEmergency_.find(emergencyId);
    if (it != notificationsByEmergency_.end()) return it->second;
    return {};
}

size_t HospitalNotificationManager::GetTotalNotificationCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [_, list] : notificationsByHospital_) {
        count += list.size();
    }
    return count;
}

void HospitalNotificationManager::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    notificationsByHospital_.clear();
    notificationsByEmergency_.clear();
    nextId_ = 1;
}

} // namespace aegis::coordinator::hospital

