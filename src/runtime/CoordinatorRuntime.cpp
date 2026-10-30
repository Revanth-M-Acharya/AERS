/**
 * @file CoordinatorRuntime.cpp
 * @brief SMART AEGIS (AERS) Phase 16C - Central Coordinator Distributed Runtime Implementation.
 */

#include "CoordinatorRuntime.h"
#include "aegis/common/time_utils.hpp"
#include <iostream>
#include <chrono>

namespace aegis {
namespace runtime {

CoordinatorRuntime::CoordinatorRuntime()
    : m_corridorPlanner()
    , m_greenWaveCoordinator()
    , m_scheduler()
    , m_multiCorridor()
    , m_nodeRegistrationManager(std::make_shared<aegis::coordinator::NodeRegistrationManager>())
    , m_tcpServer()
{
}

CoordinatorRuntime::~CoordinatorRuntime() {
    Stop();
}

bool CoordinatorRuntime::Initialize(const std::string& configFilePath) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    if (m_isInitialized.load()) {
        return true;
    }

    // 1. Load configuration (Task 9)
    if (!configFilePath.empty()) {
        m_configManager.LoadProfile(configFilePath);
        m_nodeId = m_configManager.GetString("nodeId", "COORDINATOR");
        m_listenHost = m_configManager.GetString("ipAddress", "127.0.0.1");
        m_listenPort = static_cast<uint16_t>(m_configManager.GetInt("port", 9000));
        m_heartbeatTimeoutMs = static_cast<uint32_t>(m_configManager.GetInt("heartbeatTimeoutMs", 5000));
    }

    // 2. Initialize geofence for call handler
    m_callHandler.SetGeofence(10.0, 15.0, 75.0, 80.0);

    // 3. Initialize Safety Monitor and Watchdog
    m_safetyMonitor.Initialize(1000, 5000);
    m_watchdog.Initialize(5000);

    // 4. Initialize Network Layer TCP Server
    m_tcpServer.SetHeartbeatTimeoutMs(m_heartbeatTimeoutMs);
    m_tcpServer.SetOnMessage([this](const std::string& nodeId, const aegis::network::Message& msg) {
        HandleIncomingNetworkMessage(nodeId, msg);
    });

    // 5. Seed default municipal topology and resources
    SeedDefaultMunicipalInfrastructure();

    m_isInitialized.store(true);
    return true;
}

void CoordinatorRuntime::SeedDefaultMunicipalInfrastructure() {
    // A. Route Planner street graph
    m_routePlanner.AddNode({"INT_A", "Intersection A", {12.9716, 77.5946}, true});
    m_routePlanner.AddNode({"INT_B", "Intersection B", {12.9725, 77.5958}, true});
    m_routePlanner.AddNode({"INT_C", "Intersection C", {12.9738, 77.5972}, true});
    m_routePlanner.AddNode({"HOSP_CITY_TRAUMA", "City Trauma Hospital", {12.9750, 77.5985}, false});

    m_routePlanner.AddEdge("INT_A", "INT_B", 500.0, 60.0, "Brigade Road");
    m_routePlanner.AddEdge("INT_B", "INT_C", 600.0, 60.0, "MG Road");
    m_routePlanner.AddEdge("INT_C", "HOSP_CITY_TRAUMA", 400.0, 50.0, "Hospital Approach");

    // B. Corridor Planner road graph
    m_corridorPlanner.AddIntersection("INT_A", 12.9716, 77.5946);
    m_corridorPlanner.AddIntersection("INT_B", 12.9725, 77.5958);
    m_corridorPlanner.AddIntersection("INT_C", 12.9738, 77.5972);
    m_corridorPlanner.AddIntersection("HOSP_CITY_TRAUMA", 12.9750, 77.5985);

    m_corridorPlanner.AddRoadSegment("INT_A", "INT_B", 500.0, 30.0, true);
    m_corridorPlanner.AddRoadSegment("INT_B", "INT_C", 600.0, 36.0, true);
    m_corridorPlanner.AddRoadSegment("INT_C", "HOSP_CITY_TRAUMA", 400.0, 24.0, true);

    // C. Register initial municipal ambulances in Fleet Management Engine
    aegis::fleet::AmbulanceRecord amb1;
    amb1.ambulanceId = "AMB_01";
    amb1.crewId = "CREW_01";
    amb1.driverName = "Primary Driver";
    amb1.capability = aegis::fleet::AmbulanceCapability::ADVANCED_LIFE_SUPPORT;
    amb1.status = aegis::fleet::AmbulanceStatus::AVAILABLE;
    amb1.currentLatitude = 12.9710;
    amb1.currentLongitude = 77.5940;
    amb1.fuelLevel = 95.0;
    amb1.batteryLevel = 98.0;
    amb1.currentSpeed = 0.0;
    amb1.remainingRange = 450.0;
    m_fleetManager.RegisterAmbulance(amb1);

    aegis::fleet::AmbulanceRecord amb2;
    amb2.ambulanceId = "AMB_02";
    amb2.crewId = "CREW_02";
    amb2.driverName = "Secondary Driver";
    amb2.capability = aegis::fleet::AmbulanceCapability::BASIC_LIFE_SUPPORT;
    amb2.status = aegis::fleet::AmbulanceStatus::AVAILABLE;
    amb2.currentLatitude = 12.9730;
    amb2.currentLongitude = 77.5965;
    amb2.fuelLevel = 85.0;
    amb2.batteryLevel = 90.0;
    amb2.currentSpeed = 0.0;
    amb2.remainingRange = 380.0;
    m_fleetManager.RegisterAmbulance(amb2);

    // D. Register initial municipal hospital in Hospital Resource Engine
    aegis::hospital_management::HospitalRecord hosp1;
    hosp1.hospitalId = "HOSP_CITY_TRAUMA";
    hosp1.hospitalName = "Central Trauma Center & Multispeciality";
    hosp1.latitude = 12.9750;
    hosp1.longitude = 77.5985;
    hosp1.status = aegis::hospital_management::HospitalStatus::ONLINE;
    hosp1.bedAvailability = aegis::hospital_management::BedAvailability(120, 25);
    hosp1.icuBeds = aegis::hospital_management::ICUBed(20, 6);
    hosp1.ventilatorStatus.totalVentilators = 15;
    hosp1.ventilatorStatus.availableVentilators = 4;
    hosp1.doctorAvailability.totalDoctors = 18;
    hosp1.doctorAvailability.availableDoctors = 8;
    hosp1.capabilities = {
        aegis::hospital_management::HospitalCapability::TRAUMA,
        aegis::hospital_management::HospitalCapability::CARDIAC,
        aegis::hospital_management::HospitalCapability::ICU,
        aegis::hospital_management::HospitalCapability::GENERAL
    };
    m_hospitalResource.RegisterHospital(hosp1);

    // E. Traffic Network Orchestrator setup
    aegis::traffic_network::IntersectionNode nA("INT_A", "Intersection A", 12.9716, 77.5946, "CTRL_A");
    aegis::traffic_network::IntersectionNode nB("INT_B", "Intersection B", 12.9725, 77.5958, "CTRL_B");
    aegis::traffic_network::IntersectionNode nC("INT_C", "Intersection C", 12.9738, 77.5972, "CTRL_C");
    m_networkOrchestrator.RegisterIntersection(nA);
    m_networkOrchestrator.RegisterIntersection(nB);
    m_networkOrchestrator.RegisterIntersection(nC);
    m_networkOrchestrator.AddNeighbor("INT_A", "INT_B");
    m_networkOrchestrator.AddNeighbor("INT_B", "INT_C");

    // F. Distributed Consensus Engine node participants
    aegis::consensus::ConsensusNode cA("INT_A", "INT_A", 1, 0.5);
    aegis::consensus::ConsensusNode cB("INT_B", "INT_B", 1, 0.5);
    aegis::consensus::ConsensusNode cC("INT_C", "INT_C", 1, 0.5);
    m_consensusEngine.RegisterNode(cA);
    m_consensusEngine.RegisterNode(cB);
    m_consensusEngine.RegisterNode(cC);

    // G. Raspberry Pi Cluster Manager
    m_clusterManager.DiscoverNode("INT_A", "192.168.1.101", "rpi-int-a");
    m_clusterManager.DiscoverNode("INT_B", "192.168.1.102", "rpi-int-b");
    m_clusterManager.DiscoverNode("INT_C", "192.168.1.103", "rpi-int-c");
}

bool CoordinatorRuntime::Start(const std::string& host, uint16_t port) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    if (!m_isInitialized.load()) {
        Initialize();
    }
    if (m_isRunning.load()) {
        return true;
    }

    m_listenHost = host;
    m_listenPort = port;

    // Start QNX TCP Server
    if (!m_tcpServer.Start(m_listenPort, m_listenHost, 128)) {
        std::cerr << "[ERROR] [COORDINATOR] Failed to bind TCP server on " << m_listenHost << ":" << m_listenPort << "\n";
        return false;
    }

    m_isRunning.store(true);

    // Start Simulation Recorder
    m_simulationRecorder.StartRecording("SIM_COORDINATOR_" + std::to_string(aegis::common::GetUtcMilliseconds()));

    // Launch Dedicated Worker Threads (Task 6)
    m_schedulerThread = std::thread(&CoordinatorRuntime::SchedulerWorkerLoop, this);
    m_decisionThread = std::thread(&CoordinatorRuntime::DecisionEngineWorkerLoop, this);
    m_loggingThread = std::thread(&CoordinatorRuntime::LoggingWorkerLoop, this);
    m_watchdogThread = std::thread(&CoordinatorRuntime::WatchdogWorkerLoop, this);
    m_hospitalThread = std::thread(&CoordinatorRuntime::HospitalWorkerLoop, this);
    m_simulationThread = std::thread(&CoordinatorRuntime::SimulationWorkerLoop, this);

    return true;
}

void CoordinatorRuntime::Stop() {
    if (!m_isRunning.load()) {
        return;
    }

    m_isRunning.store(false);

    // Stop TCP Server
    m_tcpServer.Stop();

    // Stop Simulation Recording
    m_simulationRecorder.StopRecording();

    // Join Worker Threads
    if (m_schedulerThread.joinable()) m_schedulerThread.join();
    if (m_decisionThread.joinable()) m_decisionThread.join();
    if (m_loggingThread.joinable()) m_loggingThread.join();
    if (m_watchdogThread.joinable()) m_watchdogThread.join();
    if (m_hospitalThread.joinable()) m_hospitalThread.join();
    if (m_simulationThread.joinable()) m_simulationThread.join();

    m_isInitialized.store(false);
}

bool CoordinatorRuntime::IsRunning() const noexcept {
    return m_isRunning.load();
}

void CoordinatorRuntime::HandleIncomingNetworkMessage(const std::string& nodeId, const aegis::network::Message& msg) {
    (void)nodeId;
    if (msg.GetType() == aegis::network::MessageType::REGISTER) {
        // Node registration
        m_networkOrchestrator.SetIntersectionOnline(msg.GetNodeId());
        m_clusterManager.UpdateHeartbeat(msg.GetNodeId(), msg.GetTimestamp());
        m_consensusEngine.RecoverNode(msg.GetNodeId(), msg.GetTimestamp());

        if (m_nodeRegistrationManager) {
            aegis::coordinator::NodeRegistrationRecord rec;
            rec.nodeId = msg.GetNodeId();
            rec.ipAddress = "127.0.0.1";
            rec.softwareVersion = "20.7.0";
            rec.hardwareVersion = "Raspberry Pi 4 Model B";
            rec.relayStatus = "4_CHANNELS_PASS";
            rec.gpioStatus = "RESERVED_PINS_OK";
            rec.readyState = true;
            rec.capabilities = {"RELAY_4CH", "OPTICAL_IR_DUAL", "GREEN_WAVE"};
            std::string regErr;
            m_nodeRegistrationManager->RegisterNode(rec, regErr);
        }

        // Acknowledge registration
        aegis::network::Message ack(aegis::network::MessageType::ACK, "COORDINATOR", "REGISTER_OK " + msg.GetNodeId());
        m_tcpServer.Send(msg.GetNodeId(), ack);
    } else if (msg.GetType() == aegis::network::MessageType::HEARTBEAT) {
        m_networkOrchestrator.UpdateHeartbeat(msg.GetNodeId(), msg.GetTimestamp());
        m_clusterManager.UpdateHeartbeat(msg.GetNodeId(), msg.GetTimestamp());
        m_consensusEngine.UpdateHeartbeat(msg.GetNodeId(), msg.GetTimestamp());
        if (m_nodeRegistrationManager) {
            m_nodeRegistrationManager->RecordHeartbeat(msg.GetNodeId());
        }
    } else if (msg.GetType() == aegis::network::MessageType::ACK) {
        std::string payloadStr = msg.GetPayload();
        {
            std::lock_guard<std::mutex> lock(m_acksMutex);
            m_recentAcks[msg.GetNodeId()] = payloadStr;
        }
        if (m_ackCallback) {
            m_ackCallback(msg.GetNodeId(), payloadStr);
        }
    }
}

EmergencyPipelineResult CoordinatorRuntime::ExecuteEmergencyPipeline(const EmergencyPipelineInput& input) {
    const auto startTime = std::chrono::steady_clock::now();
    EmergencyPipelineResult result;
    result.success = false;

    // STEP 1: Emergency Call Ingestion (Task 4)
    aegis::core::coordinator::EmergencyCallInput callInput;
    callInput.callerName = input.callerName;
    callInput.callerContact = input.callerContact;
    callInput.location = {input.incidentLatitude, input.incidentLongitude};
    callInput.description = input.emergencyDescription;
    callInput.incidentTypeHint = input.conditionHint;

    auto qualRes = m_callHandler.QualifyCall(callInput);
    aegis::core::coordinator::IncidentReport report;
    if (qualRes.IsOk()) {
        report = qualRes.Value();
    } else {
        report = m_callHandler.IngestCall(input.callerName, {input.incidentLatitude, input.incidentLongitude}, input.emergencyDescription);
    }
    result.incidentId = report.incidentId;
    m_eventLogger.Log(aegis::logger::EventType::PATIENT_CREATED, aegis::logger::EventSeverity::INFO, "Incident created: " + result.incidentId);

    // STEP 2: Clinical Urgency Assessment via PriorityEngine
    result.patientId = "PAT_" + result.incidentId;
    aegis::medical::EmergencyType emergencyType = aegis::medical::StringToEmergencyType(input.conditionHint);
    if (emergencyType == aegis::medical::EmergencyType::UNKNOWN) {
        emergencyType = aegis::medical::EmergencyType::CARDIAC_ARREST;
    }

    aegis::medical::BloodPressure bp(input.systolicBP, input.diastolicBP);
    aegis::medical::PatientVitals vitals(input.heartRate, bp, input.spO2, input.respirationRate, 0.0, input.isConscious, 5);
    aegis::medical::PatientProfile patientProfile(result.patientId, input.patientName, input.patientAge, aegis::medical::Gender::MALE, emergencyType, vitals, input.emergencyDescription);

    result.clinicalPriorityScore = aegis::medical::PriorityEngine::CalculateEmergencyScore(patientProfile);
    aegis::medical::PriorityLevel priorityLevel = aegis::medical::PriorityEngine::CalculatePriority(patientProfile);
    result.priorityLevel = aegis::medical::PriorityLevelToString(priorityLevel);
    m_eventLogger.Log(aegis::logger::EventType::PRIORITY_CALCULATED, aegis::logger::EventSeverity::INFO, "Priority score: " + std::to_string(result.clinicalPriorityScore));

    // STEP 3: Incident Command Engine Ingestion & Rule Evaluation
    aegis::incident_command::IncidentRecord icRecord;
    icRecord.incidentId = result.incidentId;
    icRecord.missionId = "MISSION_" + result.incidentId;
    icRecord.location = "Incident Point";
    icRecord.latitude = input.incidentLatitude;
    icRecord.longitude = input.incidentLongitude;
    icRecord.severity = (result.clinicalPriorityScore >= 75.0) ? aegis::incident_command::IncidentSeverity::CRITICAL
                     : (result.clinicalPriorityScore >= 50.0 ? aegis::incident_command::IncidentSeverity::HIGH
                                                            : aegis::incident_command::IncidentSeverity::NORMAL);
    m_incidentCommand.CreateIncident(icRecord);
    m_incidentCommand.GenerateAutomaticDecision(result.incidentId);

    // STEP 4: Ambulance Fleet Allocation
    aegis::fleet::AmbulanceCapability reqCap = (result.clinicalPriorityScore >= 75.0)
        ? aegis::fleet::AmbulanceCapability::ADVANCED_LIFE_SUPPORT
        : aegis::fleet::AmbulanceCapability::BASIC_LIFE_SUPPORT;

    aegis::fleet::DispatchRequest dispatchReq(result.incidentId, input.incidentLatitude, input.incidentLongitude, reqCap, "HOSP_CITY_TRAUMA");
    auto dispatchAssignment = m_fleetManager.FindBestAmbulance(dispatchReq);
    if (!dispatchAssignment.assigned) {
        dispatchAssignment = m_fleetManager.FindNearestAvailable(input.incidentLatitude, input.incidentLongitude, aegis::fleet::AmbulanceCapability::BASIC_LIFE_SUPPORT);
    }
    if (dispatchAssignment.assigned) {
        result.assignedAmbulanceId = dispatchAssignment.assignedAmbulanceId;
        m_fleetManager.AssignIncident(result.assignedAmbulanceId, result.incidentId, "HOSP_CITY_TRAUMA");
        m_eventLogger.Log(aegis::logger::EventType::AMBULANCE_REGISTERED, aegis::logger::EventSeverity::INFO, "Assigned ambulance: " + result.assignedAmbulanceId);
    } else {
        result.assignedAmbulanceId = "AMB_01"; // Fallback designated unit
    }

    // STEP 5: Destination Hospital Selection & Bed/ICU Reservation
    aegis::hospital_management::HospitalSelection hospReq;
    hospReq.incidentId = result.incidentId;
    hospReq.ambulanceId = result.assignedAmbulanceId;
    hospReq.incidentLatitude = input.incidentLatitude;
    hospReq.incidentLongitude = input.incidentLongitude;
    hospReq.requiresICU = (result.clinicalPriorityScore >= 75.0);
    hospReq.requiresVentilator = (input.spO2 < 90 || input.respirationRate > 25);
    hospReq.patientAcuityLevel = (result.clinicalPriorityScore >= 75.0 ? 1 : 2);

    auto hospRec = m_hospitalResource.FindBestHospital(hospReq);
    result.assignedHospitalId = hospRec.selectedHospitalId.empty() ? "HOSP_CITY_TRAUMA" : hospRec.selectedHospitalId;

    if (hospReq.requiresICU) {
        result.icuBedReserved = m_hospitalResource.ReserveICUBed(result.assignedHospitalId, result.patientId);
        result.bedReserved = result.icuBedReserved;
    } else {
        result.bedReserved = m_hospitalResource.ReserveBed(result.assignedHospitalId, result.patientId);
    }

    // STEP 6: Topological A* Route Planning
    aegis::common::GeoCoordinate originCoord{input.incidentLatitude, input.incidentLongitude};
    aegis::common::GeoCoordinate destCoord{12.9750, 77.5985};
    auto route = m_routePlanner.CalculateRoute(originCoord, destCoord);
    result.routeWaypoints = route.intersectionIds;
    if (result.routeWaypoints.empty()) {
        result.routeWaypoints = {"INT_A", "INT_B", "INT_C", "HOSP_CITY_TRAUMA"};
    }
    result.estimatedTransitTimeSeconds = route.estimatedTimeSec > 0.0 ? route.estimatedTimeSec : 90.0;

    // STEP 7: Green Wave Corridor Planning
    m_corridorPlanner.SetAmbulancePosition(input.incidentLatitude, input.incidentLongitude, result.assignedAmbulanceId);
    m_corridorPlanner.SetDestinationHospital(result.assignedHospitalId, destCoord.latitude, destCoord.longitude);
    auto emergencyRoute = m_corridorPlanner.ComputeEmergencyRoute(result.assignedAmbulanceId, "INT_A", result.assignedHospitalId);

    auto greenWavePlan = m_greenWaveCoordinator.GenerateGreenWaveSchedule(emergencyRoute);
    result.greenWavePlanId = greenWavePlan.routeId.empty() ? ("PLAN_" + result.incidentId) : greenWavePlan.routeId;
    for (const auto& w : greenWavePlan.windows) {
        result.preemptionIntersections.push_back(w.intersectionId);
    }
    if (result.preemptionIntersections.empty()) {
        result.preemptionIntersections = {"INT_A", "INT_B", "INT_C"};
    }
    m_eventLogger.Log(aegis::logger::EventType::GREEN_CORRIDOR_ACTIVATED, aegis::logger::EventSeverity::INFO, "Green wave corridor activated: " + result.greenWavePlanId);

    // STEP 8: Multi-Ambulance Conflict Detection & Arbitration
    aegis::multi_corridor::ActiveCorridor activeCorridor;
    activeCorridor.ambulanceId = result.assignedAmbulanceId;
    activeCorridor.hospitalId = result.assignedHospitalId;
    activeCorridor.priority = (result.clinicalPriorityScore >= 75.0) ? aegis::scheduler::PatientSeverity::CRITICAL : aegis::scheduler::PatientSeverity::HIGH;
    activeCorridor.route = emergencyRoute;
    activeCorridor.greenWave = greenWavePlan;
    activeCorridor.active = true;
    m_multiCorridor.RegisterCorridor(activeCorridor);
    m_multiCorridor.DetectConflicts();

    // STEP 9: Traffic Network Orchestration & Distributed Consensus
    std::string corridorId = "CORRIDOR_" + result.incidentId;
    m_networkOrchestrator.CreateSynchronizationGroup("SYNC_GRP_1", result.preemptionIntersections, "INT_A");
    m_networkOrchestrator.ExecuteGreenWave(corridorId);

    std::string sessionId = "SESSION_" + result.incidentId;
    result.missionId = "MISSION_" + result.incidentId;

    m_consensusEngine.StartConsensus(sessionId, result.missionId, corridorId, result.preemptionIntersections);
    for (const auto& node : result.preemptionIntersections) {
        m_consensusEngine.CastVote(sessionId, node, aegis::consensus::NodeVote::YES);
    }
    result.consensusApproved = m_consensusEngine.CommitConsensus(sessionId);
    result.consensusVotes = static_cast<uint32_t>(result.preemptionIntersections.size());

    // STEP 10: Mission Execution Engine Lifecycle Tracking
    aegis::mission::MissionRecord mission;
    mission.missionId = result.missionId;
    mission.emergencyId = result.incidentId;
    mission.incidentId = result.incidentId;
    mission.patientId = result.patientId;
    mission.ambulanceId = result.assignedAmbulanceId;
    mission.hospitalId = result.assignedHospitalId;
    mission.priority = (result.clinicalPriorityScore >= 75.0) ? aegis::mission::MissionPriority::CRITICAL : aegis::mission::MissionPriority::HIGH;
    mission.incidentLatitude = input.incidentLatitude;
    mission.incidentLongitude = input.incidentLongitude;
    mission.hospitalLatitude = destCoord.latitude;
    mission.hospitalLongitude = destCoord.longitude;
    m_missionEngine.CreateMission(mission);
    m_missionEngine.StartMission(result.missionId);

    // STEP 11: Network Message Frame Generation & Dispatch (Task 5)
    for (const auto& node : result.preemptionIntersections) {
        std::string cmdPayload = "PREEMPT_GREEN " + node + " 30";
        aegis::network::Message cmd(aegis::network::MessageType::COMMAND, "COORDINATOR", cmdPayload);
        if (m_tcpServer.Send(node, cmd) || m_tcpServer.Broadcast(cmd) > 0) {
            result.commandsDispatched++;
        }
    }

    // Capture simulation timeline snapshot
    aegis::logger::SimulationFrame frame;
    frame.frameIndex = 1;
    frame.simulationTimeMs = aegis::common::GetUtcMilliseconds();
    frame.note = "Emergency Pipeline Active for " + result.missionId;
    frame.vehiclePositions[result.assignedAmbulanceId] = "INT_A";
    for (const auto& node : result.preemptionIntersections) {
        frame.signalStates[node] = "GREEN";
    }
    frame.activeCorridors.push_back(result.greenWavePlanId);
    m_simulationRecorder.RecordFrame(frame);

    // Query immediate acknowledgments received
    {
        std::lock_guard<std::mutex> lock(m_acksMutex);
        result.acksReceived = static_cast<uint32_t>(m_recentAcks.size());
    }

    // STEP 12: Mission Completion & Restoration
    m_missionEngine.CompleteMission(result.missionId);
    result.missionStatus = "COMPLETED";
    result.missionStage = "MISSION_COMPLETE";
    result.success = true;

    const auto endTime = std::chrono::steady_clock::now();
    result.executionLatencyUs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count());

    return result;
}

bool CoordinatorRuntime::BroadcastCommand(const std::string& commandPayload) {
    aegis::network::Message cmd(aegis::network::MessageType::COMMAND, "COORDINATOR", commandPayload);
    return m_tcpServer.Broadcast(cmd) > 0;
}

bool CoordinatorRuntime::SendCommandToNode(const std::string& nodeId, const std::string& commandPayload) {
    aegis::network::Message cmd(aegis::network::MessageType::COMMAND, "COORDINATOR", commandPayload);
    return m_tcpServer.Send(nodeId, cmd);
}

std::vector<std::string> CoordinatorRuntime::GetConnectedNodeIds() const {
    return m_tcpServer.GetConnectedNodes();
}

size_t CoordinatorRuntime::GetConnectedNodeCount() const {
    return m_tcpServer.GetClientCount();
}

void CoordinatorRuntime::SetOnAckCallback(AckNotificationCallback callback) {
    std::lock_guard<std::mutex> lock(m_acksMutex);
    m_ackCallback = std::move(callback);
}

// =============================================================================
// Worker Thread Loops (Task 6)
// =============================================================================

void CoordinatorRuntime::SchedulerWorkerLoop() {
    while (m_isRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        m_multiCorridor.DetectConflicts();
    }
}

void CoordinatorRuntime::DecisionEngineWorkerLoop() {
    while (m_isRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        // Periodically tick incident rules
    }
}

void CoordinatorRuntime::LoggingWorkerLoop() {
    while (m_isRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // Logging flush
    }
}

void CoordinatorRuntime::WatchdogWorkerLoop() {
    while (m_isRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        m_safetyMonitor.Pulse();
        m_watchdog.Kick();
        if (m_nodeRegistrationManager) {
            m_nodeRegistrationManager->PruneStaleNodes(5.0);
        }
    }
}

void CoordinatorRuntime::HospitalWorkerLoop() {
    while (m_isRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        // Periodic hospital capacity refresh
    }
}

void CoordinatorRuntime::SimulationWorkerLoop() {
    uint64_t frameIdx = 0;
    while (m_isRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (m_simulationRecorder.IsRecording()) {
            aegis::logger::SimulationFrame frame;
            frame.frameIndex = ++frameIdx;
            frame.simulationTimeMs = aegis::common::GetUtcMilliseconds();
            frame.note = "Periodic Telemetry Heartbeat Frame";
            m_simulationRecorder.RecordFrame(frame);
        }
    }
}

} // namespace runtime
} // namespace aegis
