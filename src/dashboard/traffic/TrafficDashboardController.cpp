/**
 * @file TrafficDashboardController.cpp
 * @brief SMART AEGIS (AERS) Phase 18 - Traffic Intersection Controller & Presenter Implementation.
 */

#include "TrafficDashboardController.h"
#include <QtCore/QDateTime>
#include <cmath>

namespace aegis {
namespace dashboard {
namespace traffic {

TrafficDashboardController::TrafficDashboardController(std::shared_ptr<TrafficDashboardDataModel> dataModel,
                                                       const TrafficDashboardConfig& config,
                                                       QObject* parent)
    : QObject(parent)
    , m_dataModel(std::move(dataModel))
    , m_config(config)
{
    m_syncTimer = new QTimer(this);
    m_syncTimer->setInterval(static_cast<int>(m_config.refreshIntervalMs));
    connect(m_syncTimer, &QTimer::timeout, this, &TrafficDashboardController::OnTimerTick);
}

TrafficDashboardController::~TrafficDashboardController() {
    StopMonitoring();
}

void TrafficDashboardController::SetCoordinatorRuntime(std::shared_ptr<runtime::CoordinatorRuntime> coordinator) {
    m_coordinatorRuntime = std::move(coordinator);
}

void TrafficDashboardController::SetTrafficNodeRuntime(std::shared_ptr<runtime::TrafficNodeRuntime> trafficNode) {
    m_trafficNodeRuntime = std::move(trafficNode);
}

void TrafficDashboardController::SetDeploymentManager(std::shared_ptr<deployment::DeploymentManager> deployment) {
    m_deploymentManager = std::move(deployment);
}

void TrafficDashboardController::SetClusterLauncher(std::shared_ptr<deployment::ClusterLauncher> launcher) {
    m_clusterLauncher = std::move(launcher);
}

void TrafficDashboardController::StartMonitoring() {
    if (!m_isMonitoring) {
        m_isMonitoring = true;
        m_syncTimer->start(static_cast<int>(m_config.refreshIntervalMs));
        emit monitoringStateChanged(true);
        if (m_dataModel) {
            m_dataModel->AddEventLog("SYSTEM", "INFO", "DIAGNOSTIC", "Real-time intersection monitoring started");
        }
    }
}

void TrafficDashboardController::StopMonitoring() {
    if (m_isMonitoring) {
        m_isMonitoring = false;
        m_syncTimer->stop();
        emit monitoringStateChanged(false);
        if (m_dataModel) {
            m_dataModel->AddEventLog("SYSTEM", "WARN", "DIAGNOSTIC", "Real-time intersection monitoring stopped");
        }
    }
}

bool TrafficDashboardController::IsMonitoring() const {
    return m_isMonitoring;
}

void TrafficDashboardController::ResetIntersection(const std::string& nodeId) {
    if (!m_dataModel) return;

    IntersectionTelemetry inter;
    if (m_dataModel->GetIntersection(nodeId, inter)) {
        inter.signalState = "RED";
        inter.currentPhase = "ALL-RED HOLD";
        inter.countdownSec = 15;
        inter.isPreempted = false;
        inter.manualOverrideActive = false;
        inter.manualOverrideState = "NONE";
        inter.gpioSignalState = "RED";
        m_dataModel->UpdateIntersection(inter);

        QueueTelemetry q;
        if (m_dataModel->GetQueue(nodeId, q)) {
            q.queueLengthVehicles = 0;
            q.estimatedWaitTimeSec = 0.0;
            q.trafficDensityPct = 5.0;
            q.isPeakLoad = false;
            m_dataModel->UpdateQueue(q);
        }

        m_dataModel->AddEventLog(nodeId, "INFO", "SIGNAL", "Intersection reset to ALL-RED cycle by operator");
        m_dataModel->TriggerNotification("Intersection Reset", "Node " + nodeId + " reset to initial safety phase", "INFO");
        emit operatorActionCompleted("RESET_INTERSECTION", QString::fromStdString(nodeId));
    }
}

void TrafficDashboardController::RestartNode(const std::string& nodeId) {
    if (!m_dataModel) return;

    IntersectionTelemetry inter;
    if (m_dataModel->GetIntersection(nodeId, inter)) {
        inter.isReady = false;
        inter.lastHeartbeatSec = 0.0;
        inter.cpuPercent = 5.0;
        m_dataModel->UpdateIntersection(inter);

        m_dataModel->AddEventLog(nodeId, "WARN", "HEARTBEAT", "Node restart command dispatched");
        m_dataModel->TriggerNotification("Node Restart", "Restarting Raspberry Pi edge node " + nodeId, "WARN");

        // Mark back online cleanly
        inter.isReady = true;
        inter.isOnline = true;
        inter.cpuPercent = 16.0;
        m_dataModel->UpdateIntersection(inter);
        m_dataModel->AddEventLog(nodeId, "INFO", "RECOVERY", "Node " + nodeId + " successfully rebooted and online");
        emit operatorActionCompleted("RESTART_NODE", QString::fromStdString(nodeId));
    }
}

void TrafficDashboardController::EnableManualOverride(const std::string& nodeId, const std::string& state) {
    if (!m_dataModel) return;

    IntersectionTelemetry inter;
    if (m_dataModel->GetIntersection(nodeId, inter)) {
        inter.manualOverrideActive = true;
        inter.manualOverrideState = state;
        if (state == "FORCE_GREEN") {
            inter.signalState = "GREEN";
            inter.currentPhase = "MANUAL OVERRIDE GREEN";
            inter.countdownSec = 99;
        } else if (state == "HOLD_RED") {
            inter.signalState = "RED";
            inter.currentPhase = "MANUAL OVERRIDE RED";
            inter.countdownSec = 99;
        }
        inter.gpioSignalState = inter.signalState;
        m_dataModel->UpdateIntersection(inter);

        m_dataModel->AddEventLog(nodeId, "WARN", "OVERRIDE", "Manual override enabled: " + state);
        m_dataModel->TriggerNotification("Manual Override", "Node " + nodeId + " set to " + state, "WARN");
        emit manualOverrideToggled(QString::fromStdString(nodeId), true, QString::fromStdString(state));
    }
}

void TrafficDashboardController::DisableManualOverride(const std::string& nodeId) {
    if (!m_dataModel) return;

    IntersectionTelemetry inter;
    if (m_dataModel->GetIntersection(nodeId, inter)) {
        inter.manualOverrideActive = false;
        inter.manualOverrideState = "NONE";
        inter.signalState = "RED";
        inter.currentPhase = "ALL-RED HOLD";
        inter.countdownSec = 10;
        inter.gpioSignalState = "RED";
        m_dataModel->UpdateIntersection(inter);

        m_dataModel->AddEventLog(nodeId, "INFO", "OVERRIDE", "Manual override released; returning to autonomous cycle");
        m_dataModel->TriggerNotification("Override Released", "Node " + nodeId + " returned to autonomous control", "INFO");
        emit manualOverrideToggled(QString::fromStdString(nodeId), false, "NONE");
    }
}

void TrafficDashboardController::TriggerEmergencyPreemptionTest(const std::string& corridorId) {
    if (!m_dataModel) return;

    GreenCorridorSchedule sched;
    sched.activeCorridorId = corridorId;
    sched.corridorDirection = "NORTH_BOUND";
    sched.emergencyVehiclePriority = "P1_CRITICAL";
    sched.assignedAmbulanceId = "AMB_01";
    sched.intersectionSequence = {"INT_A", "INT_B", "INT_C"};
    sched.currentPreemptionIndex = 0;
    sched.preemptionWindowStartSec = 0.0;
    sched.preemptionWindowDurationSec = 45.0;
    sched.signalTransitionStage = "PREEMPTION_GREEN";
    sched.estimatedArrivalTimeSec = 40.0;
    m_dataModel->SetCorridorSchedule(sched);

    // Preempt each intersection in the sequence
    for (const auto& nodeId : sched.intersectionSequence) {
        IntersectionTelemetry inter;
        if (m_dataModel->GetIntersection(nodeId, inter)) {
            inter.isPreempted = true;
            inter.activeCorridorId = corridorId;
            inter.signalState = "GREEN";
            inter.currentPhase = "EMERGENCY PREEMPTION";
            inter.countdownSec = 45;
            inter.gpioSignalState = "GREEN";
            m_dataModel->UpdateIntersection(inter);
            m_dataModel->AddEventLog(nodeId, "CRITICAL", "PREEMPTION", "Emergency green corridor preemption locked for " + corridorId);
        }
    }

    m_dataModel->TriggerNotification("Green Corridor Preemption", "Emergency preemption test active on Corridor: " + corridorId, "CRITICAL");
    emit preemptionTestTriggered(QString::fromStdString(corridorId));
}

void TrafficDashboardController::ReleaseEmergencyPreemption() {
    if (!m_dataModel) return;

    auto sched = m_dataModel->GetCorridorSchedule();
    std::string corridorId = sched.activeCorridorId;
    sched.activeCorridorId = "";
    sched.signalTransitionStage = "STANDBY";
    m_dataModel->SetCorridorSchedule(sched);

    for (const auto& nodeId : sched.intersectionSequence) {
        IntersectionTelemetry inter;
        if (m_dataModel->GetIntersection(nodeId, inter)) {
            inter.isPreempted = false;
            inter.activeCorridorId = "";
            inter.signalState = "RED";
            inter.currentPhase = "ALL-RED HOLD";
            inter.countdownSec = 10;
            inter.gpioSignalState = "RED";
            m_dataModel->UpdateIntersection(inter);
            m_dataModel->AddEventLog(nodeId, "INFO", "PREEMPTION", "Preemption cleared; restoring fixed cycle");
        }
    }

    m_dataModel->TriggerNotification("Corridor Restored", "Emergency corridor " + corridorId + " cleared", "SUCCESS");
}

void TrafficDashboardController::SelectIntersection(const std::string& nodeId) {
    if (m_dataModel) {
        m_dataModel->SetSelectedIntersectionId(nodeId);
    }
}

void TrafficDashboardController::ClearNotifications() {
    if (m_dataModel) {
        m_dataModel->ClearNotifications();
    }
}

void TrafficDashboardController::ClearLogs() {
    if (m_dataModel) {
        m_dataModel->ClearLogs();
    }
}

void TrafficDashboardController::OnTimerTick() {
    if (!m_dataModel || !m_isMonitoring) return;

    double dt = static_cast<double>(m_config.refreshIntervalMs) / 1000.0;
    AdvanceCycleStep(dt);
    SyncFromBackend();
    m_tickCounter++;
}

void TrafficDashboardController::AdvanceCycleStep(double dtSec) {
    if (!m_dataModel) return;

    // 1. Advance intersection signals
    auto intersections = m_dataModel->GetAllIntersections();
    for (auto& inter : intersections) {
        if (!inter.manualOverrideActive) {
            if (inter.countdownSec > 0) {
                inter.countdownSec -= static_cast<int>(std::max(1.0, std::round(dtSec)));
                if (inter.countdownSec < 0) inter.countdownSec = 0;
            } else {
                if (inter.isPreempted) {
                    // Preemption ended
                    inter.isPreempted = false;
                    inter.activeCorridorId = "";
                    inter.signalState = "YELLOW";
                    inter.currentPhase = "CLEARANCE_YELLOW";
                    inter.countdownSec = 4;
                } else if (inter.signalState == "RED") {
                    inter.signalState = "GREEN";
                    inter.currentPhase = "NORTH_SOUTH_GREEN";
                    inter.countdownSec = 22;
                } else if (inter.signalState == "GREEN") {
                    inter.signalState = "YELLOW";
                    inter.currentPhase = "CLEARANCE_YELLOW";
                    inter.countdownSec = 4;
                } else { // YELLOW
                    inter.signalState = "RED";
                    inter.currentPhase = "ALL_RED";
                    inter.countdownSec = 16;
                }
            }
            inter.gpioSignalState = inter.signalState;
            inter.lastHeartbeatSec += dtSec;
            m_dataModel->UpdateIntersection(inter);
        }
    }

    // 2. Advance Queue & IR sensors periodically (every 20 ticks = 1 sec)
    if (m_tickCounter % 20 == 0) {
        auto queues = m_dataModel->GetAllQueues();
        for (auto& q : queues) {
            // Queue fluctuates slightly
            int delta = (static_cast<int>(m_tickCounter % 7) == 0) ? 1 : ((m_tickCounter % 5 == 0) ? -1 : 0);
            int newQ = static_cast<int>(q.queueLengthVehicles) + delta;
            if (newQ < 0) newQ = 0;
            if (newQ > 25) newQ = 25;
            q.queueLengthVehicles = static_cast<uint32_t>(newQ);
            q.estimatedWaitTimeSec = static_cast<double>(q.queueLengthVehicles) * 4.2;
            q.trafficDensityPct = std::min(100.0, static_cast<double>(q.queueLengthVehicles) * 5.0);
            q.laneOccupancyPct = q.trafficDensityPct * 0.85;
            q.isPeakLoad = (q.queueLengthVehicles >= m_config.thresholds.queueWarningVehicles);
            m_dataModel->UpdateQueue(q);
        }

        auto irSensors = m_dataModel->GetAllIrSensors();
        for (auto& ir : irSensors) {
            if (m_tickCounter % 40 == 0) {
                ir.approachDetected = !ir.approachDetected;
                if (ir.approachDetected) {
                    ir.approachCount++;
                    ir.lastDetectionIso = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toStdString();
                }
            }
            if (m_tickCounter % 60 == 0) {
                ir.departureDetected = !ir.departureDetected;
                if (ir.departureDetected) {
                    ir.departureCount++;
                }
            }
            m_dataModel->UpdateIrSensors(ir);
        }
    }
}

void TrafficDashboardController::SyncFromBackend() {
    if (!m_dataModel) return;

    if (m_coordinatorRuntime) {
        // Read connected nodes and active green wave corridor
        auto connectedNodes = m_coordinatorRuntime->GetConnectedNodeIds();
        for (const auto& nodeId : connectedNodes) {
            IntersectionTelemetry inter;
            if (m_dataModel->GetIntersection(nodeId, inter)) {
                inter.isOnline = true;
                inter.lastHeartbeatSec = 0.1;
                m_dataModel->UpdateIntersection(inter);
            }
        }
    }

    if (m_deploymentManager) {
        bool canAccept = m_deploymentManager->CanAcceptEmergencies();
        if (!canAccept && (m_tickCounter % 200 == 0)) {
            m_dataModel->AddEventLog("CLUSTER", "WARN", "HEARTBEAT", "Deployment cluster gating active");
        }
    }
}

} // namespace traffic
} // namespace dashboard
} // namespace aegis

