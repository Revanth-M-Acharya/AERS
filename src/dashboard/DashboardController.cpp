/**
 * @file DashboardController.cpp
 * @brief SMART AEGIS (AERS) Phase 17 - Control Centre Presenter & Runtime Controller Implementation.
 */

#include "DashboardController.h"
#include <QtCore/QDateTime>
#include <cmath>

namespace aegis {
namespace dashboard {

DashboardController::DashboardController(std::shared_ptr<DashboardDataModel> dataModel,
                                         const DashboardConfig& config,
                                         QObject* parent)
    : QObject(parent)
    , m_dataModel(std::move(dataModel))
    , m_config(config)
    , m_simSpeed(config.simulationSpeed)
{
    m_syncTimer = new QTimer(this);
    int intervalMs = (m_config.targetRefreshRateHz > 0) ? (1000 / m_config.targetRefreshRateHz) : 50;
    m_syncTimer->setInterval(intervalMs);
    connect(m_syncTimer, &QTimer::timeout, this, &DashboardController::OnTimerTick);
}

DashboardController::~DashboardController() {
    Stop();
}

void DashboardController::SetCoordinatorRuntime(std::shared_ptr<runtime::CoordinatorRuntime> coordinator) {
    m_coordinatorRuntime = std::move(coordinator);
}

void DashboardController::SetDeploymentManager(std::shared_ptr<deployment::DeploymentManager> deployment) {
    m_deploymentManager = std::move(deployment);
}

void DashboardController::Start() {
    if (!m_syncTimer->isActive()) {
        int intervalMs = (m_config.targetRefreshRateHz > 0) ? (1000 / m_config.targetRefreshRateHz) : 50;
        m_syncTimer->start(intervalMs);
    }
    SyncFromBackend();
}

void DashboardController::Stop() {
    if (m_syncTimer->isActive()) {
        m_syncTimer->stop();
    }
}

bool DashboardController::IsRunning() const {
    return m_syncTimer->isActive();
}

void DashboardController::SetSimulationSpeed(double factor) {
    if (factor <= 0.0) factor = 1.0;
    if (factor > 20.0) factor = 20.0;
    m_simSpeed = factor;
    emit simulationSpeedChanged(m_simSpeed);
}

void DashboardController::PauseSimulation() {
    m_isPaused = true;
    emit simulationPausedChanged(m_isPaused);
}

void DashboardController::ResumeSimulation() {
    m_isPaused = false;
    emit simulationPausedChanged(m_isPaused);
}

void DashboardController::TogglePause() {
    m_isPaused = !m_isPaused;
    emit simulationPausedChanged(m_isPaused);
}

void DashboardController::ResetSimulation() {
    if (m_dataModel) {
        m_dataModel->ResetToDefaults();
    }
    m_tickCount = 0;
    emit alertRaised("Simulation Reset", "Municipal simulation reset to base initial state", "INFO");
}

void DashboardController::OnTimerTick() {
    if (!m_dataModel) return;

    double dt = (1.0 / static_cast<double>(m_config.targetRefreshRateHz)) * m_simSpeed;

    if (!m_isPaused) {
        AdvanceSimulationStep(dt);
    }

    SyncFromBackend();
    m_tickCount++;
}

void DashboardController::AdvanceSimulationStep(double dtSec) {
    if (!m_dataModel) return;

    // 1. Advance traffic signal timers
    auto intersections = m_dataModel->GetIntersections();
    for (auto& inter : intersections) {
        if (!inter.isPreempted) {
            if (inter.countdownSec > 0) {
                inter.countdownSec -= static_cast<int>(std::max(1.0, std::round(dtSec)));
                if (inter.countdownSec < 0) inter.countdownSec = 0;
            } else {
                // Cycle phase
                if (inter.signalState == "RED") {
                    inter.signalState = "GREEN";
                    inter.currentPhase = "NORTH-SOUTH GREEN";
                    inter.countdownSec = 25;
                } else if (inter.signalState == "GREEN") {
                    inter.signalState = "YELLOW";
                    inter.currentPhase = "CLEARANCE YELLOW";
                    inter.countdownSec = 4;
                } else {
                    inter.signalState = "RED";
                    inter.currentPhase = "ALL-RED HOLD";
                    inter.countdownSec = 20;
                }
            }
            m_dataModel->UpdateIntersection(inter);
        }
    }

    // 2. Advance active ambulance positions along corridors
    auto fleet = m_dataModel->GetFleet();
    for (auto& unit : fleet) {
        if (unit.status == "EN_ROUTE_HOSPITAL" || unit.status == "DISPATCHED") {
            if (unit.transitEtaSec > 0.0) {
                unit.transitEtaSec -= dtSec;
                if (unit.transitEtaSec < 0.0) unit.transitEtaSec = 0.0;
            }

            // Move vehicle towards hospital
            // Hospital: 12.9750, 77.5985
            double targetLat = 12.9750;
            double targetLon = 77.5985;
            double dLat = targetLat - unit.latitude;
            double dLon = targetLon - unit.longitude;
            double dist = std::hypot(dLat, dLon);

            if (dist > 0.0001) {
                double step = 0.00005 * dtSec;
                unit.latitude += (dLat / dist) * step;
                unit.longitude += (dLon / dist) * step;
                unit.speedKmh = 55.0;
            } else {
                unit.status = "AT_HOSPITAL";
                unit.speedKmh = 0.0;
            }

            m_dataModel->UpdateFleetVehicle(unit);
        }
    }
}

runtime::EmergencyPipelineResult DashboardController::TriggerEmergency(const runtime::EmergencyPipelineInput& input) {
    runtime::EmergencyPipelineResult result;

    if (m_coordinatorRuntime) {
        result = m_coordinatorRuntime->ExecuteEmergencyPipeline(input);
    } else {
        // Deterministic simulation fallback
        result.success = true;
        result.incidentId = "INC_" + input.callId;
        result.missionId = "MSN_" + input.callId;
        result.patientId = "PAT_" + input.callId;
        result.clinicalPriorityScore = 92.5;
        result.priorityLevel = "P1_CRITICAL";
        result.assignedAmbulanceId = "AMB_01";
        result.assignedHospitalId = "HOSP_CITY_TRAUMA";
        result.bedReserved = true;
        result.icuBedReserved = true;
        result.routeWaypoints = {"AMB_STATION_1", "INT_A", "INT_B", "INT_C", "HOSP_CITY_TRAUMA"};
        result.preemptionIntersections = {"INT_A", "INT_B", "INT_C"};
        result.estimatedTransitTimeSeconds = 64.0;
        result.greenWavePlanId = "GW_PLAN_" + input.callId;
        result.consensusApproved = true;
        result.consensusVotes = 4;
        result.commandsDispatched = 3;
        result.acksReceived = 3;
        result.missionStatus = "DISPATCHED";
        result.missionStage = "AMBULANCE_DISPATCH";
        result.executionLatencyUs = 1240;
    }

    if (m_dataModel) {
        EmergencyRecord rec;
        rec.incidentId = result.incidentId.empty() ? ("INC_" + input.callId) : result.incidentId;
        rec.callId = input.callId;
        rec.priorityLevel = result.priorityLevel;
        rec.clinicalScore = result.clinicalPriorityScore;
        rec.complaint = input.emergencyDescription;
        rec.latitude = input.incidentLatitude;
        rec.longitude = input.incidentLongitude;
        rec.assignedAmbulanceId = result.assignedAmbulanceId;
        rec.assignedHospitalId = result.assignedHospitalId;
        rec.routeWaypoints = result.routeWaypoints;
        rec.currentStage = PipelineStage::FLEET_DISPATCHED;
        rec.etaSeconds = static_cast<uint32_t>(result.estimatedTransitTimeSeconds);
        rec.isResolved = false;

        m_dataModel->AddOrUpdateEmergency(rec);
        m_dataModel->SetCorridor(rec.incidentId, result.preemptionIntersections);

        // Preempt intersections in model
        for (const auto& nodeId : result.preemptionIntersections) {
            IntersectionRecord inter;
            inter.nodeId = nodeId;
            inter.name = (nodeId == "INT_A") ? "Brigade Road" : (nodeId == "INT_B" ? "MG Road" : "Hospital Approach");
            inter.signalState = "GREEN";
            inter.currentPhase = "PREEMPTION HOLD";
            inter.countdownSec = static_cast<int>(result.estimatedTransitTimeSeconds);
            inter.isPreempted = true;
            inter.activeCorridorId = rec.incidentId;
            inter.irSensorActive = true;
            inter.manualOverrideActive = false;
            m_dataModel->UpdateIntersection(inter);
        }

        // Update assigned ambulance
        FleetVehicleRecord amb;
        amb.vehicleId = result.assignedAmbulanceId.empty() ? "AMB_01" : result.assignedAmbulanceId;
        amb.status = "EN_ROUTE_HOSPITAL";
        amb.speedKmh = 58.0;
        amb.headingDeg = 145.0;
        amb.latitude = input.incidentLatitude;
        amb.longitude = input.incidentLongitude;
        amb.assignedIncidentId = rec.incidentId;
        amb.assignedHospitalId = result.assignedHospitalId;
        amb.transitEtaSec = result.estimatedTransitTimeSeconds;
        amb.hasPatientOnboard = true;
        amb.patientVitals.heartRateBpm = input.heartRate;
        amb.patientVitals.spo2Pct = input.spO2;
        amb.patientVitals.bloodPressureSystolic = input.systolicBP;
        amb.patientVitals.bloodPressureDiastolic = input.diastolicBP;
        amb.patientVitals.gcsScore = input.isConscious ? 15 : 8;
        m_dataModel->UpdateFleetVehicle(amb);

        // Add audit logs
        DashboardLogEntry entry1;
        entry1.timestampIso = QDateTime::currentDateTime().toString("hh:mm:ss.zzz").toStdString();
        entry1.severity = "CRITICAL";
        entry1.component = "TRIAGE";
        entry1.message = "P1 Emergency Call " + input.callId + " (" + input.conditionHint + ") clinical triage verified";
        m_dataModel->AddLog(entry1);

        DashboardLogEntry entry2;
        entry2.timestampIso = QDateTime::currentDateTime().toString("hh:mm:ss.zzz").toStdString();
        entry2.severity = "INFO";
        entry2.component = "DISPATCH";
        entry2.message = "Ambulance " + amb.vehicleId + " dispatched to " + amb.assignedHospitalId + " with green corridor";
        m_dataModel->AddLog(entry2);

        // Update city metrics
        auto metrics = m_dataModel->GetCityMetrics();
        metrics.activeCorridorsCount++;
        metrics.preemptedIntersectionsCount = static_cast<uint32_t>(result.preemptionIntersections.size());
        metrics.cityConditionStr = "CRITICAL";
        metrics.totalEmergenciesHandled++;
        m_dataModel->SetCityMetrics(metrics);
    }

    emit emergencyTriggered(QString::fromStdString(result.incidentId));
    emit alertRaised(
        "Emergency Mission Launched",
        QString("Dispatched %1 to %2 for Call %3")
            .arg(QString::fromStdString(result.assignedAmbulanceId))
            .arg(QString::fromStdString(result.assignedHospitalId))
            .arg(QString::fromStdString(input.callId)),
        "CRITICAL"
    );

    return result;
}

runtime::EmergencyPipelineResult DashboardController::TriggerQuickEmergency(const std::string& callId,
                                                                           const std::string& description,
                                                                           double lat,
                                                                           double lon,
                                                                           uint16_t hr,
                                                                           uint16_t spo2) {
    runtime::EmergencyPipelineInput input;
    input.callId = callId;
    input.emergencyDescription = description;
    input.incidentLatitude = lat;
    input.incidentLongitude = lon;
    input.heartRate = hr;
    input.spO2 = spo2;
    input.systolicBP = 85;
    input.diastolicBP = 55;
    input.isConscious = (spo2 > 90);
    return TriggerEmergency(input);
}

void DashboardController::SyncFromBackend() {
    if (!m_dataModel) return;

    if (m_coordinatorRuntime) {
        // Read connected nodes
        auto nodes = m_coordinatorRuntime->GetConnectedNodeIds();
        for (const auto& nodeId : nodes) {
            ClusterNodeRecord nr;
            nr.nodeId = nodeId;
            nr.isOnline = true;
            nr.latencyMs = 0.5;
            nr.lastHeartbeatSec = 0.1;
            m_dataModel->UpdateClusterNode(nr);
        }
    }

    if (m_deploymentManager) {
        bool canAccept = m_deploymentManager->CanAcceptEmergencies();
        auto metrics = m_dataModel->GetCityMetrics();
        if (!canAccept && metrics.cityConditionStr == "NORMAL") {
            metrics.cityConditionStr = "DEGRADED";
            m_dataModel->SetCityMetrics(metrics);
        }
    }
}

} // namespace dashboard
} // namespace aegis
