/**
 * @file EmergencyControlCenter.cpp
 * @brief Implementation of Emergency Control Center intake pipeline and resource assignment.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/control_center/EmergencyControlCenter.hpp"
#include "common/utils/TimeUtils.hpp"

namespace aegis::coordinator::control_center {

EmergencyControlCenter::EmergencyControlCenter(std::shared_ptr<orchestration::CoordinatorStateManager> stateManager,
                                               std::shared_ptr<orchestration::IEventBus> eventBus,
                                               std::shared_ptr<orchestration::EmergencySessionManager> sessionManager,
                                               std::shared_ptr<orchestration::CoordinatorFSM> fsm)
    : stateManager_(std::move(stateManager)),
      eventBus_(std::move(eventBus)),
      sessionManager_(std::move(sessionManager)),
      fsm_(std::move(fsm)) {

    validator_ = std::make_shared<EmergencyValidator>();
    priorityEngine_ = std::make_shared<EmergencyPriorityEngine>();
    incidentManager_ = std::make_shared<IncidentManager>();
    resourceAllocator_ = std::make_shared<ResourceAllocator>(eventBus_);
    dispatcherConsole_ = std::make_shared<DispatcherConsole>(incidentManager_);

    hospitalRegistry_ = std::make_shared<hospital::HospitalRegistry>(stateManager_);
    hospitalSelectionEngine_ = std::make_shared<hospital::HospitalSelectionEngine>(hospitalRegistry_, eventBus_, sessionManager_);
    bedManager_ = std::make_shared<hospital::BedAvailabilityManager>(hospitalRegistry_);
    notificationManager_ = std::make_shared<hospital::HospitalNotificationManager>(eventBus_);
    doctorPrepManager_ = std::make_shared<hospital::DoctorPreparationManager>();
    handoverManager_ = std::make_shared<hospital::PatientHandoverManager>(eventBus_, sessionManager_, incidentManager_, bedManager_);
}

std::string EmergencyControlCenter::GenerateEmergencyId() {
    return "EMG-" + std::to_string(nextEmergencySeq_++);
}

std::string EmergencyControlCenter::GenerateIncidentId() {
    return "INC-" + std::to_string(nextIncidentSeq_++);
}

EmergencyIntakeResult EmergencyControlCenter::ProcessEmergencyCall(const EmergencyRequest& req, uint64_t monoMs) {
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();
    EmergencyIntakeResult result;

    // 1. Validation
    ValidationResult val = validator_->Validate(req);
    if (!val.isValid) {
        result.success = false;
        result.statusMessage = "Validation Rejected: " + val.rejectionReason;
        return result;
    }

    std::string emgId = GenerateEmergencyId();
    std::string incId = GenerateIncidentId();
    result.emergencyId = emgId;
    result.incidentId = incId;

    // 2. Priority Calculation
    PriorityLevel prio = priorityEngine_->CalculatePriority(req);
    AmbulanceClass ambClass = priorityEngine_->DetermineAmbulanceClass(req, prio);
    result.priority = prio;
    result.ambulanceClass = ambClass;

    if (eventBus_) {
        eventBus_->Publish(orchestration::Event(
            orchestration::EventType::EmergencyCreated,
            emgId,
            req.callerId,
            "Type=" + EmergencyTypeToString(req.type) + ";Severity=" + PatientSeverityToString(req.severity),
            monoMs
        ));
        eventBus_->Publish(orchestration::Event(
            orchestration::EventType::EmergencyValidated,
            emgId,
            incId,
            "Priority=" + PriorityLevelToString(prio) + ";Class=" + AmbulanceClassToString(ambClass),
            monoMs
        ));
    }

    // 3. Hospital Selection
    auto optHosp = hospitalSelectionEngine_->SelectBestHospital(req.type, prio, req.latitude, req.longitude);
    if (!optHosp.has_value()) {
        result.success = false;
        result.statusMessage = "No available hospital with required capability or capacity";
        return result;
    }
    std::string hospId = optHosp->hospitalId;
    result.assignedHospitalId = hospId;

    // 4. Ambulance Allocation
    std::string primaryAmbId;
    if (stateManager_) {
        auto allAmbs = stateManager_->GetAllAmbulances();
        for (const auto& amb : allAmbs) {
            if (amb.isActive && !resourceAllocator_->IsAmbulanceBusy(amb.ambulanceId)) {
                primaryAmbId = amb.ambulanceId;
                break;
            }
        }
    }
    if (primaryAmbId.empty()) {
        primaryAmbId = "AMB-01"; // Default cluster unit
    }
    result.assignedAmbulanceId = primaryAmbId;

    // 5. Dispatcher Allocation
    std::string dispId = dispatcherConsole_->GetNextAvailableDispatcher();
    if (dispId.empty()) {
        dispId = "DISP-AUTO";
    }
    result.assignedDispatcherId = dispId;

    // 6. Resource Mutual Exclusion Reservation
    resourceAllocator_->Allocate(incId, primaryAmbId, "", hospId, dispId, {}, monoMs);
    dispatcherConsole_->AssignIncident(dispId, incId);

    // 7. Bed Reservation
    hospital::BedType bedType = (prio == PriorityLevel::Priority1) ?
        hospital::BedType::ICU : hospital::BedType::GENERAL_EMERGENCY;
    bedManager_->ReserveBed(hospId, bedType, emgId, monoMs);

    // 8. Incident Creation
    IncidentRecord incRec;
    incRec.incidentId = incId;
    incRec.emergencyId = emgId;
    incRec.type = req.type;
    incRec.priority = prio;
    incRec.severity = req.severity;
    incRec.ambulanceClass = ambClass;
    incRec.assignedAmbulanceId = primaryAmbId;
    incRec.assignedHospitalId = hospId;
    incRec.assignedDispatcherId = dispId;
    incRec.originIntersectionId = req.closestIntersectionId;
    incRec.destinationHospitalId = hospId;
    incRec.state = IncidentState::DISPATCHED;
    incRec.createdAtMonoMs = monoMs;
    incidentManager_->CreateIncident(incRec);

    // 9. Emergency Session Creation in Coordinator State Manager
    if (sessionManager_) {
        sessionManager_->CreateSession(emgId, primaryAmbId, hospId, static_cast<uint32_t>(prio), monoMs);
        sessionManager_->ActivateSession(emgId, monoMs);
    }

    // 10. Hospital Notification
    hospital::HospitalNotification notif;
    notif.type = hospital::HospitalNotificationType::INCOMING_PATIENT;
    notif.emergencyId = emgId;
    notif.hospitalId = hospId;
    notif.ambulanceId = primaryAmbId;
    notif.patientCondition = PatientSeverityToString(req.severity);
    notif.priorityLevel = static_cast<uint32_t>(prio);
    notif.requiredDepartment = priorityEngine_->DetermineRequiredDepartment(req.type);
    notificationManager_->SendNotification(notif, monoMs);

    // 11. Doctor Team Preparation Trigger
    doctorPrepManager_->StartPreparation(emgId, hospId, "Dr. OnCall Trauma Specialist", notif.requiredDepartment, monoMs);

    // 12. Advance Coordinator FSM to ROUTE_PLANNING if in IDLE
    if (fsm_ && fsm_->GetCurrentState() == orchestration::CoordinatorState::IDLE) {
        fsm_->TransitionTo(orchestration::CoordinatorState::ROUTE_PLANNING, "Emergency intake assigned: " + emgId);
    }

    if (eventBus_) {
        eventBus_->Publish(orchestration::Event(
            orchestration::EventType::EmergencyAssigned,
            emgId,
            primaryAmbId,
            "Assigned to " + primaryAmbId + " heading to " + hospId,
            monoMs
        ));
        eventBus_->Publish(orchestration::Event(
            orchestration::EventType::HospitalAssigned,
            emgId,
            hospId,
            "Department=" + notif.requiredDepartment,
            monoMs
        ));
    }

    result.success = true;
    result.statusMessage = "Emergency processed and active: " + emgId;
    return result;
}

bool EmergencyControlCenter::HandleHospitalFailover(const std::string& emergencyId,
                                                    const std::string& failedHospitalId,
                                                    uint64_t monoMs) {
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();

    // Release old bed
    bedManager_->ReleaseBed(failedHospitalId, emergencyId);

    // Divert via selection engine
    auto optNewHosp = hospitalSelectionEngine_->HandleHospitalDivert(emergencyId, failedHospitalId, 0.0, 0.0, monoMs);
    if (!optNewHosp.has_value()) return false;

    std::string newHospId = optNewHosp->hospitalId;

    // Reserve bed at new hospital
    bedManager_->ReserveBed(newHospId, hospital::BedType::ICU, emergencyId, monoMs);

    // Update incident
    auto optInc = incidentManager_->GetIncidentByEmergencyId(emergencyId);
    if (optInc.has_value()) {
        incidentManager_->AssignResources(optInc->incidentId, "", newHospId);
        resourceAllocator_->ReassignHospital(optInc->incidentId, newHospId, monoMs);
    }

    // Send diverted notification
    hospital::HospitalNotification notif;
    notif.type = hospital::HospitalNotificationType::ROUTE_CHANGED;
    notif.emergencyId = emergencyId;
    notif.hospitalId = newHospId;
    notif.details = "Diverted from failed facility " + failedHospitalId;
    notificationManager_->SendNotification(notif, monoMs);

    // Start prep at new hospital
    doctorPrepManager_->StartPreparation(emergencyId, newHospId, "Dr. Backup Trauma Specialist", "TRAUMA", monoMs);

    return true;
}

bool EmergencyControlCenter::CancelEmergency(const std::string& emergencyId,
                                             const std::string& reason,
                                             uint64_t monoMs) {
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();

    auto optInc = incidentManager_->GetIncidentByEmergencyId(emergencyId);
    if (optInc.has_value()) {
        incidentManager_->CancelIncident(optInc->incidentId, reason, monoMs);
        resourceAllocator_->Release(optInc->incidentId, monoMs);
        dispatcherConsole_->UnassignIncident(optInc->assignedDispatcherId, optInc->incidentId);
        bedManager_->ReleaseBed(optInc->assignedHospitalId, emergencyId);

        hospital::HospitalNotification notif;
        notif.type = hospital::HospitalNotificationType::MISSION_CANCELLED;
        notif.emergencyId = emergencyId;
        notif.hospitalId = optInc->assignedHospitalId;
        notif.details = reason;
        notificationManager_->SendNotification(notif, monoMs);
    }

    if (sessionManager_) {
        sessionManager_->CancelSession(emergencyId, reason, monoMs);
    }

    return true;
}

void EmergencyControlCenter::Reset() {
    validator_ = std::make_shared<EmergencyValidator>();
    priorityEngine_ = std::make_shared<EmergencyPriorityEngine>();
    incidentManager_->Reset();
    resourceAllocator_->Reset();
    dispatcherConsole_->Reset();
    hospitalRegistry_->Reset();
    bedManager_->Reset();
    notificationManager_->Reset();
    doctorPrepManager_->Reset();
    handoverManager_->Reset();
    nextEmergencySeq_ = 1001;
    nextIncidentSeq_ = 2001;
}

} // namespace aegis::coordinator::control_center

