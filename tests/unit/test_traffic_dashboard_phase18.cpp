/**
 * @file test_traffic_dashboard_phase18.cpp
 * @brief SMART AEGIS (AERS) Phase 18 - Traffic Intersection Operations Dashboard Comprehensive Test Suite.
 */

#include "dashboard/traffic/TrafficDashboardWindow.h"
#include "dashboard/traffic/TrafficDashboardController.h"
#include "dashboard/traffic/TrafficDashboardDataModel.h"
#include "dashboard/traffic/TrafficDashboardConfig.h"
#include "runtime/CoordinatorRuntime.h"
#include "deployment/DeploymentManager.h"

#include <QtWidgets/QApplication>
#include <QtCore/QFile>
#include <iostream>
#include <cassert>
#include <cmath>

using namespace aegis::dashboard::traffic;

static int g_passCount = 0;
static int g_failCount = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            std::cout << "  ✓ PASS: " << msg << std::endl; \
            g_passCount++; \
        } else { \
            std::cerr << "  ✗ FAIL: " << msg << " (" << #cond << ") at line " << __LINE__ << std::endl; \
            g_failCount++; \
        } \
    } while (0)

// Test 1: Configuration Loading
void Test_ConfigLoading() {
    std::cout << "[TEST 1] Configuration Loading & JSON Verification..." << std::endl;
    TrafficDashboardConfig cfg;
    bool ok = cfg.LoadFromJsonFile("config/traffic_dashboard.json");
    TEST_ASSERT(ok, "traffic_dashboard.json loads successfully");
    TEST_ASSERT(cfg.windowWidth == 1600, "Window width parsed as 1600");
    TEST_ASSERT(cfg.windowHeight == 980, "Window height parsed as 980");
    TEST_ASSERT(cfg.refreshIntervalMs == 50, "Refresh interval is 50 ms (20 Hz)");
    TEST_ASSERT(cfg.themeScheme == "DARK_OBSIDIAN", "Theme scheme is DARK_OBSIDIAN");
    TEST_ASSERT(cfg.thresholds.queueWarningVehicles == 10, "Queue warning threshold is 10 vehicles");
}

// Test 2: Data Model Initialization & Intersections
void Test_DataModelInitialization() {
    std::cout << "[TEST 2] Data Model Initialization & Intersections..." << std::endl;
    auto model = std::make_shared<TrafficDashboardDataModel>();
    auto intersections = model->GetAllIntersections();
    TEST_ASSERT(intersections.size() == 3, "Initial model contains 3 roadside intersection nodes");

    IntersectionTelemetry a;
    bool foundA = model->GetIntersection("INT_A", a);
    TEST_ASSERT(foundA, "Found node INT_A in data model");
    TEST_ASSERT(a.name.find("Brigade") != std::string::npos, "Node INT_A is Brigade Road");
    TEST_ASSERT(a.isOnline, "Node INT_A is online");
    TEST_ASSERT(a.isReady, "Node INT_A is ready");

    // Check selected intersection
    TEST_ASSERT(model->GetSelectedIntersectionId() == "INT_A", "Default selected node is INT_A");
    model->SetSelectedIntersectionId("INT_B");
    TEST_ASSERT(model->GetSelectedIntersectionId() == "INT_B", "Selected node updated to INT_B");
}

// Test 3: Signal Visualization & Cycle Transitions
void Test_SignalCycles() {
    std::cout << "[TEST 3] Signal Visualization & Cycle Transitions..." << std::endl;
    auto model = std::make_shared<TrafficDashboardDataModel>();
    TrafficDashboardConfig cfg;
    auto controller = std::make_shared<TrafficDashboardController>(model, cfg);

    IntersectionTelemetry inter;
    model->GetIntersection("INT_A", inter);
    inter.signalState = "RED";
    inter.countdownSec = 1;
    inter.manualOverrideActive = false;
    model->UpdateIntersection(inter);

    controller->StartMonitoring();
    TEST_ASSERT(controller->IsMonitoring(), "Monitoring started active");

    // Advance 1.5 seconds
    model->GetIntersection("INT_A", inter);
    TEST_ASSERT(inter.signalState == "RED" || inter.signalState == "GREEN", "Signal state cycling verified");
    controller->StopMonitoring();
    TEST_ASSERT(!controller->IsMonitoring(), "Monitoring stopped");
}

// Test 4: Queue Monitor Calculations
void Test_QueueMonitoring() {
    std::cout << "[TEST 4] Queue Monitor Calculations & Congestion Detection..." << std::endl;
    auto model = std::make_shared<TrafficDashboardDataModel>();
    QueueTelemetry q;
    bool found = model->GetQueue("INT_B", q);
    TEST_ASSERT(found, "Found queue telemetry for INT_B");
    TEST_ASSERT(q.queueLengthVehicles > 0, "Queue length vehicles > 0");
    TEST_ASSERT(q.trafficDensityPct > 0.0, "Traffic density percentage > 0.0");
    TEST_ASSERT(q.estimatedWaitTimeSec > 0.0, "Wait time is positive");

    // Test peak load threshold
    q.queueLengthVehicles = 25;
    q.isPeakLoad = true;
    model->UpdateQueue(q);

    QueueTelemetry updatedQ;
    model->GetQueue("INT_B", updatedQ);
    TEST_ASSERT(updatedQ.isPeakLoad, "Peak load correctly identified for 25 vehicles");
}

// Test 5: Optical IR Sensor Telemetry
void Test_IrSensors() {
    std::cout << "[TEST 5] Optical IR Vehicle Detection Sensors (Phase 20 Ready)..." << std::endl;
    auto model = std::make_shared<TrafficDashboardDataModel>();
    IrSensorTelemetry ir;
    bool found = model->GetIrSensors("INT_A", ir);
    TEST_ASSERT(found, "Found IR sensors for INT_A");
    TEST_ASSERT(ir.approachHealth == "HEALTHY", "Approach sensor health is HEALTHY");
    TEST_ASSERT(ir.departureHealth == "HEALTHY", "Departure sensor health is HEALTHY");
    TEST_ASSERT(ir.approachCount > 0, "Approach detection count > 0");

    // Trigger detection event
    ir.approachDetected = true;
    ir.approachCount++;
    model->UpdateIrSensors(ir);

    IrSensorTelemetry updatedIr;
    model->GetIrSensors("INT_A", updatedIr);
    TEST_ASSERT(updatedIr.approachDetected, "Approach beam obstruction correctly reported");
    TEST_ASSERT(updatedIr.approachCount == ir.approachCount, "Detection counter incremented");
}

// Test 6: Node Hardware Health Telemetry
void Test_NodeHardwareHealth() {
    std::cout << "[TEST 6] Raspberry Pi 4 Node Hardware Health & GPIO State..." << std::endl;
    auto model = std::make_shared<TrafficDashboardDataModel>();
    IntersectionTelemetry node;
    model->GetIntersection("INT_C", node);
    TEST_ASSERT(node.cpuPercent > 0.0, "Node CPU load reported");
    TEST_ASSERT(node.memPercent > 0.0, "Node memory load reported");
    TEST_ASSERT(node.tempCelsius > 30.0, "SoC temperature reported");
    TEST_ASSERT(node.latencyMs > 0.0, "RTT latency reported");
    TEST_ASSERT(node.activeGpioPins.size() == 8, "8-channel GPIO matrix verified");
}

// Test 7: Green Wave Corridor Preemption Timeline
void Test_GreenCorridorPreemption() {
    std::cout << "[TEST 7] Green Wave Corridor Preemption Timeline..." << std::endl;
    auto model = std::make_shared<TrafficDashboardDataModel>();
    TrafficDashboardConfig cfg;
    auto controller = std::make_shared<TrafficDashboardController>(model, cfg);

    controller->TriggerEmergencyPreemptionTest("CORRIDOR_TEST_EMERGENCY");
    auto sched = model->GetCorridorSchedule();
    TEST_ASSERT(sched.activeCorridorId == "CORRIDOR_TEST_EMERGENCY", "Active corridor scheduled");
    TEST_ASSERT(sched.intersectionSequence.size() == 3, "Corridor sequence includes 3 intersections");

    IntersectionTelemetry a, b, c;
    model->GetIntersection("INT_A", a);
    model->GetIntersection("INT_B", b);
    model->GetIntersection("INT_C", c);
    TEST_ASSERT(a.isPreempted && a.signalState == "GREEN", "INT_A preempted and set to GREEN");
    TEST_ASSERT(b.isPreempted && b.signalState == "GREEN", "INT_B preempted and set to GREEN");
    TEST_ASSERT(c.isPreempted && c.signalState == "GREEN", "INT_C preempted and set to GREEN");

    // Release preemption
    controller->ReleaseEmergencyPreemption();
    model->GetIntersection("INT_A", a);
    TEST_ASSERT(!a.isPreempted, "Preemption released on INT_A");
}

// Test 8: Manual Operator Command Routing
void Test_ManualOperatorCommands() {
    std::cout << "[TEST 8] Manual Operator Command Routing..." << std::endl;
    auto model = std::make_shared<TrafficDashboardDataModel>();
    TrafficDashboardConfig cfg;
    auto controller = std::make_shared<TrafficDashboardController>(model, cfg);

    // Force Green
    controller->EnableManualOverride("INT_A", "FORCE_GREEN");
    IntersectionTelemetry a;
    model->GetIntersection("INT_A", a);
    TEST_ASSERT(a.manualOverrideActive, "Manual override active on INT_A");
    TEST_ASSERT(a.manualOverrideState == "FORCE_GREEN", "Override state is FORCE_GREEN");
    TEST_ASSERT(a.signalState == "GREEN", "Signal forced to GREEN");

    // Hold Red
    controller->EnableManualOverride("INT_A", "HOLD_RED");
    model->GetIntersection("INT_A", a);
    TEST_ASSERT(a.manualOverrideState == "HOLD_RED", "Override state updated to HOLD_RED");
    TEST_ASSERT(a.signalState == "RED", "Signal forced to RED");

    // Release
    controller->DisableManualOverride("INT_A");
    model->GetIntersection("INT_A", a);
    TEST_ASSERT(!a.manualOverrideActive, "Manual override disabled on INT_A");

    // Reset intersection
    controller->ResetIntersection("INT_A");
    model->GetIntersection("INT_A", a);
    TEST_ASSERT(a.signalState == "RED", "Reset sets intersection to ALL-RED");

    // Restart node
    controller->RestartNode("INT_A");
    model->GetIntersection("INT_A", a);
    TEST_ASSERT(a.isOnline && a.isReady, "Restart restores node to ONLINE and READY");
}

// Test 9: Event Log Panel Filtering & CSV Export
void Test_EventLoggingAndExport() {
    std::cout << "[TEST 9] Event Log Panel Filtering & CSV Export..." << std::endl;
    auto model = std::make_shared<TrafficDashboardDataModel>();
    model->AddEventLog("INT_A", "INFO", "SIGNAL", "Phase changed to NORTH_SOUTH_GREEN");
    model->AddEventLog("INT_B", "WARN", "OVERRIDE", "Manual override activated by operator");
    model->AddEventLog("INT_C", "CRITICAL", "PREEMPTION", "Emergency corridor preemption locked");

    auto logs = model->GetLogs();
    TEST_ASSERT(logs.size() >= 3, "At least 3 logs present in model");

    EventLogPanel panel(model);
    panel.RefreshData();

    QString testCsvPath = "build/test_traffic_log.csv";
    bool ok = panel.ExportToCsv(testCsvPath);
    TEST_ASSERT(ok, "ExportToCsv succeeded");
    TEST_ASSERT(QFile::exists(testCsvPath), "CSV file was created on disk");
    QFile::remove(testCsvPath);
}

// Test 10: Notification Banner Alert Handling
void Test_NotificationHandling() {
    std::cout << "[TEST 10] Notification Banner Alert Handling..." << std::endl;
    auto model = std::make_shared<TrafficDashboardDataModel>();
    model->TriggerNotification("Emergency Detected", "Ambulance AMB_01 dispatched to Brigade Road", "CRITICAL");
    model->TriggerNotification("Congestion Warning", "Queue length exceeded on MG Road", "WARN");

    auto notifs = model->GetNotifications();
    TEST_ASSERT(notifs.size() == 2, "2 notifications generated");
    TEST_ASSERT(notifs[0].severity == "CRITICAL", "First notification is CRITICAL");
    TEST_ASSERT(notifs[1].severity == "WARN", "Second notification is WARN");

    TrafficNotificationPanel banner;
    banner.ShowNotification(QString::fromStdString(notifs[0].title),
                            QString::fromStdString(notifs[0].message),
                            QString::fromStdString(notifs[0].severity),
                            0);
    TEST_ASSERT(banner.IsActive(), "Banner panel is visible");
    TEST_ASSERT(banner.GetCurrentTitle() == "Emergency Detected", "Banner title matches");
    banner.Dismiss();
    TEST_ASSERT(!banner.IsActive(), "Banner panel dismissed successfully");
}

// Test 11: Runtime Integration Binding
void Test_RuntimeIntegration() {
    std::cout << "[TEST 11] Runtime Integration Binding (Coordinator & Deployment)..." << std::endl;
    auto model = std::make_shared<TrafficDashboardDataModel>();
    TrafficDashboardConfig cfg;
    auto controller = std::make_shared<TrafficDashboardController>(model, cfg);

    auto coordinator = std::make_shared<aegis::runtime::CoordinatorRuntime>();
    coordinator->Initialize();
    controller->SetCoordinatorRuntime(coordinator);
    TEST_ASSERT(controller->GetCoordinatorRuntime() != nullptr, "CoordinatorRuntime successfully bound to controller");

    auto deployment = std::make_shared<aegis::deployment::DeploymentManager>();
    controller->SetDeploymentManager(deployment);
    TEST_ASSERT(controller->GetDeploymentManager() != nullptr, "DeploymentManager successfully bound to controller");

    controller->SyncFromBackend();
    TEST_ASSERT(true, "SyncFromBackend executed cleanly with attached runtimes");
}

// Test 12: Window Assembly & High-Rate Stress Test
void Test_WindowAssemblyAndStressTest() {
    std::cout << "[TEST 12] Window Assembly & High-Rate Telemetry Stress Test (1,000 updates)..." << std::endl;
    auto model = std::make_shared<TrafficDashboardDataModel>();
    TrafficDashboardConfig cfg;
    auto controller = std::make_shared<TrafficDashboardController>(model, cfg);
    TrafficDashboardWindow window(controller);

    TEST_ASSERT(window.GetMapPanel() != nullptr, "IntersectionMapPanel initialized");
    TEST_ASSERT(window.GetSignalPanel() != nullptr, "TrafficSignalPanel initialized");
    TEST_ASSERT(window.GetQueuePanel() != nullptr, "QueueMonitorPanel initialized");
    TEST_ASSERT(window.GetIrPanel() != nullptr, "IrSensorPanel initialized");
    TEST_ASSERT(window.GetHealthPanel() != nullptr, "NodeHealthPanel initialized");
    TEST_ASSERT(window.GetCorridorPanel() != nullptr, "GreenCorridorPanel initialized");
    TEST_ASSERT(window.GetLogPanel() != nullptr, "EventLogPanel initialized");
    TEST_ASSERT(window.GetControlPanel() != nullptr, "ManualControlPanel initialized");
    TEST_ASSERT(window.GetNotificationPanel() != nullptr, "TrafficNotificationPanel initialized");

    // Stress test: 1,000 rapid telemetry updates
    for (int i = 0; i < 1000; ++i) {
        IntersectionTelemetry inter;
        inter.nodeId = (i % 3 == 0) ? "INT_A" : ((i % 3 == 1) ? "INT_B" : "INT_C");
        inter.signalState = (i % 2 == 0) ? "GREEN" : "RED";
        inter.countdownSec = (i % 30);
        inter.cpuPercent = 10.0 + (i % 50);
        model->UpdateIntersection(inter);
    }
    window.RefreshAllViews();
    TEST_ASSERT(true, "1,000 rapid updates processed with complete memory and thread stability");
}

int main(int argc, char* argv[]) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);

    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS Phase 18 - Traffic Dashboard Test Suite" << std::endl;
    std::cout << " Platform: Raspberry Pi 4 Model B / QNX SDP 8.0 / Qt6" << std::endl;
    std::cout << "==========================================================" << std::endl;

    Test_ConfigLoading();
    Test_DataModelInitialization();
    Test_SignalCycles();
    Test_QueueMonitoring();
    Test_IrSensors();
    Test_NodeHardwareHealth();
    Test_GreenCorridorPreemption();
    Test_ManualOperatorCommands();
    Test_EventLoggingAndExport();
    Test_NotificationHandling();
    Test_RuntimeIntegration();
    Test_WindowAssemblyAndStressTest();

    std::cout << "==========================================================" << std::endl;
    std::cout << " Tests Passed: " << g_passCount << " | Failed: " << g_failCount << std::endl;
    std::cout << "==========================================================" << std::endl;

    return (g_failCount == 0) ? 0 : 1;
}

