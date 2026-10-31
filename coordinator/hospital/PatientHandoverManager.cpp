/**
 * @file PatientHandoverManager.cpp
 * @brief Implementation of patient handover lifecycle and mission closure.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/hospital/PatientHandoverManager.hpp"
#include "common/utils/TimeUtils.hpp"

namespace aegis::coordinator::hospital {

PatientHandoverManager::PatientHandoverManager(std::shared_ptr<orchestration::IEventBus> eventBus,
                                               std::shared_ptr<orchestration::EmergencySessionManager> sessionManager,
                                               std::shared_ptr<control_center::IncidentManager> incidentManager,
                                               std::shared_ptr<BedAvailabilityManager> bedManager)
    : eventBus_(std::move(eventBus)),
      sessionManager_(std::move(sessionManager)),
      incidentManager_(std::move(incidentManager)),
      bedManager_(std::move(bedManager)) {}

bool PatientHandoverManager::RecordArrival(const std::string& emergencyId,
                                          const std::string& hospitalId,
                                          uint64_t monoMs) {
    if (emergencyId.empty()) return false;
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = records_.find(emergencyId);
    if (it != records_.end()) {
        it->second.status = HandoverStatus::ARRIVED_AT_BAY;
        it->second.arrivalMonoMs = monoMs;
        it->second.hospitalId = hospitalId;
    } else {
        HandoverRecord rec;
        rec.emergencyId = emergencyId;
        rec.hospitalId = hospitalId;
        rec.status = HandoverStatus::ARRIVED_AT_BAY;
        rec.arrivalMonoMs = monoMs;
        records_[emergencyId] = rec;
    }

    if (incidentManager_) {
        auto optInc = incidentManager_->GetIncidentByEmergencyId(emergencyId);
        if (optInc.has_value()) {
            incidentManager_->UpdateState(optInc->incidentId, control_center::IncidentState::AT_HOSPITAL, "Arrived at bay", monoMs);
        }
    }

    return true;
}

bool PatientHandoverManager::RecordHospitalAcceptance(const std::string& emergencyId,
                                                      const std::string& receivingDoctor,
                                                      uint64_t monoMs) {
    if (emergencyId.empty()) return false;
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = records_.find(emergencyId);
    if (it == records_.end()) return false;

    it->second.status = HandoverStatus::ACCEPTED_BY_HOSPITAL;
    it->second.receivingDoctor = receivingDoctor;
    it->second.acceptedMonoMs = monoMs;

    return true;
}

bool PatientHandoverManager::RecordTreatmentStarted(const std::string& emergencyId,
                                                   const std::string& roomOrBed,
                                                   uint64_t monoMs) {
    if (emergencyId.empty()) return false;
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();

    std::string hospId;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = records_.find(emergencyId);
        if (it == records_.end()) return false;

        it->second.status = HandoverStatus::TREATMENT_STARTED;
        it->second.assignedRoomOrBed = roomOrBed;
        it->second.treatmentStartedMonoMs = monoMs;
        hospId = it->second.hospitalId;
    }

    if (bedManager_ && !hospId.empty()) {
        bedManager_->ConfirmAdmission(hospId, emergencyId);
    }

    return true;
}

bool PatientHandoverManager::CompleteMission(const std::string& emergencyId, uint64_t monoMs) {
    if (emergencyId.empty()) return false;
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();

    std::string ambId;
    std::string hospId;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = records_.find(emergencyId);
        if (it == records_.end()) return false;

        it->second.status = HandoverStatus::HANDOVER_COMPLETED;
        it->second.completedMonoMs = monoMs;
        ambId = it->second.ambulanceId;
        hospId = it->second.hospitalId;
    }

    if (sessionManager_) {
        sessionManager_->CompleteSession(emergencyId, monoMs);
    }

    if (incidentManager_) {
        auto optInc = incidentManager_->GetIncidentByEmergencyId(emergencyId);
        if (optInc.has_value()) {
            incidentManager_->CompleteIncident(optInc->incidentId, monoMs);
        }
    }

    if (eventBus_) {
        eventBus_->Publish(orchestration::Event(
            orchestration::EventType::MissionCompleted,
            emergencyId,
            hospId,
            "HandoverComplete: Amb=" + ambId,
            monoMs
        ));
    }

    return true;
}

bool PatientHandoverManager::ArchiveMission(const std::string& emergencyId, uint64_t monoMs) {
    if (emergencyId.empty()) return false;
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = records_.find(emergencyId);
        if (it == records_.end()) return false;

        it->second.status = HandoverStatus::MISSION_ARCHIVED;
        it->second.archivedMonoMs = monoMs;
    }

    if (sessionManager_) {
        sessionManager_->ArchiveSession(emergencyId, monoMs);
    }

    if (incidentManager_) {
        auto optInc = incidentManager_->GetIncidentByEmergencyId(emergencyId);
        if (optInc.has_value()) {
            incidentManager_->ArchiveIncident(optInc->incidentId, monoMs);
        }
    }

    if (eventBus_) {
        eventBus_->Publish(orchestration::Event(
            orchestration::EventType::MissionArchived,
            emergencyId,
            "",
            "Archived & Closed",
            monoMs
        ));
    }

    return true;
}

std::optional<HandoverRecord> PatientHandoverManager::GetHandoverRecord(const std::string& emergencyId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = records_.find(emergencyId);
    if (it != records_.end()) return it->second;
    return std::nullopt;
}

bool PatientHandoverManager::IsHandoverComplete(const std::string& emergencyId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = records_.find(emergencyId);
    if (it != records_.end()) {
        return (it->second.status == HandoverStatus::HANDOVER_COMPLETED ||
                it->second.status == HandoverStatus::MISSION_ARCHIVED);
    }
    return false;
}

void PatientHandoverManager::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    records_.clear();
}

} // namespace aegis::coordinator::hospital

