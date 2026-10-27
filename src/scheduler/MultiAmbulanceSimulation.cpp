/**
 * @file MultiAmbulanceSimulation.cpp
 * @brief Implementation of MultiAmbulanceSimulation clinical scenarios and conflict resolution orchestrator.
 */

#include "scheduler/MultiAmbulanceSimulation.h"
#include <sstream>
#include <iomanip>

namespace aegis {
namespace scheduler {

std::string SimulationResult::ToString() const {
    std::ostringstream oss;
    oss << "=== MultiAmbulanceSimulation Results ===\n"
        << "  Success                     : " << (success ? "YES" : "NO") << "\n"
        << "  Total Ambulances Sim'd      : " << totalAmbulances << "\n"
        << "  Total Conflicts Detected    : " << totalConflictsDetected << "\n"
        << "  Total Conflicts Resolved    : " << totalConflictsResolved << "\n"
        << "  Total Commands Generated    : " << totalCommandsGenerated << "\n"
        << "  Awareness Alerts Dispatched : " << totalAwarenessNotifications << "\n"
        << "  Summary Log                 :\n" << summaryLog;
    return oss.str();
}

std::string SimulationResult::ToJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"success\":" << (success ? "true" : "false") << ","
        << "\"total_ambulances\":" << totalAmbulances << ","
        << "\"total_conflicts_detected\":" << totalConflictsDetected << ","
        << "\"total_conflicts_resolved\":" << totalConflictsResolved << ","
        << "\"total_commands_generated\":" << totalCommandsGenerated << ","
        << "\"total_awareness_notifications\":" << totalAwarenessNotifications << ","
        << "\"decisions\":[";

    for (size_t i = 0; i < decisions.size(); ++i) {
        oss << decisions[i].ToJson();
        if (i + 1 < decisions.size()) oss << ",";
    }
    oss << "],\"commands\":[";

    for (size_t i = 0; i < generatedCommands.size(); ++i) {
        oss << generatedCommands[i].ToJson();
        if (i + 1 < generatedCommands.size()) oss << ",";
    }
    oss << "]}";
    return oss.str();
}

MultiAmbulanceSimulation::MultiAmbulanceSimulation()
    : m_registry(std::make_shared<ambulance::AmbulanceRegistry>())
    , m_missionManager(std::make_shared<MissionManager>())
    , m_detectionEngine(std::make_shared<ConflictDetectionEngine>(10))
    , m_scheduler(std::make_shared<IntelligentPriorityScheduler>(15))
    , m_resolutionManager(std::make_shared<ConflictResolutionManager>())
    , m_overrideManager(std::make_shared<ManualOverrideManager>(m_registry))
    , m_awarenessService(std::make_shared<AmbulanceAwarenessService>())
    , m_coordinator(std::make_shared<coordinator::CoordinatorManager>()) {

    m_resolutionManager->SetCoordinator(m_coordinator);
}

void MultiAmbulanceSimulation::Reset() {
    m_registry->Clear();
    m_missionManager->Clear();
    m_detectionEngine->Clear();
    m_scheduler->Clear();
    m_resolutionManager->Clear();
    m_overrideManager->Clear();
    m_awarenessService->Clear();
    m_simAmbulances.clear();
}

void MultiAmbulanceSimulation::SetupEightClinicalEmergencies() {
    Reset();

    // 1. Cardiac Arrest (CRITICAL, score ~90)
    {
        SimulationAmbulance amb;
        amb.ambulanceId = "AMB_CARDIAC";
        amb.diseaseName = "Cardiac Arrest";
        amb.route = {"INT_A", "INT_B", "INT_HOSP_1"};
        amb.targetIntersection = "INT_A";
        amb.speedKmh = 75.0;
        amb.etaSeconds = 10;
        amb.waitingTimeSeconds = 0;
        amb.manualOverrideLevel = 0;

        medical::PatientProfile patient;
        patient.SetPatientId("PAT_CARDIAC");
        patient.SetPatientName("John Doe");
        patient.SetAge(58);
        patient.SetGender(medical::Gender::MALE);
        patient.SetDisease(medical::EmergencyType::CARDIAC_ARREST);
        patient.SetHeartRate(0);
        patient.SetBloodPressure(0, 0);
        patient.SetSpO2(0);
        patient.SetRespirationRate(0);
        patient.SetConscious(false);
        patient.SetEstimatedTimeSinceIncident(4);
        amb.patient = patient;

        ambulance::AmbulanceProfile prof(amb.ambulanceId, ambulance::AmbulanceStatus::TRANSPORTING, "City General Hospital", 1);
        prof.SetPatient(patient);
        prof.SetCurrentRoute(amb.route);
        prof.SetCurrentIntersection("APPROACHING_INT_A");
        prof.SetETASeconds(amb.etaSeconds);
        prof.SetCurrentSpeed(amb.speedKmh);
        prof.SetDistanceRemainingMeters(210.0);
        amb.profile = prof;

        m_registry->RegisterAmbulance(prof);
        m_missionManager->CreateMission("MIS-01", amb.ambulanceId, patient.GetPatientId(), amb.route, "City General Hospital", prof.GetPriorityScore(), "CORR-01", "Cardiac arrest resuscitation transport");
        m_simAmbulances.push_back(amb);
    }

    // 2. Internal Bleeding (CRITICAL, score ~80)
    {
        SimulationAmbulance amb;
        amb.ambulanceId = "AMB_BLEEDING";
        amb.diseaseName = "Internal Bleeding";
        amb.route = {"INT_B", "INT_C", "INT_HOSP_1"};
        amb.targetIntersection = "INT_B";
        amb.speedKmh = 70.0;
        amb.etaSeconds = 12;
        amb.waitingTimeSeconds = 0;
        amb.manualOverrideLevel = 0;

        medical::PatientProfile patient;
        patient.SetPatientId("PAT_BLEEDING");
        patient.SetPatientName("Sarah Connor");
        patient.SetAge(34);
        patient.SetGender(medical::Gender::FEMALE);
        patient.SetDisease(medical::EmergencyType::INTERNAL_BLEEDING);
        patient.SetHeartRate(145);
        patient.SetBloodPressure(65, 40);
        patient.SetSpO2(88);
        patient.SetRespirationRate(30);
        patient.SetBloodLossPercentage(38.0);
        patient.SetPainLevel(9);
        patient.SetConscious(true);
        patient.SetEstimatedTimeSinceIncident(20);
        amb.patient = patient;

        ambulance::AmbulanceProfile prof(amb.ambulanceId, ambulance::AmbulanceStatus::TRANSPORTING, "City General Hospital", 1);
        prof.SetPatient(patient);
        prof.SetCurrentRoute(amb.route);
        prof.SetCurrentIntersection("APPROACHING_INT_B");
        prof.SetETASeconds(amb.etaSeconds);
        prof.SetCurrentSpeed(amb.speedKmh);
        prof.SetDistanceRemainingMeters(230.0);
        amb.profile = prof;

        m_registry->RegisterAmbulance(prof);
        m_missionManager->CreateMission("MIS-02", amb.ambulanceId, patient.GetPatientId(), amb.route, "City General Hospital", prof.GetPriorityScore(), "CORR-02", "Severe hemorrhagic shock surgery");
        m_simAmbulances.push_back(amb);
    }

    // 3. Heart Attack (HIGH, score ~70)
    {
        SimulationAmbulance amb;
        amb.ambulanceId = "AMB_HEART_ATTACK";
        amb.diseaseName = "Heart Attack";
        amb.route = {"INT_A", "INT_D", "INT_HOSP_2"};
        amb.targetIntersection = "INT_A";
        amb.speedKmh = 65.0;
        amb.etaSeconds = 14;
        amb.waitingTimeSeconds = 0;
        amb.manualOverrideLevel = 0;

        medical::PatientProfile patient;
        patient.SetPatientId("PAT_HEART_ATTACK");
        patient.SetPatientName("Robert Miller");
        patient.SetAge(64);
        patient.SetGender(medical::Gender::MALE);
        patient.SetDisease(medical::EmergencyType::HEART_ATTACK);
        patient.SetHeartRate(118);
        patient.SetBloodPressure(160, 95);
        patient.SetSpO2(92);
        patient.SetRespirationRate(24);
        patient.SetPainLevel(9);
        patient.SetConscious(true);
        patient.SetEstimatedTimeSinceIncident(45);
        amb.patient = patient;

        ambulance::AmbulanceProfile prof(amb.ambulanceId, ambulance::AmbulanceStatus::TRANSPORTING, "St. Jude Memorial", 1);
        prof.SetPatient(patient);
        prof.SetCurrentRoute(amb.route);
        prof.SetCurrentIntersection("APPROACHING_INT_A");
        prof.SetETASeconds(amb.etaSeconds);
        prof.SetCurrentSpeed(amb.speedKmh);
        prof.SetDistanceRemainingMeters(250.0);
        amb.profile = prof;

        m_registry->RegisterAmbulance(prof);
        m_missionManager->CreateMission("MIS-03", amb.ambulanceId, patient.GetPatientId(), amb.route, "St. Jude Memorial", prof.GetPriorityScore(), "CORR-03", "Acute STEMI cath lab delivery");
        m_simAmbulances.push_back(amb);
    }

    // 4. Stroke (HIGH, score ~65)
    {
        SimulationAmbulance amb;
        amb.ambulanceId = "AMB_STROKE";
        amb.diseaseName = "Stroke";
        amb.route = {"INT_A", "INT_B", "INT_HOSP_2"};
        amb.targetIntersection = "INT_A";
        amb.speedKmh = 62.0;
        amb.etaSeconds = 15;
        amb.waitingTimeSeconds = 0;
        amb.manualOverrideLevel = 0;

        medical::PatientProfile patient;
        patient.SetPatientId("PAT_STROKE");
        patient.SetPatientName("Margaret Thatcher");
        patient.SetAge(71);
        patient.SetGender(medical::Gender::FEMALE);
        patient.SetDisease(medical::EmergencyType::STROKE);
        patient.SetHeartRate(88);
        patient.SetBloodPressure(195, 115);
        patient.SetSpO2(94);
        patient.SetRespirationRate(18);
        patient.SetPainLevel(4);
        patient.SetConscious(true);
        patient.SetEstimatedTimeSinceIncident(55);
        amb.patient = patient;

        ambulance::AmbulanceProfile prof(amb.ambulanceId, ambulance::AmbulanceStatus::TRANSPORTING, "St. Jude Memorial", 1);
        prof.SetPatient(patient);
        prof.SetCurrentRoute(amb.route);
        prof.SetCurrentIntersection("APPROACHING_INT_A");
        prof.SetETASeconds(amb.etaSeconds);
        prof.SetCurrentSpeed(amb.speedKmh);
        prof.SetDistanceRemainingMeters(260.0);
        amb.profile = prof;

        m_registry->RegisterAmbulance(prof);
        m_missionManager->CreateMission("MIS-04", amb.ambulanceId, patient.GetPatientId(), amb.route, "St. Jude Memorial", prof.GetPriorityScore(), "CORR-04", "Acute stroke thrombolysis window");
        m_simAmbulances.push_back(amb);
    }

    // 5. Major Accident (HIGH, score ~62)
    {
        SimulationAmbulance amb;
        amb.ambulanceId = "AMB_ACCIDENT";
        amb.diseaseName = "Major Accident";
        amb.route = {"INT_C", "INT_D", "INT_HOSP_1"};
        amb.targetIntersection = "INT_C";
        amb.speedKmh = 68.0;
        amb.etaSeconds = 18;
        amb.waitingTimeSeconds = 0;
        amb.manualOverrideLevel = 0;

        medical::PatientProfile patient;
        patient.SetPatientId("PAT_ACCIDENT");
        patient.SetPatientName("David Clark");
        patient.SetAge(28);
        patient.SetGender(medical::Gender::MALE);
        patient.SetDisease(medical::EmergencyType::MAJOR_ACCIDENT);
        patient.SetHeartRate(126);
        patient.SetBloodPressure(85, 55);
        patient.SetSpO2(90);
        patient.SetRespirationRate(26);
        patient.SetBloodLossPercentage(25.0);
        patient.SetPainLevel(8);
        patient.SetConscious(true);
        patient.SetEstimatedTimeSinceIncident(15);
        amb.patient = patient;

        ambulance::AmbulanceProfile prof(amb.ambulanceId, ambulance::AmbulanceStatus::TRANSPORTING, "City General Hospital", 1);
        prof.SetPatient(patient);
        prof.SetCurrentRoute(amb.route);
        prof.SetCurrentIntersection("APPROACHING_INT_C");
        prof.SetETASeconds(amb.etaSeconds);
        prof.SetCurrentSpeed(amb.speedKmh);
        prof.SetDistanceRemainingMeters(340.0);
        amb.profile = prof;

        m_registry->RegisterAmbulance(prof);
        m_missionManager->CreateMission("MIS-05", amb.ambulanceId, patient.GetPatientId(), amb.route, "City General Hospital", prof.GetPriorityScore(), "CORR-05", "Highway rollover multi-trauma");
        m_simAmbulances.push_back(amb);
    }

    // 6. Pediatric Emergency (HIGH/MEDIUM, score ~58)
    {
        SimulationAmbulance amb;
        amb.ambulanceId = "AMB_PEDIATRIC";
        amb.diseaseName = "Pediatric Emergency";
        amb.route = {"INT_C", "INT_A", "INT_CHILDRENS"};
        amb.targetIntersection = "INT_C";
        amb.speedKmh = 60.0;
        amb.etaSeconds = 20;
        amb.waitingTimeSeconds = 30; // Has been waiting for 30s
        amb.manualOverrideLevel = 0;

        medical::PatientProfile patient;
        patient.SetPatientId("PAT_PEDIATRIC");
        patient.SetPatientName("Emma Watson");
        patient.SetAge(4);
        patient.SetGender(medical::Gender::FEMALE);
        patient.SetDisease(medical::EmergencyType::PEDIATRIC_EMERGENCY);
        patient.SetHeartRate(165);
        patient.SetBloodPressure(80, 50);
        patient.SetSpO2(89);
        patient.SetRespirationRate(42);
        patient.SetPainLevel(8);
        patient.SetConscious(true);
        patient.SetEstimatedTimeSinceIncident(10);
        amb.patient = patient;

        ambulance::AmbulanceProfile prof(amb.ambulanceId, ambulance::AmbulanceStatus::TRANSPORTING, "Children's Speciality Hospital", 1);
        prof.SetPatient(patient);
        prof.SetCurrentRoute(amb.route);
        prof.SetCurrentIntersection("APPROACHING_INT_C");
        prof.SetETASeconds(amb.etaSeconds);
        prof.SetCurrentSpeed(amb.speedKmh);
        prof.SetDistanceRemainingMeters(330.0);
        amb.profile = prof;

        m_registry->RegisterAmbulance(prof);
        m_missionManager->CreateMission("MIS-06", amb.ambulanceId, patient.GetPatientId(), amb.route, "Children's Speciality Hospital", prof.GetPriorityScore(), "CORR-06", "Pediatric acute respiratory distress");
        m_simAmbulances.push_back(amb);
    }

    // 7. Organ Transport (MEDIUM/HIGH, score ~55, critical time sensitivity)
    {
        SimulationAmbulance amb;
        amb.ambulanceId = "AMB_ORGAN";
        amb.diseaseName = "Organ Transport";
        amb.route = {"INT_B", "INT_C", "INT_HOSP_1"};
        amb.targetIntersection = "INT_B";
        amb.speedKmh = 80.0;
        amb.etaSeconds = 16;
        amb.waitingTimeSeconds = 0;
        amb.manualOverrideLevel = 0;

        medical::PatientProfile patient;
        patient.SetPatientId("PAT_ORGAN");
        patient.SetPatientName("Donor Heart #402");
        patient.SetAge(22);
        patient.SetGender(medical::Gender::MALE);
        patient.SetDisease(medical::EmergencyType::ORGAN_TRANSPORT);
        patient.SetHeartRate(72);
        patient.SetBloodPressure(120, 80);
        patient.SetSpO2(99);
        patient.SetRespirationRate(16);
        patient.SetConscious(true);
        patient.SetEstimatedTimeSinceIncident(120);
        amb.patient = patient;

        ambulance::AmbulanceProfile prof(amb.ambulanceId, ambulance::AmbulanceStatus::TRANSPORTING, "City General Hospital - Transplant Wing", 1);
        prof.SetPatient(patient);
        prof.SetCurrentRoute(amb.route);
        prof.SetCurrentIntersection("APPROACHING_INT_B");
        prof.SetETASeconds(amb.etaSeconds);
        prof.SetCurrentSpeed(amb.speedKmh);
        prof.SetDistanceRemainingMeters(350.0);
        amb.profile = prof;

        m_registry->RegisterAmbulance(prof);
        m_missionManager->CreateMission("MIS-07", amb.ambulanceId, patient.GetPatientId(), amb.route, "City General Hospital - Transplant Wing", prof.GetPriorityScore(), "CORR-07", "Donor heart cold ischemic delivery");
        m_simAmbulances.push_back(amb);
    }

    // 8. Pregnancy (MEDIUM/LOW, score ~35)
    {
        SimulationAmbulance amb;
        amb.ambulanceId = "AMB_PREGNANCY";
        amb.diseaseName = "Pregnancy";
        amb.route = {"INT_A", "INT_C", "INT_MATERNITY"};
        amb.targetIntersection = "INT_A";
        amb.speedKmh = 55.0;
        amb.etaSeconds = 17;
        amb.waitingTimeSeconds = 0;
        amb.manualOverrideLevel = 0;

        medical::PatientProfile patient;
        patient.SetPatientId("PAT_PREGNANCY");
        patient.SetPatientName("Elena Rostova");
        patient.SetAge(31);
        patient.SetGender(medical::Gender::FEMALE);
        patient.SetDisease(medical::EmergencyType::PREGNANCY);
        patient.SetHeartRate(95);
        patient.SetBloodPressure(130, 85);
        patient.SetSpO2(97);
        patient.SetRespirationRate(20);
        patient.SetPainLevel(7);
        patient.SetConscious(true);
        patient.SetEstimatedTimeSinceIncident(90);
        amb.patient = patient;

        ambulance::AmbulanceProfile prof(amb.ambulanceId, ambulance::AmbulanceStatus::TRANSPORTING, "Maternity Medical Center", 1);
        prof.SetPatient(patient);
        prof.SetCurrentRoute(amb.route);
        prof.SetCurrentIntersection("APPROACHING_INT_A");
        prof.SetETASeconds(amb.etaSeconds);
        prof.SetCurrentSpeed(amb.speedKmh);
        prof.SetDistanceRemainingMeters(270.0);
        amb.profile = prof;

        m_registry->RegisterAmbulance(prof);
        m_missionManager->CreateMission("MIS-08", amb.ambulanceId, patient.GetPatientId(), amb.route, "Maternity Medical Center", prof.GetPriorityScore(), "CORR-08", "Labor and delivery transfer");
        m_simAmbulances.push_back(amb);
    }
}

SimulationResult MultiAmbulanceSimulation::RunFourAmbulanceIntersectionContention() {
    SetupEightClinicalEmergencies();

    SimulationResult res;
    std::ostringstream log;
    log << "=== SCENARIO 1: Four-Ambulance Junction Contention at INT_A ===\n";

    // Contending ambulances at INT_A: AMB_CARDIAC (10s), AMB_HEART_ATTACK (14s), AMB_STROKE (15s), AMB_PREGNANCY (17s)
    std::vector<ambulance::AmbulanceProfile> contending;
    for (const auto& amb : m_simAmbulances) {
        if (amb.targetIntersection == "INT_A") {
            contending.push_back(amb.profile);
            m_detectionEngine->RegisterArrival(amb.ambulanceId, "INT_A", amb.etaSeconds, amb.profile.GetDistanceRemainingMeters(), amb.profile);
        }
    }

    log << "Registered " << contending.size() << " vehicles approaching INT_A.\n";

    // Detect conflict
    auto conflicts = m_detectionEngine->DetectConflicts();
    res.detectedConflicts = conflicts;
    res.totalConflictsDetected += conflicts.size();

    if (!conflicts.empty()) {
        const auto& conf = conflicts.front();
        log << "Conflict detected! Involved: ";
        for (const auto& id : conf.conflictingAmbulanceIds) log << id << " ";
        log << "(Time window: " << conf.earliestEtaSeconds << "s - " << conf.latestEtaSeconds << "s)\n";

        // Generate cross-ambulance awareness notifications
        auto alerts = m_awarenessService->GenerateAwarenessForConflict(conf, "CONFLICT_DETECTED");
        res.awarenessNotifications.insert(res.awarenessNotifications.end(), alerts.begin(), alerts.end());
        res.totalAwarenessNotifications += alerts.size();
        log << "Dispatched " << alerts.size() << " peer awareness advisories.\n";

        // Schedule resolution
        auto decision = m_scheduler->ScheduleConflict(conf, contending);
        res.decisions.push_back(decision);
        res.totalConflictsResolved++;
        log << "Intelligent Priority Scheduler Decision: Winner is " << decision.winnerAmbulanceId
            << " (Reason: " << decision.decisionReason << ")\n";

        // Generate action commands
        auto commands = m_resolutionManager->GenerateResolutionCommands(decision);
        res.generatedCommands.insert(res.generatedCommands.end(), commands.begin(), commands.end());
        res.totalCommandsGenerated += commands.size();

        for (const auto& cmd : commands) {
            log << "  -> Command: " << cmd.ambulanceId << " => "
                << ResolutionActionToString(cmd.action) << " (Delay: " << cmd.delaySeconds << "s)\n";
        }
    }

    res.totalAmbulances = contending.size();
    res.success = (res.totalConflictsResolved > 0 && !res.generatedCommands.empty());
    res.summaryLog = log.str();
    return res;
}

SimulationResult MultiAmbulanceSimulation::RunManualOverrideInterventionScenario() {
    SetupEightClinicalEmergencies();

    SimulationResult res;
    std::ostringstream log;
    log << "=== SCENARIO 2: Two-Ambulance Contention at INT_B with Manual Override ===\n";

    // Contending ambulances at INT_B: AMB_BLEEDING (Internal Bleeding) vs AMB_ORGAN (Organ Transport)
    std::vector<ambulance::AmbulanceProfile> contending;
    for (const auto& amb : m_simAmbulances) {
        if (amb.ambulanceId == "AMB_BLEEDING" || amb.ambulanceId == "AMB_ORGAN") {
            contending.push_back(amb.profile);
            m_detectionEngine->RegisterArrival(amb.ambulanceId, "INT_B", amb.etaSeconds, amb.profile.GetDistanceRemainingMeters(), amb.profile);
        }
    }

    log << "AMB_BLEEDING (Score ~85) vs AMB_ORGAN (Score ~55).\n";

    // Apply manual override to AMB_ORGAN
    log << "Dispatcher applies Manual Override Level 2 to AMB_ORGAN (donor organ ischemic window expiring).\n";
    m_overrideManager->ApplyOverrideLevel("AMB_ORGAN", 2, "DISPATCH_LEAD", "Urgent organ ischemic window constraint");

    // Refresh profile in contending list
    auto updatedOrgan = m_registry->SearchAmbulance("AMB_ORGAN");
    if (updatedOrgan.has_value()) {
        for (auto& amb : contending) {
            if (amb.GetAmbulanceId() == "AMB_ORGAN") {
                amb = *updatedOrgan;
            }
        }
    }

    auto conflicts = m_detectionEngine->DetectConflicts();
    res.detectedConflicts = conflicts;
    res.totalConflictsDetected += conflicts.size();

    if (!conflicts.empty()) {
        const auto& conf = conflicts.front();
        auto decision = m_scheduler->ScheduleConflict(conf, contending);
        res.decisions.push_back(decision);
        res.totalConflictsResolved++;
        log << "Scheduler Decision: Winner is " << decision.winnerAmbulanceId
            << " (Reason: " << decision.decisionReason << ")\n";

        auto commands = m_resolutionManager->GenerateResolutionCommands(decision);
        res.generatedCommands.insert(res.generatedCommands.end(), commands.begin(), commands.end());
        res.totalCommandsGenerated += commands.size();

        for (const auto& cmd : commands) {
            log << "  -> Command: " << cmd.ambulanceId << " => "
                << ResolutionActionToString(cmd.action) << " (Delay: " << cmd.delaySeconds << "s)\n";
        }
    }

    res.totalAmbulances = contending.size();
    res.success = (res.totalConflictsResolved > 0 && !res.decisions.empty() &&
                   res.decisions.front().winnerAmbulanceId == "AMB_ORGAN");
    res.summaryLog = log.str();
    return res;
}

SimulationResult MultiAmbulanceSimulation::RunWaitingTimeStarvationPreventionScenario() {
    SetupEightClinicalEmergencies();

    SimulationResult res;
    std::ostringstream log;
    log << "=== SCENARIO 3: Contention at INT_C with Waiting Time Urgency & Alternate Routing ===\n";

    // Contending ambulances at INT_C: AMB_ACCIDENT (Major Accident) vs AMB_PEDIATRIC (Pediatric Emergency)
    std::vector<ambulance::AmbulanceProfile> contending;
    std::unordered_map<std::string, uint32_t> waitTimes;
    for (const auto& amb : m_simAmbulances) {
        if (amb.ambulanceId == "AMB_ACCIDENT" || amb.ambulanceId == "AMB_PEDIATRIC") {
            contending.push_back(amb.profile);
            waitTimes[amb.ambulanceId] = amb.waitingTimeSeconds;
            m_detectionEngine->RegisterArrival(amb.ambulanceId, "INT_C", amb.etaSeconds, amb.profile.GetDistanceRemainingMeters(), amb.profile);
        }
    }

    log << "AMB_ACCIDENT (Score ~62, Wait: 0s) vs AMB_PEDIATRIC (Score ~58, Wait: 30s).\n";

    auto conflicts = m_detectionEngine->DetectConflicts();
    res.detectedConflicts = conflicts;
    res.totalConflictsDetected += conflicts.size();

    if (!conflicts.empty()) {
        const auto& conf = conflicts.front();
        auto decision = m_scheduler->ScheduleConflict(conf, contending, waitTimes);
        res.decisions.push_back(decision);
        res.totalConflictsResolved++;
        log << "Scheduler Decision: Winner is " << decision.winnerAmbulanceId
            << " (Reason: " << decision.decisionReason << ")\n";

        // Provide alternate route for AMB_ACCIDENT
        std::unordered_map<std::string, std::vector<std::string>> altRoutes;
        altRoutes["AMB_ACCIDENT"] = {"INT_C_BYPASS", "INT_D", "INT_HOSP_1"};

        // Set alternate route threshold to 15s to demonstrate triggering
        m_resolutionManager->SetAlternateRouteThresholdSeconds(15);

        auto commands = m_resolutionManager->GenerateResolutionCommands(decision, altRoutes);
        res.generatedCommands.insert(res.generatedCommands.end(), commands.begin(), commands.end());
        res.totalCommandsGenerated += commands.size();

        for (const auto& cmd : commands) {
            log << "  -> Command: " << cmd.ambulanceId << " => "
                << ResolutionActionToString(cmd.action) << " (Delay: " << cmd.delaySeconds << "s)\n";
        }
    }

    res.totalAmbulances = contending.size();
    res.success = (res.totalConflictsResolved > 0 && !res.decisions.empty() &&
                   res.decisions.front().winnerAmbulanceId == "AMB_PEDIATRIC");
    res.summaryLog = log.str();
    return res;
}

SimulationResult MultiAmbulanceSimulation::RunFullSimulation() {
    SimulationResult fullResult;
    std::ostringstream fullLog;

    fullLog << "==========================================================\n"
            << " SMART AEGIS: Multi-Ambulance Phase 8 Full Simulation\n"
            << " Demonstrating 8 Clinical Emergency Models Simultaneously\n"
            << "==========================================================\n";

    auto res1 = RunFourAmbulanceIntersectionContention();
    auto res2 = RunManualOverrideInterventionScenario();
    auto res3 = RunWaitingTimeStarvationPreventionScenario();

    fullResult.totalAmbulances = m_simAmbulances.size();
    fullResult.totalConflictsDetected = res1.totalConflictsDetected + res2.totalConflictsDetected + res3.totalConflictsDetected;
    fullResult.totalConflictsResolved = res1.totalConflictsResolved + res2.totalConflictsResolved + res3.totalConflictsResolved;
    fullResult.totalCommandsGenerated = res1.totalCommandsGenerated + res2.totalCommandsGenerated + res3.totalCommandsGenerated;
    fullResult.totalAwarenessNotifications = res1.totalAwarenessNotifications + res2.totalAwarenessNotifications + res3.totalAwarenessNotifications;

    fullResult.decisions.insert(fullResult.decisions.end(), res1.decisions.begin(), res1.decisions.end());
    fullResult.decisions.insert(fullResult.decisions.end(), res2.decisions.begin(), res2.decisions.end());
    fullResult.decisions.insert(fullResult.decisions.end(), res3.decisions.begin(), res3.decisions.end());

    fullResult.generatedCommands.insert(fullResult.generatedCommands.end(), res1.generatedCommands.begin(), res1.generatedCommands.end());
    fullResult.generatedCommands.insert(fullResult.generatedCommands.end(), res2.generatedCommands.begin(), res2.generatedCommands.end());
    fullResult.generatedCommands.insert(fullResult.generatedCommands.end(), res3.generatedCommands.begin(), res3.generatedCommands.end());

    fullResult.awarenessNotifications.insert(fullResult.awarenessNotifications.end(), res1.awarenessNotifications.begin(), res1.awarenessNotifications.end());

    fullLog << res1.summaryLog << "\n" << res2.summaryLog << "\n" << res3.summaryLog << "\n";
    fullLog << "Simulation concluded: All 8 clinical emergency missions successfully arbitrated!\n";

    fullResult.success = (res1.success && res2.success && res3.success);
    fullResult.summaryLog = fullLog.str();
    return fullResult;
}

} // namespace scheduler
} // namespace aegis
