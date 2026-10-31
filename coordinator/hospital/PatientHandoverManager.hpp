/**
 * @file PatientHandoverManager.hpp
 * @brief Clinical patient handover, admission transition, and mission completion manager.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/orchestration/event_bus/IEventBus.hpp"
#include "coordinator/orchestration/session/EmergencySessionManager.hpp"
#include "coordinator/control_center/IncidentManager.hpp"
#include "coordinator/hospital/BedAvailabilityManager.hpp"
#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <cstdint>

namespace aegis::coordinator::hospital {

enum class HandoverStatus {
    PENDING_ARRIVAL,
    ARRIVED_AT_BAY,
    ACCEPTED_BY_HOSPITAL,
    TREATMENT_STARTED,
    HANDOVER_COMPLETED,
    MISSION_ARCHIVED
};

inline std::string HandoverStatusToString(HandoverStatus status) {
    switch (status) {
        case HandoverStatus::PENDING_ARRIVAL:      return "PENDING_ARRIVAL";
        case HandoverStatus::ARRIVED_AT_BAY:       return "ARRIVED_AT_BAY";
        case HandoverStatus::ACCEPTED_BY_HOSPITAL: return "ACCEPTED_BY_HOSPITAL";
        case HandoverStatus::TREATMENT_STARTED:    return "TREATMENT_STARTED";
        case HandoverStatus::HANDOVER_COMPLETED:   return "HANDOVER_COMPLETED";
        case HandoverStatus::MISSION_ARCHIVED:     return "MISSION_ARCHIVED";
        default:                                   return "UNKNOWN";
    }
}

struct HandoverRecord {
    std::string emergencyId;
    std::string hospitalId;
    std::string ambulanceId;
    std::string receivingDoctor;
    std::string assignedRoomOrBed;
    HandoverStatus status{HandoverStatus::PENDING_ARRIVAL};
    uint64_t arrivalMonoMs{0};
    uint64_t acceptedMonoMs{0};
    uint64_t treatmentStartedMonoMs{0};
    uint64_t completedMonoMs{0};
    uint64_t archivedMonoMs{0};
};

class PatientHandoverManager {
public:
    PatientHandoverManager(std::shared_ptr<orchestration::IEventBus> eventBus = nullptr,
                           std::shared_ptr<orchestration::EmergencySessionManager> sessionManager = nullptr,
                           std::shared_ptr<control_center::IncidentManager> incidentManager = nullptr,
                           std::shared_ptr<BedAvailabilityManager> bedManager = nullptr);
    ~PatientHandoverManager() = default;

    /**
     * @brief Record ambulance arrival at hospital emergency bay.
     */
    bool RecordArrival(const std::string& emergencyId, const std::string& hospitalId, uint64_t monoMs = 0);

    /**
     * @brief Record hospital formal acceptance by receiving clinical doctor.
     */
    bool RecordHospitalAcceptance(const std::string& emergencyId, const std::string& receivingDoctor, uint64_t monoMs = 0);

    /**
     * @brief Record treatment started in ER, ICU, or Trauma Suite.
     */
    bool RecordTreatmentStarted(const std::string& emergencyId, const std::string& roomOrBed, uint64_t monoMs = 0);

    /**
     * @brief Mark mission completed and free active staging.
     */
    bool CompleteMission(const std::string& emergencyId, uint64_t monoMs = 0);

    /**
     * @brief Finalize and archive mission.
     */
    bool ArchiveMission(const std::string& emergencyId, uint64_t monoMs = 0);

    std::optional<HandoverRecord> GetHandoverRecord(const std::string& emergencyId) const;
    bool IsHandoverComplete(const std::string& emergencyId) const;

    void Reset();

private:
    std::shared_ptr<orchestration::IEventBus> eventBus_;
    std::shared_ptr<orchestration::EmergencySessionManager> sessionManager_;
    std::shared_ptr<control_center::IncidentManager> incidentManager_;
    std::shared_ptr<BedAvailabilityManager> bedManager_;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, HandoverRecord> records_;
};

} // namespace aegis::coordinator::hospital

