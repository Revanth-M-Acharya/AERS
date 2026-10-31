/**
 * @file EmergencyControlCenter.hpp
 * @brief Master control center orchestrator uniting call intake, triage, resource allocation, and hospital delivery.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/control_center/EmergencyValidator.hpp"
#include "coordinator/control_center/EmergencyPriorityEngine.hpp"
#include "coordinator/control_center/IncidentManager.hpp"
#include "coordinator/control_center/ResourceAllocator.hpp"
#include "coordinator/control_center/DispatcherConsole.hpp"
#include "coordinator/hospital/HospitalRegistry.hpp"
#include "coordinator/hospital/HospitalSelectionEngine.hpp"
#include "coordinator/hospital/BedAvailabilityManager.hpp"
#include "coordinator/hospital/HospitalNotificationManager.hpp"
#include "coordinator/hospital/DoctorPreparationManager.hpp"
#include "coordinator/hospital/PatientHandoverManager.hpp"

#include "coordinator/orchestration/state/CoordinatorStateManager.hpp"
#include "coordinator/orchestration/event_bus/IEventBus.hpp"
#include "coordinator/orchestration/session/EmergencySessionManager.hpp"
#include "coordinator/orchestration/fsm/CoordinatorFSM.hpp"

#include <string>
#include <memory>
#include <atomic>
#include <utility>

namespace aegis::coordinator::control_center {

struct EmergencyIntakeResult {
    bool success{false};
    std::string emergencyId;
    std::string incidentId;
    PriorityLevel priority{PriorityLevel::Priority1};
    AmbulanceClass ambulanceClass{AmbulanceClass::ALS};
    std::string assignedAmbulanceId;
    std::string assignedHospitalId;
    std::string assignedDispatcherId;
    std::string statusMessage;
};

class EmergencyControlCenter {
public:
    EmergencyControlCenter(std::shared_ptr<orchestration::CoordinatorStateManager> stateManager,
                           std::shared_ptr<orchestration::IEventBus> eventBus,
                           std::shared_ptr<orchestration::EmergencySessionManager> sessionManager,
                           std::shared_ptr<orchestration::CoordinatorFSM> fsm = nullptr);
    ~EmergencyControlCenter() = default;

    /**
     * @brief Ingest, validate, prioritize, select hospital, allocate resources, and activate mission.
     */
    EmergencyIntakeResult ProcessEmergencyCall(const EmergencyRequest& req, uint64_t monoMs = 0);

    /**
     * @brief Handle hospital failover or rerouting.
     */
    bool HandleHospitalFailover(const std::string& emergencyId, const std::string& failedHospitalId, uint64_t monoMs = 0);

    /**
     * @brief Cancel an active emergency call.
     */
    bool CancelEmergency(const std::string& emergencyId, const std::string& reason, uint64_t monoMs = 0);

    // Subsystem Accessors
    std::shared_ptr<EmergencyValidator> GetValidator() const { return validator_; }
    std::shared_ptr<EmergencyPriorityEngine> GetPriorityEngine() const { return priorityEngine_; }
    std::shared_ptr<IncidentManager> GetIncidentManager() const { return incidentManager_; }
    std::shared_ptr<ResourceAllocator> GetResourceAllocator() const { return resourceAllocator_; }
    std::shared_ptr<DispatcherConsole> GetDispatcherConsole() const { return dispatcherConsole_; }
    std::shared_ptr<hospital::HospitalRegistry> GetHospitalRegistry() const { return hospitalRegistry_; }
    std::shared_ptr<hospital::HospitalSelectionEngine> GetHospitalSelectionEngine() const { return hospitalSelectionEngine_; }
    std::shared_ptr<hospital::BedAvailabilityManager> GetBedManager() const { return bedManager_; }
    std::shared_ptr<hospital::HospitalNotificationManager> GetNotificationManager() const { return notificationManager_; }
    std::shared_ptr<hospital::DoctorPreparationManager> GetDoctorPrepManager() const { return doctorPrepManager_; }
    std::shared_ptr<hospital::PatientHandoverManager> GetHandoverManager() const { return handoverManager_; }

    void Reset();

private:
    std::string GenerateEmergencyId();
    std::string GenerateIncidentId();

    std::shared_ptr<orchestration::CoordinatorStateManager> stateManager_;
    std::shared_ptr<orchestration::IEventBus> eventBus_;
    std::shared_ptr<orchestration::EmergencySessionManager> sessionManager_;
    std::shared_ptr<orchestration::CoordinatorFSM> fsm_;

    std::shared_ptr<EmergencyValidator> validator_;
    std::shared_ptr<EmergencyPriorityEngine> priorityEngine_;
    std::shared_ptr<IncidentManager> incidentManager_;
    std::shared_ptr<ResourceAllocator> resourceAllocator_;
    std::shared_ptr<DispatcherConsole> dispatcherConsole_;

    std::shared_ptr<hospital::HospitalRegistry> hospitalRegistry_;
    std::shared_ptr<hospital::HospitalSelectionEngine> hospitalSelectionEngine_;
    std::shared_ptr<hospital::BedAvailabilityManager> bedManager_;
    std::shared_ptr<hospital::HospitalNotificationManager> notificationManager_;
    std::shared_ptr<hospital::DoctorPreparationManager> doctorPrepManager_;
    std::shared_ptr<hospital::PatientHandoverManager> handoverManager_;

    std::atomic<uint32_t> nextEmergencySeq_{1001};
    std::atomic<uint32_t> nextIncidentSeq_{2001};
};

} // namespace aegis::coordinator::control_center

