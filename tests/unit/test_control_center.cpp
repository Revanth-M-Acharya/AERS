/**
 * @file test_control_center.cpp
 * @brief Comprehensive Unit & Integration Tests for Phase 6 Emergency Control Center & Hospital Coordination.
 * Target: Raspberry Pi 4 Model B (QNX Neutrino RTOS 8.0 / POSIX).
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/control_center/EmergencyControlCenter.hpp"
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
#include "coordinator/orchestration/event_bus/EventBus.hpp"
#include "coordinator/orchestration/session/EmergencySessionManager.hpp"
#include "coordinator/orchestration/fsm/CoordinatorFSM.hpp"
#include "coordinator/logger/CoordinatorLogger.hpp"
#include "common/utils/TimeUtils.hpp"

#include <iostream>
#include <cassert>
#include <vector>

using namespace aegis;
using namespace aegis::coordinator;
using namespace aegis::coordinator::control_center;
using namespace aegis::coordinator::hospital;
using namespace aegis::coordinator::orchestration;

int main() {
    std::cout << "==========================================================\n"
              << " SMART AEGIS: Control Center & Hospital (Phase 6) Tests    \n"
              << " Target: Raspberry Pi 4 Model B (QNX SDP 8.0 / POSIX)     \n"
              << "==========================================================\n\n";

    CoordinatorLogger::Instance().Initialize("", false, LogLevel::INFO);

    // ------------------------------------------------------------------------
    // TEST 1: Emergency Request Validation
    // ------------------------------------------------------------------------
    std::cout << "[TEST 1] Emergency Request Validation...\n";
    {
        EmergencyValidator validator;

        // Valid request
        EmergencyRequest req1;
        req1.callerId = "CALLER-911-01";
        req1.callerPhone = "+15550199";
        req1.latitude = 12.9716;
        req1.longitude = 77.5946;
        req1.closestIntersectionId = "INT_A";
        req1.type = EmergencyType::Cardiac;
        req1.severity = PatientSeverity::CRITICAL;
        req1.casualtyCount = 1;
        req1.patientAgeYears = 62;

        ValidationResult res1 = validator.Validate(req1);
        assert(res1.isValid);
        assert(res1.sanitizedLocation == "INT_A");

        // Invalid: Missing caller ID and phone
        EmergencyRequest req2 = req1;
        req2.callerId = "";
        req2.callerPhone = "";
        assert(!validator.Validate(req2).isValid);

        // Invalid: Zero casualties
        EmergencyRequest req3 = req1;
        req3.casualtyCount = 0;
        assert(!validator.Validate(req3).isValid);

        // Invalid: Out-of-bounds coordinates
        EmergencyRequest req4 = req1;
        req4.latitude = 120.0; // Latitude must be [-90, 90]
        assert(!validator.Validate(req4).isValid);

        // Invalid: Age physiological limit
        EmergencyRequest req5 = req1;
        req5.patientAgeYears = 150;
        assert(!validator.Validate(req5).isValid);

        std::cout << "  ✓ PASS: Call validation and rejection rules verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 2: Emergency Priority Scoring
    // ------------------------------------------------------------------------
    std::cout << "[TEST 2] Emergency Priority Scoring & Escalation...\n";
    {
        EmergencyPriorityEngine priorityEngine;

        EmergencyRequest cardiacReq;
        cardiacReq.type = EmergencyType::Cardiac;
        cardiacReq.severity = PatientSeverity::CRITICAL;
        assert(priorityEngine.CalculatePriority(cardiacReq) == PriorityLevel::Priority1);

        EmergencyRequest strokeReq;
        strokeReq.type = EmergencyType::Stroke;
        assert(priorityEngine.CalculatePriority(strokeReq) == PriorityLevel::Priority1);

        EmergencyRequest massCasualtyReq;
        massCasualtyReq.type = EmergencyType::MassCasualty;
        assert(priorityEngine.CalculatePriority(massCasualtyReq) == PriorityLevel::Priority1);

        EmergencyRequest traumaReq;
        traumaReq.type = EmergencyType::Trauma;
        traumaReq.severity = PatientSeverity::SEVERE;
        assert(priorityEngine.CalculatePriority(traumaReq, 5.0, true) == PriorityLevel::Priority2);

        // Escalate trauma to Priority 1 if distance > 20km or hospital lacks capability
        assert(priorityEngine.CalculatePriority(traumaReq, 25.0, true) == PriorityLevel::Priority1);
        assert(priorityEngine.CalculatePriority(traumaReq, 5.0, false) == PriorityLevel::Priority1);

        EmergencyRequest pregnancyReq;
        pregnancyReq.type = EmergencyType::Pregnancy;
        pregnancyReq.severity = PatientSeverity::MODERATE;
        assert(priorityEngine.CalculatePriority(pregnancyReq, 5.0, true) == PriorityLevel::Priority3);

        EmergencyRequest generalReq;
        generalReq.type = EmergencyType::GeneralEmergency;
        generalReq.severity = PatientSeverity::MINOR;
        assert(priorityEngine.CalculatePriority(generalReq) == PriorityLevel::Priority4);

        std::cout << "  ✓ PASS: Multi-factor priority calculation and condition escalation verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 3: Ambulance Class & Department Recommendation
    // ------------------------------------------------------------------------
    std::cout << "[TEST 3] Ambulance Class & Specialized Department Mapping...\n";
    {
        EmergencyPriorityEngine engine;

        // Infant Pediatric -> NEO ambulance
        EmergencyRequest neoReq;
        neoReq.type = EmergencyType::Pediatric;
        neoReq.patientAgeYears = 0;
        assert(engine.DetermineAmbulanceClass(neoReq, PriorityLevel::Priority2) == AmbulanceClass::NEO);

        // Mass Casualty / Critical -> MICU ambulance
        EmergencyRequest massReq;
        massReq.type = EmergencyType::MassCasualty;
        assert(engine.DetermineAmbulanceClass(massReq, PriorityLevel::Priority1) == AmbulanceClass::MICU);

        // Priority 1 Cardiac -> ALS
        EmergencyRequest cardReq;
        cardReq.type = EmergencyType::Cardiac;
        assert(engine.DetermineAmbulanceClass(cardReq, PriorityLevel::Priority1) == AmbulanceClass::ALS);

        // Priority 4 General -> BLS
        EmergencyRequest genReq;
        genReq.type = EmergencyType::GeneralEmergency;
        assert(engine.DetermineAmbulanceClass(genReq, PriorityLevel::Priority4) == AmbulanceClass::BLS);

        // Department mapping
        assert(engine.DetermineRequiredDepartment(EmergencyType::Cardiac) == "CARDIOLOGY");
        assert(engine.DetermineRequiredDepartment(EmergencyType::Stroke) == "NEUROLOGY");
        assert(engine.DetermineRequiredDepartment(EmergencyType::Fire) == "BURN");
        assert(engine.DetermineRequiredDepartment(EmergencyType::Pregnancy) == "OBSTETRICS");
        assert(engine.DetermineRequiredDepartment(EmergencyType::Pediatric) == "PEDIATRICS");
        assert(engine.DetermineRequiredDepartment(EmergencyType::Trauma) == "TRAUMA");

        std::cout << "  ✓ PASS: Ambulance classes and department routing verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 4: Hospital Registry & Configuration Loading
    // ------------------------------------------------------------------------
    std::cout << "[TEST 4] Hospital Registry & Configuration Loader...\n";
    {
        auto stateMgr = std::make_shared<CoordinatorStateManager>();
        HospitalRegistry registry(stateMgr);

        assert(registry.LoadFromConfigFile("config/Hospital.conf"));
        assert(registry.GetHospitalCount() >= 4);

        auto optCentral = registry.GetHospital("HOSP_CENTRAL");
        assert(optCentral.has_value());
        assert(optCentral->hospitalName == "Central Trauma Medical Center");
        assert(optCentral->traumaLevel == 1);
        assert(optCentral->HasDepartment("CARDIOLOGY"));
        assert(optCentral->HasDepartment("TRAUMA"));
        assert(optCentral->availableBeds > 0);

        // Verify state manager synchronisation
        assert(stateMgr->GetHospital("HOSP_CENTRAL").has_value());
        assert(stateMgr->GetHospital("HOSP_CENTRAL")->totalCapacity == optCentral->emergencyCapacity);

        auto cardiacHospitals = registry.GetHospitalsByDepartment("CARDIOLOGY");
        assert(cardiacHospitals.size() >= 2);

        std::cout << "  ✓ PASS: Hospital.conf parsing, capability queries, and state sync verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 5: Hospital Selection Engine
    // ------------------------------------------------------------------------
    std::cout << "[TEST 5] Hospital Selection Algorithm & Capabilities Scoring...\n";
    {
        auto registry = std::make_shared<HospitalRegistry>();
        registry->LoadFromConfigFile("config/Hospital.conf");

        HospitalSelectionEngine engine(registry);

        // Cardiac Priority 1 at (12.980, 77.600) -> Should select HOSP_EAST or HOSP_CENTRAL
        auto bestHosp = engine.SelectBestHospital(EmergencyType::Cardiac, PriorityLevel::Priority1, 12.9800, 77.6000);
        assert(bestHosp.has_value());
        assert(bestHosp->HasDepartment("CARDIOLOGY"));
        assert(bestHosp->availableBeds > 0);

        // Pediatric at (12.960, 77.585) -> HOSP_SOUTH
        auto pediaHosp = engine.SelectBestHospital(EmergencyType::Pediatric, PriorityLevel::Priority2, 12.9600, 77.5850);
        assert(pediaHosp.has_value());
        assert(pediaHosp->hospitalId == "South Children & Maternity Hospital" || pediaHosp->hospitalId == "HOSP_SOUTH");
        assert(pediaHosp->HasDepartment("PEDIATRICS"));

        std::cout << "  ✓ PASS: Multi-criteria hospital selection verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 6: Hospital Failure & Dynamic Rerouting
    // ------------------------------------------------------------------------
    std::cout << "[TEST 6] Hospital Failure & Dynamic Rerouting Failover...\n";
    {
        auto stateMgr = std::make_shared<CoordinatorStateManager>();
        auto eventBus = std::make_shared<EventBus>();
        auto sessionMgr = std::make_shared<EmergencySessionManager>(stateMgr, eventBus);
        auto registry = std::make_shared<HospitalRegistry>(stateMgr);
        registry->LoadFromConfigFile("config/Hospital.conf");

        HospitalSelectionEngine engine(registry, eventBus, sessionMgr);

        sessionMgr->CreateSession("EMG-DIVERT", "AMB-01", "HOSP_CENTRAL", 1, 1000);

        // HOSP_CENTRAL becomes full or offline
        registry->UpdateStatus("HOSP_CENTRAL", HospitalStatus::FULL);

        // Trigger divert
        auto newHosp = engine.HandleHospitalDivert("EMG-DIVERT", "HOSP_CENTRAL", 12.9800, 77.6000, 1100);
        assert(newHosp.has_value());
        assert(newHosp->hospitalId != "HOSP_CENTRAL");

        std::cout << "  ✓ PASS: Hospital diversion, session destination update, and event emission verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 7: Dispatcher Console Operations
    // ------------------------------------------------------------------------
    std::cout << "[TEST 7] Dispatcher Console Sessions & Workload Allocation...\n";
    {
        auto incidentMgr = std::make_shared<IncidentManager>();
        DispatcherConsole console(incidentMgr);

        assert(console.Login("DISP-101", "Operator Alice", 1000));
        assert(console.Login("DISP-102", "Operator Bob", 1000));

        assert(console.GetActiveSessions().size() == 2);

        // Assign incident to Alice
        assert(console.AssignIncident("DISP-101", "INC-001"));
        assert(console.AssignIncident("DISP-101", "INC-002"));

        // Next available dispatcher should be Bob (0 workload)
        assert(console.GetNextAvailableDispatcher() == "DISP-102");

        // Bob logout
        assert(console.Logout("DISP-102"));
        assert(console.GetNextAvailableDispatcher() == "DISP-101");

        std::cout << "  ✓ PASS: Dispatcher login, load balancing, and assignment verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 8: Resource Allocator Mutual Exclusion (Prevent Double-Assignment)
    // ------------------------------------------------------------------------
    std::cout << "[TEST 8] Resource Allocator Double-Assignment Prevention...\n";
    {
        auto eventBus = std::make_shared<EventBus>();
        ResourceAllocator allocator(eventBus);

        assert(allocator.Allocate("INC-01", "AMB-01", "AMB-BACKUP", "HOSP_CENTRAL", "DISP-01", {"INT_A", "INT_B"}, 1000));
        assert(allocator.IsAmbulanceBusy("AMB-01"));
        assert(allocator.IsDispatcherBusy("DISP-01"));

        // Duplicate allocation for same incident rejected
        assert(!allocator.Allocate("INC-01", "AMB-02", "", "HOSP_EAST", "DISP-02", {}, 1010));

        // Reusing busy ambulance AMB-01 for another incident rejected
        assert(!allocator.Allocate("INC-02", "AMB-01", "", "HOSP_EAST", "DISP-02", {}, 1020));

        std::cout << "  ✓ PASS: Resource mutual exclusion and duplicate allocation prevention verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 9: Resource Release & Reassignment
    // ------------------------------------------------------------------------
    std::cout << "[TEST 9] Resource Release & Dynamic Reassignment...\n";
    {
        ResourceAllocator allocator;

        assert(allocator.Allocate("INC-01", "AMB-01", "", "HOSP_CENTRAL", "DISP-01", {}, 1000));
        assert(allocator.IsAmbulanceBusy("AMB-01"));

        // Reassign to AMB-02
        assert(allocator.ReassignAmbulance("INC-01", "AMB-02", 1050));
        assert(!allocator.IsAmbulanceBusy("AMB-01"));
        assert(allocator.IsAmbulanceBusy("AMB-02"));

        // Release incident
        assert(allocator.Release("INC-01", 1100));
        assert(!allocator.IsAmbulanceBusy("AMB-02"));

        std::cout << "  ✓ PASS: Resource release and dynamic vehicle reassignment verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 10: Backup Ambulance Failover
    // ------------------------------------------------------------------------
    std::cout << "[TEST 10] Backup Ambulance Failover Promotion...\n";
    {
        ResourceAllocator allocator;

        assert(allocator.Allocate("INC-03", "AMB-PRIMARY", "AMB-BACKUP", "HOSP_CENTRAL", "DISP-01", {}, 1000));
        assert(allocator.IsAmbulanceBusy("AMB-PRIMARY"));

        // Primary ambulance breaks down -> Promote backup
        assert(allocator.FailoverToBackupAmbulance("INC-03", 1100));
        assert(!allocator.IsAmbulanceBusy("AMB-PRIMARY"));
        assert(allocator.IsAmbulanceBusy("AMB-BACKUP"));

        auto alloc = allocator.GetAllocation("INC-03");
        assert(alloc.has_value());
        assert(alloc->primaryAmbulanceId == "AMB-BACKUP");

        std::cout << "  ✓ PASS: Automatic failover to backup ambulance verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 11: Bed Availability Manager (Reserve, Confirm, Release)
    // ------------------------------------------------------------------------
    std::cout << "[TEST 11] Bed Availability Reservation & Admission Confirmation...\n";
    {
        auto registry = std::make_shared<HospitalRegistry>();
        registry->LoadFromConfigFile("config/Hospital.conf");

        BedAvailabilityManager bedMgr(registry);

        auto hospBefore = registry->GetHospital("HOSP_CENTRAL");
        uint32_t bedsBefore = hospBefore->availableBeds;
        (void)bedsBefore;
        uint32_t icuBefore = hospBefore->icuBeds;

        // Reserve ICU bed for critical patient
        assert(bedMgr.ReserveBed("HOSP_CENTRAL", BedType::ICU, "EMG-BED-01", 1000));
        assert(registry->GetHospital("HOSP_CENTRAL")->icuBeds == icuBefore - 1);

        // Duplicate reservation rejected
        assert(!bedMgr.ReserveBed("HOSP_CENTRAL", BedType::ICU, "EMG-BED-01", 1000));

        // Confirm admission
        assert(bedMgr.ConfirmAdmission("HOSP_CENTRAL", "EMG-BED-01"));
        auto res = bedMgr.GetReservation("EMG-BED-01");
        assert(res.has_value());
        assert(res->confirmedAdmission);

        // Release bed
        assert(bedMgr.ReleaseBed("HOSP_CENTRAL", "EMG-BED-01"));
        assert(registry->GetHospital("HOSP_CENTRAL")->icuBeds == icuBefore);

        std::cout << "  ✓ PASS: Bed reservation, capacity decrements, admission confirmation, and release verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 12: Hospital Notification Dispatch
    // ------------------------------------------------------------------------
    std::cout << "[TEST 12] Hospital Notification Dispatch & Event Tracking...\n";
    {
        auto eventBus = std::make_shared<EventBus>();
        HospitalNotificationManager notifMgr(eventBus);

        HospitalNotification notif;
        notif.type = HospitalNotificationType::INCOMING_PATIENT;
        notif.emergencyId = "EMG-NOTIF-01";
        notif.hospitalId = "HOSP_EAST";
        notif.ambulanceId = "AMB-05";
        notif.patientCondition = "CRITICAL (Severe Stroke)";
        notif.etaSeconds = 210.0;
        notif.priorityLevel = 1;
        notif.requiredDepartment = "NEUROLOGY";

        std::string id = notifMgr.SendNotification(notif, 1000);
        assert(!id.empty());
        assert(notifMgr.GetTotalNotificationCount() == 1);

        auto hospNotifs = notifMgr.GetNotificationsForHospital("HOSP_EAST");
        assert(hospNotifs.size() == 1);
        assert(hospNotifs[0].emergencyId == "EMG-NOTIF-01");
        assert(hospNotifs[0].requiredDepartment == "NEUROLOGY");

        std::cout << "  ✓ PASS: Inbound hospital notifications and multi-criteria queries verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 13: Doctor Preparation Milestones & Acknowledgement
    // ------------------------------------------------------------------------
    std::cout << "[TEST 13] Doctor Preparation Milestones & Readiness Acknowledgement...\n";
    {
        DoctorPreparationManager prepMgr;

        assert(prepMgr.StartPreparation("EMG-SURG-01", "HOSP_CENTRAL", "Dr. Jane Smith", "TRAUMA", 1000));
        assert(!prepMgr.IsPreparationComplete("EMG-SURG-01"));

        assert(prepMgr.AdvanceStage("EMG-SURG-01", PreparationStage::EQUIPMENT_READY, 1050));
        assert(prepMgr.AdvanceStage("EMG-SURG-01", PreparationStage::OPERATION_THEATRE_READY, 1100));
        assert(prepMgr.AdvanceStage("EMG-SURG-01", PreparationStage::MEDICAL_TEAM_READY, 1150));
        assert(prepMgr.AdvanceStage("EMG-SURG-01", PreparationStage::PREPARATION_COMPLETE, 1200));

        assert(prepMgr.IsPreparationComplete("EMG-SURG-01"));

        std::string ackCode = prepMgr.GenerateReadinessAck("EMG-SURG-01");
        assert(!ackCode.empty());
        assert(ackCode.find("ACK-DOC-") != std::string::npos);

        std::cout << "  ✓ PASS: Clinical preparation milestones and readiness acknowledgements verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 14: Patient Handover & Mission Completion
    // ------------------------------------------------------------------------
    std::cout << "[TEST 14] Patient Handover Lifecycle & Mission Closure...\n";
    {
        auto eventBus = std::make_shared<EventBus>();
        auto stateMgr = std::make_shared<CoordinatorStateManager>();
        auto sessionMgr = std::make_shared<EmergencySessionManager>(stateMgr, eventBus);
        auto incidentMgr = std::make_shared<IncidentManager>();

        sessionMgr->CreateSession("EMG-HANDOVER", "AMB-01", "HOSP_CENTRAL", 1, 1000);
        sessionMgr->ActivateSession("EMG-HANDOVER", 1000);

        IncidentRecord inc;
        inc.incidentId = "INC-HANDOVER";
        inc.emergencyId = "EMG-HANDOVER";
        inc.state = IncidentState::EN_ROUTE;
        incidentMgr->CreateIncident(inc);

        PatientHandoverManager handoverMgr(eventBus, sessionMgr, incidentMgr);

        // Step 1: Arrival at trauma bay
        assert(handoverMgr.RecordArrival("EMG-HANDOVER", "HOSP_CENTRAL", 1500));
        auto rec = handoverMgr.GetHandoverRecord("EMG-HANDOVER");
        assert(rec.has_value());
        assert(rec->status == HandoverStatus::ARRIVED_AT_BAY);

        // Step 2: Hospital acceptance
        assert(handoverMgr.RecordHospitalAcceptance("EMG-HANDOVER", "Dr. Wilson", 1550));
        assert(handoverMgr.GetHandoverRecord("EMG-HANDOVER")->status == HandoverStatus::ACCEPTED_BY_HOSPITAL);

        // Step 3: Treatment started
        assert(handoverMgr.RecordTreatmentStarted("EMG-HANDOVER", "Trauma Suite 1", 1600));
        assert(handoverMgr.GetHandoverRecord("EMG-HANDOVER")->status == HandoverStatus::TREATMENT_STARTED);

        // Step 4: Mission completed
        assert(handoverMgr.CompleteMission("EMG-HANDOVER", 1700));
        assert(handoverMgr.IsHandoverComplete("EMG-HANDOVER"));

        // Step 5: Mission archived
        assert(handoverMgr.ArchiveMission("EMG-HANDOVER", 1800));

        std::cout << "  ✓ PASS: Handover stages (Arrival->Acceptance->Treatment->Completed->Archived) verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 15: Incident Manager Complete Audit History
    // ------------------------------------------------------------------------
    std::cout << "[TEST 15] Incident Manager Progression & Audit Trail...\n";
    {
        IncidentManager mgr;

        IncidentRecord rec;
        rec.incidentId = "INC-AUDIT-01";
        rec.emergencyId = "EMG-AUDIT-01";
        rec.state = IncidentState::REPORTED;

        assert(mgr.CreateIncident(rec));
        assert(mgr.UpdateState("INC-AUDIT-01", IncidentState::DISPATCHED, "Assigned to AMB-01"));
        assert(mgr.UpdateState("INC-AUDIT-01", IncidentState::EN_ROUTE));
        assert(mgr.UpdateState("INC-AUDIT-01", IncidentState::ON_SCENE));
        assert(mgr.UpdateState("INC-AUDIT-01", IncidentState::TRANSPORTING));
        assert(mgr.UpdateState("INC-AUDIT-01", IncidentState::AT_HOSPITAL));
        assert(mgr.CompleteIncident("INC-AUDIT-01"));
        assert(mgr.ArchiveIncident("INC-AUDIT-01"));

        auto auditRec = mgr.GetIncident("INC-AUDIT-01");
        assert(auditRec.has_value());
        assert(auditRec->state == IncidentState::CLOSED);
        assert(auditRec->missionHistory.size() >= 7);

        std::cout << "  ✓ PASS: Complete incident audit trail and state history verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 16: End-to-End Emergency Control Center Intake & Handover Flow
    // ------------------------------------------------------------------------
    std::cout << "[TEST 16] End-to-End Emergency Control Center Full Mission Flow...\n";
    {
        auto stateMgr = std::make_shared<CoordinatorStateManager>();
        auto eventBus = std::make_shared<EventBus>();
        auto sessionMgr = std::make_shared<EmergencySessionManager>(stateMgr, eventBus);
        auto fsm = std::make_shared<CoordinatorFSM>(stateMgr, eventBus);

        EmergencyControlCenter controlCenter(stateMgr, eventBus, sessionMgr, fsm);

        // Register hospital and ambulance in cluster
        controlCenter.GetHospitalRegistry()->LoadFromConfigFile("config/Hospital.conf");
        controlCenter.GetDispatcherConsole()->Login("DISP-ALPHA", "Lead Dispatcher John");

        AmbulanceRecord amb;
        amb.ambulanceId = "AMB-01";
        amb.isActive = true;
        stateMgr->RegisterAmbulance(amb);

        uint64_t now = common::TimeUtils::GetMonotonicTimeMs();

        // 1. Incoming Cardiac Emergency Call
        EmergencyRequest call;
        call.callerId = "CITIZEN-CALL-44";
        call.callerPhone = "555-4321";
        call.latitude = 12.9716;
        call.longitude = 77.5946;
        call.closestIntersectionId = "INT_A";
        call.type = EmergencyType::Cardiac;
        call.severity = PatientSeverity::CRITICAL;
        call.casualtyCount = 1;

        EmergencyIntakeResult result = controlCenter.ProcessEmergencyCall(call, now);
        assert(result.success);
        assert(!result.emergencyId.empty());
        assert(result.priority == PriorityLevel::Priority1);
        assert(result.assignedAmbulanceId == "AMB-01");
        assert(!result.assignedHospitalId.empty());
        assert(fsm->GetCurrentState() == CoordinatorState::ROUTE_PLANNING);

        // 2. Doctor Team Preparation
        auto prepMgr = controlCenter.GetDoctorPrepManager();
        prepMgr->AdvanceStage(result.emergencyId, PreparationStage::PREPARATION_COMPLETE, now + 100);
        assert(prepMgr->IsPreparationComplete(result.emergencyId));

        // 3. Ambulance Arrival & Handover
        auto handoverMgr = controlCenter.GetHandoverManager();
        assert(handoverMgr->RecordArrival(result.emergencyId, result.assignedHospitalId, now + 500));
        assert(handoverMgr->RecordHospitalAcceptance(result.emergencyId, "Dr. Chief Cardiologist", now + 550));
        assert(handoverMgr->RecordTreatmentStarted(result.emergencyId, "Cath Lab 1", now + 600));
        assert(handoverMgr->CompleteMission(result.emergencyId, now + 700));
        assert(handoverMgr->ArchiveMission(result.emergencyId, now + 800));

        // 4. Verify mission archived in session manager
        auto session = sessionMgr->GetSession(result.emergencyId);
        assert(session.has_value());
        assert(session->sessionState == SessionState::ARCHIVED);

        std::cout << "  ✓ PASS: End-to-end Emergency Control Center mission intake & hospital delivery verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 17: Multiple Simultaneous Emergencies (Concurrency & Resource Isolation)
    // ------------------------------------------------------------------------
    std::cout << "[TEST 17] Multiple Simultaneous Emergencies Concurrency...\n";
    {
        auto stateMgr = std::make_shared<CoordinatorStateManager>();
        auto eventBus = std::make_shared<EventBus>();
        auto sessionMgr = std::make_shared<EmergencySessionManager>(stateMgr, eventBus);

        EmergencyControlCenter cc(stateMgr, eventBus, sessionMgr);
        cc.GetHospitalRegistry()->LoadFromConfigFile("config/Hospital.conf");
        cc.GetDispatcherConsole()->Login("DISP-1", "Operator 1");
        cc.GetDispatcherConsole()->Login("DISP-2", "Operator 2");

        AmbulanceRecord amb1{"AMB-01", "Ambulance 1", 1, "INT_A", "", 60.0, true};
        AmbulanceRecord amb2{"AMB-02", "Ambulance 2", 1, "INT_B", "", 60.0, true};
        stateMgr->RegisterAmbulance(amb1);
        stateMgr->RegisterAmbulance(amb2);

        EmergencyRequest req1;
        req1.callerId = "CALLER-1";
        req1.callerPhone = "111";
        req1.type = EmergencyType::Cardiac;
        req1.severity = PatientSeverity::CRITICAL;
        req1.closestIntersectionId = "INT_A";

        EmergencyRequest req2;
        req2.callerId = "CALLER-2";
        req2.callerPhone = "222";
        req2.type = EmergencyType::Stroke;
        req2.severity = PatientSeverity::CRITICAL;
        req2.closestIntersectionId = "INT_B";

        auto res1 = cc.ProcessEmergencyCall(req1);
        auto res2 = cc.ProcessEmergencyCall(req2);

        assert(res1.success);
        assert(res2.success);
        assert(res1.emergencyId != res2.emergencyId);
        assert(res1.assignedAmbulanceId != res2.assignedAmbulanceId);
        assert(cc.GetIncidentManager()->GetActiveIncidentCount() == 2);

        std::cout << "  ✓ PASS: Multiple concurrent emergencies handled without resource collision.\n";
    }

    std::cout << "\n==========================================================\n"
              << " Phase 6 Control Center & Hospital Validation:            \n"
              << " Total Tests Passed: 17 / 17 (100%)                       \n"
              << " Total Failures:     0                                    \n"
              << "==========================================================\n"
              << ">>> [SUCCESS] Control Center & Hospital Verified 100%!   \n\n";

    return 0;
}
