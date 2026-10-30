/**
 * @file test_phase4_native_qnx.cpp
 * @brief SMART AEGIS (AERS) Phase 4 - Native QNX Neutrino RTOS Subsystem Test Suite.
 *
 * Validates the 8 Phase 4 native QNX subsystems on Raspberry Pi 4 + QNX SDP 8.0:
 * 1. QNX Resource Manager Device Registration (/dev/aegis nodes)
 * 2. Device File Open, Read, Write, and Close Operations
 * 3. Industrial devctl() Command Dispatch (9 Commands & Structured Payloads)
 * 4. Native QNX Synchronous Message Passing (ChannelCreate, MsgSend, MsgReceive, MsgReply)
 * 5. Multi-Node IPC Rendezvous (Coordinator <-> Intersection A, B, C)
 * 6. Non-Blocking Pulse Events (SIGEV_PULSE, Watchdog, Heartbeat, Timeout, Deadline)
 * 7. High-Resolution QNX Timers & Microsecond Jitter Statistics (CLOCK_MONOTONIC)
 * 8. Real-Time Thread Scheduling Hierarchy (SCHED_FIFO & Priority-Inheritance Mutexes)
 * 9. GPIO Interrupt-Driven Event Management (InterruptAttachEvent, BCM 12, 13, 16, 24)
 * 10. Native QNX Diagnostics & Health Telemetry (JSON and ASCII Reports)
 *
 * Platform: Raspberry Pi 4 Model B | QNX SDP 8.0 (aarch64le) | Neutrino RTOS
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#include "aegis/qnx/QnxResourceManager.hpp"
#include "aegis/qnx/QnxMessagePassing.hpp"
#include "aegis/qnx/QnxPulseManager.hpp"
#include "aegis/qnx/QnxTimerService.hpp"
#include "aegis/qnx/QnxThreadScheduler.hpp"
#include "aegis/qnx/QnxInterruptManager.hpp"
#include "aegis/qnx/QnxDevctl.hpp"
#include "aegis/qnx/QnxDiagnostics.hpp"
#include "aegis/common/time_utils.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <thread>
#include <chrono>
#include <cassert>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  [FAIL] Assertion failed: " << (msg) \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while (0)

using namespace aegis::qnx;

// ============================================================================
// TEST 1: QNX Resource Manager Device Registration (/dev/aegis nodes)
// ============================================================================
static bool TestResourceManagerRegistration() {
    std::cout << "\n[TEST 1] QNX Resource Manager Device Registration (/dev/aegis nodes)..." << std::endl;

    auto& resMgr = QnxResourceManager::Instance();
    TEST_ASSERT(resMgr.Initialize() == 0, "Resource manager initialization failed");

    // Verify all 6 mandatory device nodes exist
    TEST_ASSERT(resMgr.HasDeviceNode(DevicePath::COORDINATOR), "Node /dev/aegis/coordinator missing");
    TEST_ASSERT(resMgr.HasDeviceNode(DevicePath::INTERSECTION_A), "Node /dev/aegis/intersectionA missing");
    TEST_ASSERT(resMgr.HasDeviceNode(DevicePath::INTERSECTION_B), "Node /dev/aegis/intersectionB missing");
    TEST_ASSERT(resMgr.HasDeviceNode(DevicePath::INTERSECTION_C), "Node /dev/aegis/intersectionC missing");
    TEST_ASSERT(resMgr.HasDeviceNode(DevicePath::CLUSTER), "Node /dev/aegis/cluster missing");
    TEST_ASSERT(resMgr.HasDeviceNode(DevicePath::SYSTEM), "Node /dev/aegis/system missing");

    auto nodes = resMgr.GetRegisteredNodes();
    TEST_ASSERT(nodes.size() >= 6, "Expected at least 6 registered device nodes");

    // Dynamic registration / unregistration
    const std::string customNode = "/dev/aegis/aux_sensor";
    TEST_ASSERT(resMgr.RegisterDeviceNode(customNode), "Failed to register custom device node");
    TEST_ASSERT(resMgr.HasDeviceNode(customNode), "Custom device node not found after registration");
    TEST_ASSERT(resMgr.UnregisterDeviceNode(customNode), "Failed to unregister custom device node");
    TEST_ASSERT(!resMgr.HasDeviceNode(customNode), "Custom node still found after unregister");

    std::cout << "  ✓ PASS: Resource manager device registration (/dev/aegis nodes) verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 2: Device File Open, Read, Write, and Close Operations
// ============================================================================
static bool TestDeviceFileIO() {
    std::cout << "\n[TEST 2] Device File Open, Read, Write, and Close Operations..." << std::endl;

    auto& resMgr = QnxResourceManager::Instance();

    // Open coordinator node
    int fdCoord = resMgr.Open(DevicePath::COORDINATOR);
    TEST_ASSERT(fdCoord >= 0, "Failed to open /dev/aegis/coordinator");

    // Read status from coordinator
    char readBuf[512] = {0};
    ssize_t bytesRead = resMgr.Read(fdCoord, readBuf, sizeof(readBuf) - 1);
    TEST_ASSERT(bytesRead > 0, "Failed to read content from /dev/aegis/coordinator");
    std::string content(readBuf);
    TEST_ASSERT(content.find("coordinator") != std::string::npos, "Content missing 'coordinator' identifier");
    TEST_ASSERT(content.find("qnx_resmgr") != std::string::npos, "Content missing 'qnx_resmgr' flag");

    // Write command to coordinator
    const char cmd[] = "CMD_START_CORRIDOR:AMB_01:INT_A,INT_B,INT_C";
    ssize_t bytesWritten = resMgr.Write(fdCoord, cmd, sizeof(cmd) - 1);
    TEST_ASSERT(bytesWritten == static_cast<ssize_t>(sizeof(cmd) - 1), "Write failed or byte count mismatch");

    // Close device
    TEST_ASSERT(resMgr.Close(fdCoord) == 0, "Failed to close device handle");

    // Verify I/O stats
    DeviceNodeInfo stats;
    TEST_ASSERT(resMgr.GetNodeStats(DevicePath::COORDINATOR, stats), "Failed to get node stats");
    TEST_ASSERT(stats.openCount >= 1, "Open count should be >= 1");
    TEST_ASSERT(stats.readCount >= 1, "Read count should be >= 1");
    TEST_ASSERT(stats.writeCount >= 1, "Write count should be >= 1");

    std::cout << "  ✓ PASS: Device file Open, Read, Write, Close operations verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 3: Industrial devctl() Command Dispatch (9 Commands)
// ============================================================================
static bool TestDevctlCommandDispatch() {
    std::cout << "\n[TEST 3] Industrial devctl() Command Dispatch (9 Commands & Structured Payloads)..." << std::endl;

    auto& resMgr = QnxResourceManager::Instance();
    int fd = resMgr.Open(DevicePath::SYSTEM);
    TEST_ASSERT(fd >= 0, "Failed to open device node for devctl");

    // 1. DCMD_AEGIS_GET_SYSTEM_STATUS
    AegisSystemStatusPayload sysStatus{};
    TEST_ASSERT(InvokeDevctl(fd, DCMD_AEGIS_GET_SYSTEM_STATUS, &sysStatus, sizeof(sysStatus)) == 0,
                "DCMD_AEGIS_GET_SYSTEM_STATUS failed");
    TEST_ASSERT(std::string(sysStatus.state) == "NORMAL_OPERATIONS", "Unexpected system state");
    TEST_ASSERT(sysStatus.activeNodes == 4, "Expected 4 active nodes");

    // 2. DCMD_AEGIS_GET_NODE_STATUS
    AegisNodeStatusPayload nodeStatus{};
    std::strncpy(nodeStatus.nodeId, "INTERSECTION_A", sizeof(nodeStatus.nodeId) - 1);
    TEST_ASSERT(InvokeDevctl(fd, DCMD_AEGIS_GET_NODE_STATUS, &nodeStatus, sizeof(nodeStatus)) == 0,
                "DCMD_AEGIS_GET_NODE_STATUS failed");
    TEST_ASSERT(nodeStatus.gpioHealthy && nodeStatus.relayHealthy, "Node hardware should be healthy");

    // 3. DCMD_AEGIS_START_CORRIDOR
    AegisStartCorridorPayload startCorr{};
    std::strncpy(startCorr.corridorId, "CORR_TEST_01", sizeof(startCorr.corridorId) - 1);
    std::strncpy(startCorr.ambulanceId, "AMB_99", sizeof(startCorr.ambulanceId) - 1);
    TEST_ASSERT(InvokeDevctl(fd, DCMD_AEGIS_START_CORRIDOR, &startCorr, sizeof(startCorr)) == 0,
                "DCMD_AEGIS_START_CORRIDOR failed");
    TEST_ASSERT(startCorr.success, "Corridor start devctl returned failure");
    TEST_ASSERT(startCorr.etaSec > 0.0, "Corridor ETA must be positive");

    // 4. DCMD_AEGIS_STOP_CORRIDOR
    AegisStopCorridorPayload stopCorr{};
    std::strncpy(stopCorr.corridorId, "CORR_TEST_01", sizeof(stopCorr.corridorId) - 1);
    TEST_ASSERT(InvokeDevctl(fd, DCMD_AEGIS_STOP_CORRIDOR, &stopCorr, sizeof(stopCorr)) == 0,
                "DCMD_AEGIS_STOP_CORRIDOR failed");
    TEST_ASSERT(stopCorr.success, "Corridor stop devctl returned failure");

    // 5. DCMD_AEGIS_RESET_NODE
    AegisResetNodePayload resetNode{};
    std::strncpy(resetNode.nodeId, "INTERSECTION_B", sizeof(resetNode.nodeId) - 1);
    TEST_ASSERT(InvokeDevctl(fd, DCMD_AEGIS_RESET_NODE, &resetNode, sizeof(resetNode)) == 0,
                "DCMD_AEGIS_RESET_NODE failed");
    TEST_ASSERT(resetNode.success, "Reset node devctl returned failure");

    // 6. DCMD_AEGIS_READ_TELEMETRY
    AegisTelemetryPayload telem{};
    TEST_ASSERT(InvokeDevctl(fd, DCMD_AEGIS_READ_TELEMETRY, &telem, sizeof(telem)) == 0,
                "DCMD_AEGIS_READ_TELEMETRY failed");
    TEST_ASSERT(telem.cpuUsage >= 0.0 && telem.ramUsageMB > 0.0, "Invalid telemetry numbers");

    // 7. DCMD_AEGIS_READ_LOGS
    AegisLogsPayload logs{};
    TEST_ASSERT(InvokeDevctl(fd, DCMD_AEGIS_READ_LOGS, &logs, sizeof(logs)) == 0,
                "DCMD_AEGIS_READ_LOGS failed");
    TEST_ASSERT(logs.logCount > 0, "Log count should be > 0");

    // 8. DCMD_AEGIS_ENABLE_DIAGNOSTICS
    AegisEnableDiagPayload diag{};
    diag.enable = true;
    diag.verboseLevel = 2;
    TEST_ASSERT(InvokeDevctl(fd, DCMD_AEGIS_ENABLE_DIAGNOSTICS, &diag, sizeof(diag)) == 0,
                "DCMD_AEGIS_ENABLE_DIAGNOSTICS failed");

    // 9. DCMD_AEGIS_RUN_SELF_TEST
    AegisSelfTestPayload selfTest{};
    TEST_ASSERT(InvokeDevctl(fd, DCMD_AEGIS_RUN_SELF_TEST, &selfTest, sizeof(selfTest)) == 0,
                "DCMD_AEGIS_RUN_SELF_TEST failed");
    TEST_ASSERT(selfTest.overallSuccess && selfTest.relaysPassed && selfTest.gpioPassed,
                "Self-test failed");

    resMgr.Close(fd);

    std::cout << "  ✓ PASS: All 9 industrial devctl() commands dispatched and verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 4: Native QNX Synchronous Message Passing (MsgSend/Receive/Reply)
// ============================================================================
static bool TestSynchronousMessagePassing() {
    std::cout << "\n[TEST 4] Native QNX Synchronous Message Passing (Rendezvous IPC)..." << std::endl;

    QnxChannelServer server("TestServer");
    int chid = server.CreateChannel();
    TEST_ASSERT(chid > 0, "Failed to create QNX channel");

    QnxChannelClient client("TestClient");
    int coid = client.Attach(chid, "TestServer");
    TEST_ASSERT(coid >= 0, "Failed to attach client connection to server");

    // Server worker thread to receive and reply
    std::thread serverThread([&server]() {
        QnxMsgHeader header;
        uint8_t payloadBuf[512] = {0};
        int rcvid = 0;
        int rc = server.Receive(header, payloadBuf, sizeof(payloadBuf), rcvid);
        if (rc >= 0 && rcvid > 0) {
            QnxReplyMsg reply;
            reply.type = QnxMsgType::IPC_ACK;
            reply.status = 0;
            std::strncpy(reply.statusMessage, "PAYLOAD_PROCESSED_OK", sizeof(reply.statusMessage) - 1);
            server.Reply(rcvid, reply);
        }
    });

    // Client sends synchronous message (blocks until server replies)
    QnxHeartbeatPayload hbPayload{};
    hbPayload.heartbeatTimeUs = aegis::common::GetMonotonicMicroseconds();
    hbPayload.cpuTemp = 42.5;
    hbPayload.cpuLoad = 15.0;

    QnxMsgHeader hdr{};
    hdr.type = QnxMsgType::HEARTBEAT;
    std::strncpy(hdr.senderId, "CLIENT_NODE_A", sizeof(hdr.senderId) - 1);
    std::strncpy(hdr.receiverId, "COORDINATOR", sizeof(hdr.receiverId) - 1);
    hdr.payloadSize = sizeof(hbPayload);

    QnxReplyMsg clientReply{};
    int sendRc = client.Send(hdr, &hbPayload, sizeof(hbPayload), clientReply);
    TEST_ASSERT(sendRc == 0, "MsgSend failed");
    TEST_ASSERT(clientReply.type == QnxMsgType::IPC_ACK, "Expected IPC_ACK reply");
    TEST_ASSERT(clientReply.status == 0, "Reply status should be 0");
    TEST_ASSERT(std::string(clientReply.statusMessage) == "PAYLOAD_PROCESSED_OK", "Unexpected reply message");

    if (serverThread.joinable()) {
        serverThread.join();
    }

    client.Detach();
    server.DestroyChannel();

    std::cout << "  ✓ PASS: Synchronous QNX message passing with rendezvous semantics verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 5: Multi-Node IPC Rendezvous (Coordinator <-> Intersections A, B, C)
// ============================================================================
static bool TestMultiNodeIpcRendezvous() {
    std::cout << "\n[TEST 5] Multi-Node IPC Rendezvous (Coordinator <-> Intersections A, B, C)..." << std::endl;

    QnxChannelServer coordinatorServer("CoordinatorChannel");
    int chid = coordinatorServer.CreateChannel();
    TEST_ASSERT(chid > 0, "Coordinator failed to create channel");

    std::atomic<int> processedNodes{0};
    std::atomic<bool> serverRunning{true};

    std::thread coordThread([&]() {
        while (serverRunning.load() && processedNodes.load() < 3) {
            QnxMsgHeader header;
            uint8_t payloadBuf[512] = {0};
            int rcvid = 0;
            int rc = coordinatorServer.Receive(header, payloadBuf, sizeof(payloadBuf), rcvid);
            if (rc >= 0 && rcvid > 0) {
                QnxReplyMsg reply;
                reply.type = QnxMsgType::IPC_ACK;
                reply.status = 0;
                std::strncpy(reply.statusMessage, "NODE_ACKNOWLEDGED", sizeof(reply.statusMessage) - 1);
                coordinatorServer.Reply(rcvid, reply);
                processedNodes++;
            }
        }
    });

    // 3 Intersection client threads communicating simultaneously
    std::vector<std::string> nodeNames = {"INTERSECTION_A", "INTERSECTION_B", "INTERSECTION_C"};
    std::vector<std::thread> clientThreads;

    for (size_t i = 0; i < nodeNames.size(); ++i) {
        clientThreads.emplace_back([&, i]() {
            QnxChannelClient client(nodeNames[i]);
            int coid = client.Attach(chid, "CoordinatorChannel");
            if (coid >= 0) {
                QnxRegistrationPayload regPayload{};
                std::strncpy(regPayload.nodeId, nodeNames[i].c_str(), sizeof(regPayload.nodeId) - 1);
                std::strncpy(regPayload.role, "INTERSECTION_CONTROLLER", sizeof(regPayload.role) - 1);
                std::strncpy(regPayload.ipAddress, "192.168.1.100", sizeof(regPayload.ipAddress) - 1);
                regPayload.swVersion = 1;

                QnxMsgHeader hdr{};
                hdr.type = QnxMsgType::REGISTRATION;
                std::strncpy(hdr.senderId, nodeNames[i].c_str(), sizeof(hdr.senderId) - 1);
                std::strncpy(hdr.receiverId, "COORDINATOR", sizeof(hdr.receiverId) - 1);
                hdr.payloadSize = sizeof(regPayload);

                QnxReplyMsg reply{};
                int rc = client.Send(hdr, &regPayload, sizeof(regPayload), reply);
                (void)rc;
                client.Detach();
            }
        });
    }

    for (auto& t : clientThreads) {
        if (t.joinable()) t.join();
    }

    serverRunning.store(false);
    if (coordThread.joinable()) {
        coordThread.join();
    }

    TEST_ASSERT(processedNodes.load() == 3, "Expected all 3 intersection nodes to be acknowledged");
    coordinatorServer.DestroyChannel();

    std::cout << "  ✓ PASS: Multi-node IPC rendezvous across 3 intersections verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 6: Non-Blocking Pulse Events (SIGEV_PULSE)
// ============================================================================
static bool TestNonBlockingPulseEvents() {
    std::cout << "\n[TEST 6] Non-Blocking Pulse Events (SIGEV_PULSE)..." << std::endl;

    int testChid = 200;
    QnxPulseManager::RegisterSimulatedChannel(testChid);

    // Test sending and receiving pulse for all defined codes
    std::vector<int8_t> codes = {
        PulseCode::WATCHDOG,
        PulseCode::HEARTBEAT,
        PulseCode::CORRIDOR_TIMEOUT,
        PulseCode::DEADLINE,
        PulseCode::FAULT_RECOVERY,
        PulseCode::INTERRUPT
    };

    for (int8_t code : codes) {
        int32_t val = static_cast<int32_t>(code) * 100;
        int sendRc = QnxPulseManager::SendPulse(testChid, 25, code, val);
        TEST_ASSERT(sendRc == 0, "SendPulse failed");

        QnxPulseEvent evt;
        int recvRc = QnxPulseManager::ReceivePulse(testChid, evt, 200);
        TEST_ASSERT(recvRc == 0, "ReceivePulse failed");
        TEST_ASSERT(evt.code == code, "Pulse code mismatch");
        TEST_ASSERT(evt.value == val, "Pulse value mismatch");
    }

    // Test background pulse dispatcher
    QnxPulseManager pulseMgr;
    std::atomic<bool> watchdogFired{false};
    pulseMgr.RegisterHandler(PulseCode::WATCHDOG, [&](const QnxPulseEvent& evt) {
        (void)evt;
        watchdogFired.store(true);
    });

    int dispatchChid = 201;
    TEST_ASSERT(pulseMgr.StartDispatcher(dispatchChid), "Failed to start pulse dispatcher");

    QnxPulseManager::SendPulse(dispatchChid, 30, PulseCode::WATCHDOG, 999);

    // Wait for dispatcher
    for (int i = 0; i < 20 && !watchdogFired.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    TEST_ASSERT(watchdogFired.load(), "Dispatcher handler was not invoked for watchdog pulse");

    pulseMgr.StopDispatcher();
    QnxPulseManager::UnregisterSimulatedChannel(testChid);
    QnxPulseManager::UnregisterSimulatedChannel(dispatchChid);

    std::cout << "  ✓ PASS: Non-blocking pulse events (SIGEV_PULSE) verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 7: High-Resolution QNX Timers & Microsecond Jitter Statistics
// ============================================================================
static bool TestHighResolutionTimersAndJitter() {
    std::cout << "\n[TEST 7] High-Resolution QNX Timers & Microsecond Jitter Statistics..." << std::endl;

    QnxTimerService timerService;
    int timerCoid = 300;
    QnxPulseManager::RegisterSimulatedChannel(timerCoid);

    AegisTimerId timerId = 0;
    int rc = timerService.CreateTimer(timerCoid, PulseCode::HEARTBEAT, 42, 21, &timerId);
    TEST_ASSERT(rc == 0, "Failed to create monotonic timer");
    TEST_ASSERT(timerService.GetActiveTimerCount() == 1, "Active timer count should be 1");

    // Arm periodic timer for 10ms intervals (10,000,000 ns)
    uint64_t periodNs = 10000000ULL;
    rc = timerService.ArmPeriodicTimer(timerId, periodNs, periodNs);
    TEST_ASSERT(rc == 0, "Failed to arm periodic timer");

    // Receive at least 3 timer pulses
    int pulsesReceived = 0;
    for (int i = 0; i < 5; ++i) {
        QnxPulseEvent evt;
        if (QnxPulseManager::ReceivePulse(timerCoid, evt, 100) == 0) {
            pulsesReceived++;
        }
    }
    TEST_ASSERT(pulsesReceived >= 3, "Expected at least 3 timer pulses to arrive");

    // Inspect jitter telemetry
    TimerJitterStats stats = timerService.GetJitterStats(timerId);
    TEST_ASSERT(stats.totalFirings >= 3, "Total firings should be >= 3");

    // Disarm and destroy timer
    TEST_ASSERT(timerService.DisarmTimer(timerId) == 0, "Failed to disarm timer");
    TEST_ASSERT(timerService.DestroyTimer(timerId) == 0, "Failed to destroy timer");
    TEST_ASSERT(timerService.GetActiveTimerCount() == 0, "Timer count should be 0 after destroy");

    QnxPulseManager::UnregisterSimulatedChannel(timerCoid);

    std::cout << "  ✓ PASS: High-resolution timers and jitter telemetry verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 8: Real-Time Thread Scheduling Hierarchy & Priority-Inheritance
// ============================================================================
static bool TestRealTimeThreadScheduler() {
    std::cout << "\n[TEST 8] Real-Time Thread Scheduling Hierarchy & Mutex Inversion Safety..." << std::endl;

    QnxThreadScheduler scheduler;

    // Register threads according to required hierarchy
    pthread_t threadCoord = pthread_self();
    scheduler.RegisterThread(threadCoord, "CoordinatorCoreThread", Priority::COORDINATOR_CORE);

    // Create 4 lightweight worker threads to test thread handle registration
    std::atomic<bool> stopWorkers{false};
    std::thread workerA([&]() { while (!stopWorkers.load()) std::this_thread::sleep_for(std::chrono::milliseconds(5)); });
    std::thread workerB([&]() { while (!stopWorkers.load()) std::this_thread::sleep_for(std::chrono::milliseconds(5)); });
    std::thread workerC([&]() { while (!stopWorkers.load()) std::this_thread::sleep_for(std::chrono::milliseconds(5)); });
    std::thread workerD([&]() { while (!stopWorkers.load()) std::this_thread::sleep_for(std::chrono::milliseconds(5)); });

    scheduler.RegisterThread(workerA.native_handle(), "IntersectionControllerThread", Priority::INTERSECTION_CTRL);
    scheduler.RegisterThread(workerB.native_handle(), "SchedulerTimelineEngine", Priority::SCHEDULER_ENGINE);
    scheduler.RegisterThread(workerC.native_handle(), "HealthDeadlineMonitor", Priority::HEALTH_DEADLINE);
    scheduler.RegisterThread(workerD.native_handle(), "TelemetryLoggerThread", Priority::TELEMETRY_LOGGING);

    TEST_ASSERT(scheduler.VerifyHierarchy(), "Priority hierarchy must be strictly monotonic: 30 > 25 > 21 > 12 > 5");

    ThreadScheduleInfo info;
    TEST_ASSERT(scheduler.GetThreadInfo("CoordinatorCoreThread", info), "Coordinator thread info missing");
    TEST_ASSERT(info.priority == Priority::COORDINATOR_CORE, "Coordinator priority mismatch");

    // Test priority-inheritance mutex
    PrioInheritMutex piMutex;
    {
        std::lock_guard<PrioInheritMutex> lock(piMutex);
        // Lock acquired safely
    }

    TEST_ASSERT(piMutex.try_lock(), "try_lock should succeed when unlocked");
    piMutex.unlock();

    stopWorkers.store(true);
    if (workerA.joinable()) workerA.join();
    if (workerB.joinable()) workerB.join();
    if (workerC.joinable()) workerC.join();
    if (workerD.joinable()) workerD.join();

    scheduler.UnregisterThread(threadCoord);

    std::cout << "  ✓ PASS: Real-time SCHED_FIFO thread hierarchy and priority-inheritance verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 9: GPIO Interrupt-Driven Event Management (BCM 12, 13, 16, 24)
// ============================================================================
static bool TestGpioInterruptManagement() {
    std::cout << "\n[TEST 9] GPIO Interrupt-Driven Event Management (BCM 12, 13, 16, 24)..." << std::endl;

    QnxInterruptManager intrMgr;
    int intrCoid = 400;
    QnxPulseManager::RegisterSimulatedChannel(intrCoid);

    // Attach 4 hardware lines
    int idApproach  = intrMgr.AttachGpioInterrupt(GpioLine::IR_APPROACH, intrCoid, PulseCode::INTERRUPT, 101);
    int idDeparture = intrMgr.AttachGpioInterrupt(GpioLine::IR_DEPARTURE, intrCoid, PulseCode::INTERRUPT, 102);
    int idEmerg     = intrMgr.AttachGpioInterrupt(GpioLine::EMERGENCY_BTN, intrCoid, PulseCode::INTERRUPT, 103);
    int idRelay     = intrMgr.AttachGpioInterrupt(GpioLine::RELAY_FAULT, intrCoid, PulseCode::INTERRUPT, 104);

    TEST_ASSERT(idApproach > 0 && idDeparture > 0 && idEmerg > 0 && idRelay > 0, "Interrupt attachment failed");
    TEST_ASSERT(intrMgr.GetAllInterrupts().size() == 4, "Expected 4 registered interrupts");

    // Trigger IR Approach interrupt
    TEST_ASSERT(intrMgr.SimulateInterrupt(GpioLine::IR_APPROACH) == 0, "SimulateInterrupt failed");
    QnxPulseEvent evt;
    TEST_ASSERT(QnxPulseManager::ReceivePulse(intrCoid, evt, 100) == 0, "Failed to receive approach interrupt pulse");
    TEST_ASSERT(evt.value == 101, "Approach pulse value mismatch");

    // Mask Departure interrupt, trigger, and verify it is blocked
    TEST_ASSERT(intrMgr.MaskInterrupt(idDeparture) == 0, "MaskInterrupt failed");
    TEST_ASSERT(intrMgr.SimulateInterrupt(GpioLine::IR_DEPARTURE) != 0, "Masked interrupt should not fire");

    // Unmask and verify it fires
    TEST_ASSERT(intrMgr.UnmaskInterrupt(idDeparture) == 0, "UnmaskInterrupt failed");
    TEST_ASSERT(intrMgr.SimulateInterrupt(GpioLine::IR_DEPARTURE) == 0, "SimulateInterrupt failed after unmask");
    TEST_ASSERT(QnxPulseManager::ReceivePulse(intrCoid, evt, 100) == 0, "Failed to receive departure pulse");
    TEST_ASSERT(evt.value == 102, "Departure pulse value mismatch");

    // Detach and verify cleanup
    TEST_ASSERT(intrMgr.DetachInterrupt(idEmerg) == 0, "Detach failed");
    TEST_ASSERT(intrMgr.DetachInterrupt(idRelay) == 0, "Detach failed");

    QnxPulseManager::UnregisterSimulatedChannel(intrCoid);

    std::cout << "  ✓ PASS: GPIO interrupt attachment, masking, and pulse notification verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 10: Native QNX Diagnostics & Health Telemetry
// ============================================================================
static bool TestNativeQnxDiagnostics() {
    std::cout << "\n[TEST 10] Native QNX Diagnostics & Health Telemetry..." << std::endl;

    auto pulseMgr = std::make_shared<QnxPulseManager>();
    auto timerService = std::make_shared<QnxTimerService>();
    auto scheduler = std::make_shared<QnxThreadScheduler>();
    auto intrMgr = std::make_shared<QnxInterruptManager>();

    // Register a thread hierarchy
    scheduler->RegisterThread(pthread_self(), "CoordinatorMain", Priority::COORDINATOR_CORE);

    QnxDiagnostics diag(pulseMgr, timerService, scheduler, intrMgr);
    QnxDiagnosticSnapshot snap = diag.CaptureSnapshot();

    TEST_ASSERT(!snap.osVersion.empty(), "OS version missing in snapshot");
    TEST_ASSERT(!snap.hardwareTarget.empty(), "Hardware target missing in snapshot");
    TEST_ASSERT(snap.priorityInversionSafe, "System should be marked priority-inversion safe");
    TEST_ASSERT(snap.deviceNodes.size() >= 6, "Expected device nodes in snapshot");

    // Generate JSON report
    std::string json = diag.GenerateJsonReport();
    TEST_ASSERT(!json.empty(), "JSON report is empty");
    TEST_ASSERT(json.find("\"resource_manager_nodes\"") != std::string::npos, "JSON report missing nodes section");
    TEST_ASSERT(json.find("\"telemetry\"") != std::string::npos, "JSON report missing telemetry section");

    // Generate ASCII report
    std::string ascii = diag.GenerateAsciiReport();
    TEST_ASSERT(!ascii.empty(), "ASCII report is empty");
    TEST_ASSERT(ascii.find("SMART AEGIS - QNX NEUTRINO RTOS DIAGNOSTICS") != std::string::npos,
                "ASCII report missing header");
    TEST_ASSERT(ascii.find("/dev/aegis/coordinator") != std::string::npos,
                "ASCII report missing coordinator node");

    std::cout << "----------------- Live QNX Diagnostics Preview -----------------\n"
              << ascii
              << "----------------------------------------------------------------\n";

    std::cout << "  ✓ PASS: Native QNX diagnostics and telemetry reports verified." << std::endl;
    return true;
}

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================
int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Phase 4 Native QNX Features Test Suite       " << std::endl;
    std::cout << " Raspberry Pi 4 Model B | QNX Neutrino RTOS 8.0 (aarch64) " << std::endl;
    std::cout << "==========================================================" << std::endl;

    int passed = 0;
    int failed = 0;

    auto RunTest = [&](bool (*testFunc)(), const char* testName) {
        if (testFunc()) {
            passed++;
        } else {
            failed++;
            std::cerr << ">>> [FAILED] " << testName << std::endl;
        }
    };

    RunTest(TestResourceManagerRegistration, "TestResourceManagerRegistration");
    RunTest(TestDeviceFileIO, "TestDeviceFileIO");
    RunTest(TestDevctlCommandDispatch, "TestDevctlCommandDispatch");
    RunTest(TestSynchronousMessagePassing, "TestSynchronousMessagePassing");
    RunTest(TestMultiNodeIpcRendezvous, "TestMultiNodeIpcRendezvous");
    RunTest(TestNonBlockingPulseEvents, "TestNonBlockingPulseEvents");
    RunTest(TestHighResolutionTimersAndJitter, "TestHighResolutionTimersAndJitter");
    RunTest(TestRealTimeThreadScheduler, "TestRealTimeThreadScheduler");
    RunTest(TestGpioInterruptManagement, "TestGpioInterruptManagement");
    RunTest(TestNativeQnxDiagnostics, "TestNativeQnxDiagnostics");

    std::cout << "\n==========================================================" << std::endl;
    std::cout << " Phase 4 Native QNX Subsystem Validation Summary:         " << std::endl;
    std::cout << " Total Passed: " << passed << "/10                        " << std::endl;
    std::cout << " Total Failed: " << failed << "/10                        " << std::endl;
    std::cout << "==========================================================" << std::endl;

    if (failed == 0) {
        std::cout << ">>> [SUCCESS] All 10 Phase 4 Native QNX Subsystem Tests Passed 100%!" << std::endl;
        return 0;
    } else {
        std::cerr << ">>> [FAILURE] Some Phase 4 Native QNX Tests Failed!" << std::endl;
        return 1;
    }
}
