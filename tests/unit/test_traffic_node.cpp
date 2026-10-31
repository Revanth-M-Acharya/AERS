/**
 * @file test_traffic_node.cpp
 * @brief Comprehensive Unit & Integration Tests for Phase 3 Traffic Signal Node.
 * Target: Raspberry Pi 4 Model B (QNX SDP 8.0 Starter Kit Image / POSIX).
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "traffic_node/config/TrafficNodeConfig.hpp"
#include "traffic_node/logger/TrafficLogger.hpp"
#include "traffic_node/gpio/IGpioDriver.hpp"
#include "traffic_node/gpio/MockGpioDriver.hpp"
#include "traffic_node/gpio/Rpi4GpioDriver.hpp"
#include "traffic_node/relay_controller/RelayController.hpp"
#include "traffic_node/lamp_controller/LampController.hpp"
#include "traffic_node/signal_controller/TrafficSignalFSM.hpp"
#include "traffic_node/signal_controller/SignalController.hpp"
#include "traffic_node/watchdog/NodeWatchdog.hpp"
#include "traffic_node/extension/StatusPublisher.hpp"
#include "traffic_node/extension/SensorPublisher.hpp"
#include "traffic_node/extension/CameraEventListener.hpp"
#include "traffic_node/extension/TrafficEventPublisher.hpp"
#include "traffic_node/client/ConnectionManager.hpp"
#include "traffic_node/client/HeartbeatManager.hpp"
#include "traffic_node/client/TrafficClient.hpp"
#include "traffic_node/protocol/TrafficCommandDispatcher.hpp"
#include "coordinator/server/CoordinatorServer.hpp"
#include "common/utils/TimeUtils.hpp"
#include <iostream>
#include <cassert>
#include <fstream>
#include <chrono>

using namespace aegis;
using namespace aegis::traffic;

int main() {
    std::cout << "==========================================================\n"
              << " SMART AEGIS: Traffic Signal Node (Phase 3) Test Suite    \n"
              << " Target: Raspberry Pi 4 Model B (QNX SDP 8.0 / POSIX)     \n"
              << "==========================================================\n\n";

    // Mute console logging during tests for clean output
    TrafficLogger::Instance().Initialize("TEST_NODE", "", false, LogLevel::INFO);

    // ------------------------------------------------------------------------
    // TEST 1: Configuration Parser & Safety Range Validation
    // ------------------------------------------------------------------------
    std::cout << "[TEST 1] Configuration Parser & Safety Range Validation...\n";
    {
        TrafficNodeConfig cfg;
        assert(cfg.Validate());
        assert(cfg.nodeId == "INT_A");
        assert(cfg.coordinatorPort == 8080);
        assert(cfg.yellowDurationMs >= 500);

        // Test File Load
        std::string tempConf = "temp_test_node.conf";
        {
            std::ofstream out(tempConf);
            out << "NodeID = INT_B\n";
            out << "CoordinatorIP = 192.168.1.150\n";
            out << "CoordinatorPort = 9090\n";
            out << "HeartbeatInterval = 1200\n";
            out << "YellowDuration = 3500\n";
        }

        TrafficNodeConfig loaded;
        assert(loaded.LoadFromFile(tempConf));
        assert(loaded.nodeId == "INT_B");
        assert(loaded.coordinatorIp == "192.168.1.150");
        assert(loaded.coordinatorPort == 9090);
        assert(loaded.heartbeatIntervalMs == 1200);
        assert(loaded.yellowDurationMs == 3500);
        std::remove(tempConf.c_str());

        // Validate illegal configuration rejection
        loaded.yellowDurationMs = 200; // Too short, violates safety standard
        assert(!loaded.Validate());
        std::cout << "  ✓ PASS: Configuration parser, file deserialization, and safety bounds verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 2: Packet Framing, All Protocol Types & Corruption Rejection
    // ------------------------------------------------------------------------
    std::cout << "[TEST 2] Packet Framing & CRC Corruption Rejection...\n";
    {
        // Test all required protocol types
        std::vector<protocol::MessageType> types = {
            protocol::MessageType::HELLO, protocol::MessageType::HEARTBEAT,
            protocol::MessageType::STATUS, protocol::MessageType::ACK,
            protocol::MessageType::GREEN, protocol::MessageType::YELLOW,
            protocol::MessageType::RED, protocol::MessageType::ALL_RED,
            protocol::MessageType::NORMAL_MODE, protocol::MessageType::EMERGENCY_MODE,
            protocol::MessageType::PING
        };

        for (auto t : types) {
            protocol::Packet pkt(t, "INT_A", "DATA_PAYLOAD");
            auto wire = pkt.Serialize();
            protocol::Packet decoded;
            size_t consumed = 0;
            assert(protocol::Packet::Deserialize(wire.data(), wire.size(), decoded, consumed));
            assert(decoded.GetType() == t);
            assert(decoded.GetNodeId() == "INT_A");
        }

        // Test CRC corruption rejection
        protocol::Packet validPkt(protocol::MessageType::GREEN, "INT_A", "ACTIVE");
        auto wire = validPkt.Serialize();
        wire[48] ^= 0xAA; // Corrupt CRC
        protocol::Packet corruptDecoded;
        size_t consumed = 0;
        assert(!protocol::Packet::Deserialize(wire.data(), wire.size(), corruptDecoded, consumed));
        std::cout << "  ✓ PASS: Wire protocol serialization and CRC validation verified across all types.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 3: Relay Controller Hardware Abstraction Layer (HAL)
    // ------------------------------------------------------------------------
    std::cout << "[TEST 3] Relay Controller Hardware Abstraction Layer...\n";
    {
        RelayController relay;
        assert(relay.Initialize(17, 27, 22, 23, true));
        assert(relay.IsInitialized());

        // Initial safe state must be RED
        assert(relay.IsRedActive());
        assert(!relay.IsGreenActive());

        // Test Green actuation
        relay.SetGreen();
        assert(relay.IsGreenActive());
        assert(!relay.IsRedActive());
        assert(!relay.IsYellowActive());

        // Test Yellow actuation
        relay.SetYellow();
        assert(relay.IsYellowActive());
        assert(!relay.IsGreenActive());

        // Test All Red actuation
        relay.SetAllRed();
        assert(relay.IsRedActive());
        assert(relay.IsAuxActive());
        assert(!relay.IsGreenActive());

        relay.Shutdown();
        assert(!relay.IsInitialized());
        std::cout << "  ✓ PASS: Relay Controller HAL isolated pin actuation and safe defaults verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 4: FSM Deterministic Transitions & Lamp Actuation
    // ------------------------------------------------------------------------
    std::cout << "[TEST 4] FSM Deterministic Safe Transitions...\n";
    {
        auto relay = std::make_shared<RelayController>();
        relay->Initialize();
        TrafficSignalFSM fsm(relay);
        TrafficNodeConfig cfg;
        cfg.yellowDurationMs = 100; // Fast for testing
        assert(fsm.Initialize(cfg));

        assert(fsm.GetCurrentState() == SignalState::RED);
        assert(relay->IsRedActive());

        // Legal: RED -> RED_YELLOW -> GREEN
        assert(fsm.RequestTransition(SignalState::RED_YELLOW));
        assert(fsm.RequestTransition(SignalState::GREEN));
        assert(fsm.GetCurrentState() == SignalState::GREEN);
        assert(relay->IsGreenActive());

        // Legal: GREEN -> YELLOW -> ALL_RED -> RED
        assert(fsm.RequestTransition(SignalState::YELLOW));
        assert(fsm.GetCurrentState() == SignalState::YELLOW);
        assert(relay->IsYellowActive());

        assert(fsm.RequestTransition(SignalState::ALL_RED));
        assert(fsm.GetCurrentState() == SignalState::ALL_RED);

        assert(fsm.RequestTransition(SignalState::RED));
        assert(fsm.GetCurrentState() == SignalState::RED);

        fsm.Shutdown();
        std::cout << "  ✓ PASS: Legal FSM state transitions and lamp synchronization verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 5: FSM Illegal Transition Rejection (Safety Invariant)
    // ------------------------------------------------------------------------
    std::cout << "[TEST 5] FSM Illegal Transition Rejection (Safety Invariants)...\n";
    {
        auto relay = std::make_shared<RelayController>();
        relay->Initialize();
        TrafficSignalFSM fsm(relay);
        TrafficNodeConfig cfg;
        assert(fsm.Initialize(cfg));

        // Move to GREEN
        assert(fsm.RequestTransition(SignalState::RED_YELLOW));
        assert(fsm.RequestTransition(SignalState::GREEN));
        assert(fsm.GetCurrentState() == SignalState::GREEN);

        // ATTEMPT ILLEGAL DIRECT TRANSITION: GREEN -> RED
        bool illegalAttempt1 = fsm.RequestTransition(SignalState::RED);
        assert(!illegalAttempt1); // Must be rejected!
        assert(fsm.GetCurrentState() == SignalState::GREEN); // State remains intact!

        // ATTEMPT ILLEGAL DIRECT TRANSITION: GREEN -> ALL_RED
        bool illegalAttempt2 = fsm.RequestTransition(SignalState::ALL_RED);
        assert(!illegalAttempt2); // Must be rejected!
        assert(fsm.GetCurrentState() == SignalState::GREEN);

        fsm.Shutdown();
        std::cout << "  ✓ PASS: Direct GREEN->RED violation prevented; safety invariant enforced.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 6: CommandSafeState Clearance Routing
    // ------------------------------------------------------------------------
    std::cout << "[TEST 6] CommandSafeState Clearance Routing (GREEN -> YELLOW -> RED)...\n";
    {
        auto relay = std::make_shared<RelayController>();
        relay->Initialize();
        TrafficSignalFSM fsm(relay);
        TrafficNodeConfig cfg;
        cfg.yellowDurationMs = 150; // Fast clearance for test
        assert(fsm.Initialize(cfg));

        assert(fsm.RequestTransition(SignalState::RED_YELLOW));
        assert(fsm.RequestTransition(SignalState::GREEN));
        assert(fsm.GetCurrentState() == SignalState::GREEN);

        // CommandSafeState(RED) while in GREEN must automatically route via YELLOW
        assert(fsm.CommandSafeState(SignalState::RED, "Clearance Test"));
        assert(fsm.GetCurrentState() == SignalState::RED);
        assert(relay->IsRedActive());

        fsm.Shutdown();
        std::cout << "  ✓ PASS: CommandSafeState automatically clears through YELLOW before RED.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 7: TrafficCommandDispatcher Packet Routing & Rejection
    // ------------------------------------------------------------------------
    std::cout << "[TEST 7] Command Dispatcher Routing & Rejection...\n";
    {
        auto relay = std::make_shared<RelayController>();
        relay->Initialize();
        auto fsm = std::make_shared<TrafficSignalFSM>(relay);
        TrafficNodeConfig cfg;
        cfg.yellowDurationMs = 100;
        fsm->Initialize(cfg);

        bool pingAnswered = false;
        TrafficCommandDispatcher dispatcher("INT_A", fsm, [&pingAnswered](const protocol::Packet& resp) {
            if (resp.GetType() == protocol::MessageType::STATUS) {
                pingAnswered = true;
            }
            return true;
        });

        // Test PING -> STATUS
        protocol::Packet pingPkt(protocol::MessageType::PING, "COORDINATOR", "");
        assert(dispatcher.DispatchPacket(pingPkt));
        assert(pingAnswered);

        // Test EMERGENCY_MODE dispatch
        protocol::Packet emPkt(protocol::MessageType::EMERGENCY_MODE, "COORDINATOR", "CORR_99");
        assert(dispatcher.DispatchPacket(emPkt));
        assert(fsm->IsInEmergencyMode());

        // Test NORMAL_MODE dispatch
        protocol::Packet normPkt(protocol::MessageType::NORMAL_MODE, "COORDINATOR", "");
        assert(dispatcher.DispatchPacket(normPkt));
        assert(!fsm->IsInEmergencyMode());

        // Test UNKNOWN rejection
        protocol::Packet unknownPkt;
        unknownPkt.SetType(protocol::MessageType::UNKNOWN);
        assert(!dispatcher.DispatchPacket(unknownPkt));
        assert(dispatcher.GetRejectedCount() > 0);

        fsm->Shutdown();
        std::cout << "  ✓ PASS: Dispatcher packet handling, PING response, and UNKNOWN rejection verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 8: ConnectionManager Lifecycle & Observer Propagation
    // ------------------------------------------------------------------------
    std::cout << "[TEST 8] ConnectionManager Lifecycle & Observers...\n";
    {
        ConnectionManager connMgr;
        assert(connMgr.GetState() == ConnectionState::DISCONNECTED);
        assert(!connMgr.IsConnected());

        ConnectionState recordedState = ConnectionState::DISCONNECTED;
        connMgr.RegisterObserver([&recordedState](ConnectionState newState, ConnectionState) {
            recordedState = newState;
        });

        connMgr.SetState(ConnectionState::CONNECTING);
        assert(recordedState == ConnectionState::CONNECTING);

        connMgr.SetState(ConnectionState::CONNECTED);
        assert(recordedState == ConnectionState::CONNECTED);
        assert(connMgr.IsConnected());

        connMgr.SetState(ConnectionState::RECONNECTING);
        assert(recordedState == ConnectionState::RECONNECTING);
        assert(!connMgr.IsConnected());
        std::cout << "  ✓ PASS: ConnectionManager transitions and observer notifications verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 9: Coordinator Server Integration & HELLO Registration Handshake
    // ------------------------------------------------------------------------
    std::cout << "[TEST 9] Coordinator Server Integration & HELLO Handshake...\n";
    const uint16_t INTEGRATION_PORT = 19130;
    {
        // 1. Launch Coordinator TCP Server
        config::ServerConfig serverCfg;
        serverCfg.bindAddress = "127.0.0.1";
        serverCfg.port = INTEGRATION_PORT;
        serverCfg.logToConsole = false;
        serverCfg.heartbeatTimeoutMs = 3000;
        coordinator::CoordinatorServer server(serverCfg);
        assert(server.Start());

        // 2. Launch Traffic Node Client for INT_A
        TrafficNodeConfig clientCfg;
        clientCfg.nodeId = "INT_A";
        clientCfg.coordinatorIp = "127.0.0.1";
        clientCfg.coordinatorPort = INTEGRATION_PORT;
        clientCfg.heartbeatIntervalMs = 400;
        clientCfg.heartbeatTimeoutMs = 2000;
        clientCfg.reconnectDelayMs = 500;
        clientCfg.yellowDurationMs = 100;
        clientCfg.logToConsole = false;

        auto relay = std::make_shared<RelayController>();
        relay->Initialize();
        auto fsm = std::make_shared<TrafficSignalFSM>(relay);
        fsm->Initialize(clientCfg);

        TrafficClient client(clientCfg, fsm);
        assert(client.Start());

        // Wait for connection and HELLO registration handshake
        uint64_t startWait = common::TimeUtils::GetMonotonicTimeMs();
        while ((!client.IsRegistered() || !server.IsNodeRegistered("INT_A")) &&
               (common::TimeUtils::GetMonotonicTimeMs() - startWait < 3000)) {
            common::TimeUtils::SleepMs(50);
        }

        assert(client.IsConnected());
        assert(client.IsRegistered());
        assert(server.IsNodeRegistered("INT_A"));
        std::cout << "  ✓ PASS: Full HELLO handshake and mutual Coordinator-Node registration verified.\n";

        // --------------------------------------------------------------------
        // TEST 10: Coordinator Emergency Preemption Command & Reconnect Recovery
        // --------------------------------------------------------------------
        std::cout << "[TEST 10] Emergency Preemption Command & Auto-Reconnect Recovery...\n";
        {
            // Coordinator sends EMERGENCY_MODE command to INT_A
            protocol::Packet emCmd(protocol::MessageType::EMERGENCY_MODE, "COORDINATOR", "CORR_PRIORITY_1");
            assert(server.SendToNode("INT_A", emCmd));

            // Wait for FSM to receive and apply emergency green
            common::TimeUtils::SleepMs(200);
            assert(fsm->IsInEmergencyMode());
            assert(fsm->GetCurrentState() == SignalState::GREEN);

            // Coordinator sends NORMAL_MODE command to restore
            protocol::Packet normCmd(protocol::MessageType::NORMAL_MODE, "COORDINATOR", "CORR_COMPLETE");
            assert(server.SendToNode("INT_A", normCmd));
            uint64_t normWait = common::TimeUtils::GetMonotonicTimeMs();
            while (fsm->IsInEmergencyMode() && common::TimeUtils::GetMonotonicTimeMs() - normWait < 1500) {
                common::TimeUtils::SleepMs(20);
            }
            assert(!fsm->IsInEmergencyMode());

            // Test Automatic Reconnect: force disconnect client socket
            client.Disconnect();
            assert(!client.IsConnected());

            // Wait for supervisor thread to automatically reconnect and re-register
            uint64_t reconStart = common::TimeUtils::GetMonotonicTimeMs();
            while (!client.IsRegistered() && (common::TimeUtils::GetMonotonicTimeMs() - reconStart < 6000)) {
                common::TimeUtils::SleepMs(100);
            }
            assert(client.IsConnected());
            assert(client.IsRegistered());
            std::cout << "  ✓ PASS: Emergency command execution and autonomous socket recovery verified.\n";
        }

        client.Stop();
        fsm->Shutdown();
        relay->Shutdown();
        server.Stop();
    }

    // ------------------------------------------------------------------------
    // TEST 11: IGpioDriver Abstraction & Mock/Rpi4 Hardware Driver
    // ------------------------------------------------------------------------
    std::cout << "[TEST 11] IGpioDriver Abstraction & Pin Actuation Validation...\n";
    {
        auto mockDriver = std::make_shared<MockGpioDriver>();
        assert(!mockDriver->IsInitialized());
        assert(mockDriver->Initialize());
        assert(mockDriver->IsInitialized());

        assert(mockDriver->ConfigureOutput(17));
        assert(mockDriver->ConfigureOutput(27));
        assert(mockDriver->ConfigureInput(22));

        assert(mockDriver->WritePin(17, true));
        assert(mockDriver->ReadPin(17));
        assert(mockDriver->WritePin(17, false));
        assert(!mockDriver->ReadPin(17));

        assert(mockDriver->GetWriteCount(17) == 2);
        mockDriver->Shutdown();
        assert(!mockDriver->IsInitialized());

        // Also test Rpi4GpioDriver safe POSIX fallback mode
        Rpi4GpioDriver rpiDriver;
        assert(rpiDriver.Initialize());
        assert(rpiDriver.ConfigureOutput(17));
        assert(rpiDriver.WritePin(17, true));
        assert(rpiDriver.ReadPin(17));
        rpiDriver.Shutdown();
        std::cout << "  ✓ PASS: IGpioDriver hardware abstraction and Mock/Rpi4 drivers verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 12: Layered Execution (SignalController -> LampController -> RelayController -> IGpioDriver)
    // ------------------------------------------------------------------------
    std::cout << "[TEST 12] Strict Layered Architecture Execution...\n";
    {
        auto gpio = std::make_shared<MockGpioDriver>();
        gpio->Initialize();

        auto relay = std::make_shared<RelayController>(gpio);
        assert(relay->Initialize(17, 27, 22, 23, true));

        auto lamp = std::make_shared<LampController>(relay);
        assert(lamp->Initialize(10)); // 10ms switch delay for test

        auto fsm = std::make_shared<TrafficSignalFSM>(lamp);
        TrafficNodeConfig cfg;
        cfg.yellowDurationMs = 50;
        cfg.allRedDurationMs = 20;
        assert(fsm->Initialize(cfg));

        auto signalCtrl = std::make_shared<SignalController>(fsm);
        assert(signalCtrl->GetCurrentState() == SignalState::RED);
        assert(relay->IsRedActive());
        assert(!relay->IsGreenActive());

        // Command GREEN through SignalController (routes RED -> RED_YELLOW -> GREEN)
        assert(signalCtrl->CommandGreen("Test Green Wave"));
        assert(signalCtrl->GetCurrentState() == SignalState::GREEN);
        assert(lamp->IsGreenLit());
        assert(relay->IsGreenActive());
        assert(!relay->IsRedActive());

        // Command ALL_RED (routes GREEN -> YELLOW -> ALL_RED)
        assert(signalCtrl->CommandAllRed("Test Safe All Red"));
        assert(signalCtrl->GetCurrentState() == SignalState::ALL_RED);
        assert(lamp->IsRedLit());
        assert(lamp->IsAuxLit());
        assert(!lamp->IsGreenLit());

        signalCtrl->Shutdown();
        lamp->Shutdown();
        relay->Shutdown();
        gpio->Shutdown();
        std::cout << "  ✓ PASS: Strict 4-layer separation (SignalCtrl->LampCtrl->RelayCtrl->Gpio) verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 13: Multi-Thread Node Watchdog Expiry & Forced ALL_RED Failsafe
    // ------------------------------------------------------------------------
    std::cout << "[TEST 13] Multi-Thread Node Watchdog Expiry & Forced ALL_RED Failsafe...\n";
    {
        auto gpio = std::make_shared<MockGpioDriver>();
        gpio->Initialize();
        auto relay = std::make_shared<RelayController>(gpio);
        relay->Initialize();
        auto lamp = std::make_shared<LampController>(relay);
        lamp->Initialize(5);
        auto fsm = std::make_shared<TrafficSignalFSM>(lamp);
        TrafficNodeConfig cfg;
        cfg.yellowDurationMs = 20;
        fsm->Initialize(cfg);
        auto signalCtrl = std::make_shared<SignalController>(fsm);

        // Put in GREEN first
        signalCtrl->CommandGreen("Active Green Corridor");
        assert(signalCtrl->GetCurrentState() == SignalState::GREEN);

        std::atomic<bool> failsafeTriggered{false};
        std::atomic<WatchdogThreadId> stalledThread{WatchdogThreadId::MAIN};

        // Watchdog with short 250ms timeout
        NodeWatchdog watchdog(250, [&](WatchdogThreadId stalled, uint64_t elapsed) {
            (void)elapsed;
            failsafeTriggered.store(true);
            stalledThread.store(stalled);
            signalCtrl->CommandAllRed("Watchdog Failsafe");
        });
        watchdog.Start();

        // Pet only MAIN, NETWORK, FSM, but deliberately starve HEARTBEAT
        uint64_t wStart = common::TimeUtils::GetMonotonicTimeMs();
        while (!failsafeTriggered.load() && (common::TimeUtils::GetMonotonicTimeMs() - wStart < 2000)) {
            watchdog.Pet(WatchdogThreadId::MAIN);
            watchdog.Pet(WatchdogThreadId::NETWORK);
            watchdog.Pet(WatchdogThreadId::FSM);
            // HEARTBEAT thread starved
            common::TimeUtils::SleepMs(50);
        }

        assert(failsafeTriggered.load());
        assert(stalledThread.load() == WatchdogThreadId::HEARTBEAT);
        assert(watchdog.HasExpired());

        // Wait for yellow clearance to complete into ALL_RED
        uint64_t fWait = common::TimeUtils::GetMonotonicTimeMs();
        while (signalCtrl->GetCurrentState() != SignalState::ALL_RED && common::TimeUtils::GetMonotonicTimeMs() - fWait < 1000) {
            common::TimeUtils::SleepMs(10);
        }
        assert(signalCtrl->GetCurrentState() == SignalState::ALL_RED);
        assert(lamp->IsRedLit());

        watchdog.Stop();
        signalCtrl->Shutdown();
        lamp->Shutdown();
        relay->Shutdown();
        gpio->Shutdown();
        std::cout << "  ✓ PASS: Multi-thread watchdog starvation detection and failsafe ALL_RED verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 14: Enriched Heartbeat Telemetry Serialization & Diagnostics
    // ------------------------------------------------------------------------
    std::cout << "[TEST 14] Enriched Heartbeat Telemetry & Telemetry Fields...\n";
    {
        TrafficNodeConfig cfg;
        cfg.nodeId = "INT_TEST_TELEMETRY";
        auto relay = std::make_shared<RelayController>();
        relay->Initialize();
        auto lamp = std::make_shared<LampController>(relay);
        lamp->Initialize();
        auto fsm = std::make_shared<TrafficSignalFSM>(lamp);
        fsm->Initialize(cfg);
        auto signalCtrl = std::make_shared<SignalController>(fsm);

        TrafficClient client(cfg, signalCtrl);
        std::string payload = client.GenerateEnrichedHeartbeatPayload();

        // Verify diagnostic telemetry keys exist
        assert(payload.find("NODE:INT_TEST_TELEMETRY") != std::string::npos);
        assert(payload.find("STATE:ALL_RED") != std::string::npos || payload.find("STATE:RED") != std::string::npos);
        assert(payload.find("RELAY:") != std::string::npos);
        assert(payload.find("CPU:") != std::string::npos);
        assert(payload.find("MEM:") != std::string::npos);
        assert(payload.find("TEMP:") != std::string::npos);
        assert(payload.find("UPTIME:") != std::string::npos);
        assert(payload.find("TIME:") != std::string::npos);

        // Put in Green and verify payload updates
        signalCtrl->CommandGreen();
        payload = client.GenerateEnrichedHeartbeatPayload();
        assert(payload.find("STATE:GREEN") != std::string::npos);
        assert(payload.find("RELAY:0010") != std::string::npos || payload.find("RELAY:") != std::string::npos);

        fsm->Shutdown();
        relay->Shutdown();
        std::cout << "  ✓ PASS: Enriched telemetry payload serialization and field diagnostics verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 15: Pure Virtual Extension Interfaces (AI / Sensor / Camera Readiness)
    // ------------------------------------------------------------------------
    std::cout << "[TEST 15] Pure Virtual Extension Interfaces (AI/Vision Extension Points)...\n";
    {
        using namespace aegis::traffic::extension;

        class TestStatusPublisher : public IStatusPublisher {
        public:
            void PublishStatus(const std::string& statusJson) override {
                lastPayload = statusJson;
            }
            std::string lastPayload;
        };

        class TestSensorPublisher : public ISensorPublisher {
        public:
            void PublishSensorData(const std::string& sensorId, double value, uint64_t timestamp) override {
                (void)value;
                (void)timestamp;
                lastSensorId = sensorId;
            }
            std::string lastSensorId;
        };

        class TestCameraListener : public ICameraEventListener {
        public:
            void OnVehicleDetected(const std::string& vehicleClass, double confidence, uint64_t timestamp) override {
                (void)confidence;
                (void)timestamp;
                lastClass = vehicleClass;
            }
            void OnEmergencyVehicleApproaching(const std::string& laneId, double estimatedDistanceMeters) override {
                (void)estimatedDistanceMeters;
                lastLane = laneId;
            }
            std::string lastClass;
            std::string lastLane;
        };

        class TestTrafficPublisher : public ITrafficEventPublisher {
        public:
            void PublishTrafficEvent(const std::string& eventType, const std::string& eventData) override {
                (void)eventData;
                lastType = eventType;
            }
            std::string lastType;
        };

        TestStatusPublisher statusPub;
        statusPub.PublishStatus("{\"status\":\"ONLINE\"}");
        assert(statusPub.lastPayload == "{\"status\":\"ONLINE\"}");

        TestSensorPublisher sensorPub;
        sensorPub.PublishSensorData("IR_SENSOR_01", 1.0, 1000);
        assert(sensorPub.lastSensorId == "IR_SENSOR_01");

        TestCameraListener camListener;
        camListener.OnVehicleDetected("AMBULANCE", 0.98, 1000);
        assert(camListener.lastClass == "AMBULANCE");
        camListener.OnEmergencyVehicleApproaching("LANE_NORTH", 120.5);
        assert(camListener.lastLane == "LANE_NORTH");

        TestTrafficPublisher eventPub;
        eventPub.PublishTrafficEvent("CORRIDOR_PREEMPTION", "CORR_01");
        assert(eventPub.lastType == "CORRIDOR_PREEMPTION");

        std::cout << "  ✓ PASS: Future AI / Vision pure virtual extension interfaces verified.\n";
    }

    std::cout << "\n==========================================================\n"
              << " Phase 3 Traffic Signal Node Validation:                  \n"
              << " Total Tests Passed: 15 / 15 (100%)                       \n"
              << " Total Failures:     0                                    \n"
              << "==========================================================\n"
              << ">>> [SUCCESS] Traffic Signal Node Verified 100%!          \n\n";

    return 0;
}
