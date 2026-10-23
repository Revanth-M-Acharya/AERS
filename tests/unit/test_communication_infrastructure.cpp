#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <cmath>
#include <cstdlib>

#include "aegis/common/time_utils.hpp"
#include "aegis/network/retry_policy.hpp"
#include "aegis/network/serializer.hpp"
#include "aegis/network/message_router.hpp"
#include "aegis/network/authentication_manager.hpp"
#include "aegis/network/connection_manager.hpp"
#include "aegis/network/heartbeat_manager.hpp"
#include "aegis/network/socket_client.hpp"
#include "aegis/network/socket_server.hpp"
#include "aegis/network/mqtt_client.hpp"
#include "aegis/network/network_manager.hpp"

#define AEGIS_TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "\n[FAIL] Assertion failed: (" #cond ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::abort(); \
        } \
    } while (false)

using namespace aegis;
using namespace aegis::network;

// =============================================================================
// Unit Tests: Communication Infrastructure (Phase 2)
// =============================================================================

static void TestRetryPolicy() {
    std::cout << "[TEST] 1. RetryPolicy Exponential Backoff & Limits... ";

    RetryPolicy policy(50, 500, 2.0, 3);
    AEGIS_TEST_ASSERT(policy.CanRetry());
    AEGIS_TEST_ASSERT(policy.GetAttemptCount() == 0);

    // Initial delay: 50ms
    AEGIS_TEST_ASSERT(policy.GetNextDelayMs() == 50);

    // Attempt 1: 50 * 2^1 = 100ms
    policy.RecordAttempt();
    AEGIS_TEST_ASSERT(policy.GetAttemptCount() == 1);
    AEGIS_TEST_ASSERT(policy.CanRetry());
    AEGIS_TEST_ASSERT(policy.GetNextDelayMs() == 100);

    // Attempt 2: 50 * 2^2 = 200ms
    policy.RecordAttempt();
    AEGIS_TEST_ASSERT(policy.GetAttemptCount() == 2);
    AEGIS_TEST_ASSERT(policy.CanRetry());
    AEGIS_TEST_ASSERT(policy.GetNextDelayMs() == 200);

    // Attempt 3: 50 * 2^3 = 400ms
    policy.RecordAttempt();
    AEGIS_TEST_ASSERT(policy.GetAttemptCount() == 3);
    AEGIS_TEST_ASSERT(!policy.CanRetry()); // maxAttempts = 3 reached

    // Reset
    policy.Reset();
    AEGIS_TEST_ASSERT(policy.GetAttemptCount() == 0);
    AEGIS_TEST_ASSERT(policy.CanRetry());
    AEGIS_TEST_ASSERT(policy.GetNextDelayMs() == 50);

    std::cout << "PASSED" << std::endl;
}

static void TestSerializerRoundTrip() {
    std::cout << "[TEST] 2. Serializer ICD Round-Trip (All 10 Schemas)... ";

    // 1. EmergencyRequestMsg
    {
        EmergencyRequestMsg req;
        req.incidentId = "INC-2026-9001";
        req.timestamp = 1718000000123ULL;
        req.callerContact = "+91-9876543210";
        req.latitude = 12.971598;
        req.longitude = 77.594562;
        req.triageCategory = "CODE_RED";
        req.incidentType = "CARDIAC";
        req.requiredCapabilities = {"ECG", "VENTILATOR"};

        std::string json = Serializer::Serialize(req);
        AEGIS_TEST_ASSERT(!json.empty());
        AEGIS_TEST_ASSERT(json.find("INC-2026-9001") != std::string::npos);

        auto res = Serializer::DeserializeEmergencyRequest(json);
        AEGIS_TEST_ASSERT(res.IsOk());
        const auto& d = res.Value();
        AEGIS_TEST_ASSERT(d.incidentId == req.incidentId);
        AEGIS_TEST_ASSERT(d.timestamp == req.timestamp);
        AEGIS_TEST_ASSERT(d.callerContact == req.callerContact);
        AEGIS_TEST_ASSERT(std::abs(d.latitude - req.latitude) < 0.0001);
        AEGIS_TEST_ASSERT(std::abs(d.longitude - req.longitude) < 0.0001);
        AEGIS_TEST_ASSERT(d.triageCategory == req.triageCategory);
        AEGIS_TEST_ASSERT(d.incidentType == req.incidentType);
        AEGIS_TEST_ASSERT(d.requiredCapabilities.size() == 2);
    }

    // 2. GPSUpdateMsg
    {
        GPSUpdateMsg gps;
        gps.unitId = "AMB-01";
        gps.timestamp = 1718000000234ULL;
        gps.latitude = 12.972500;
        gps.longitude = 77.596000;
        gps.speed = 65.5f;
        gps.bearing = 88.2f;
        gps.hdop = 0.8f;
        gps.fixQuality = 2;

        std::string json = Serializer::Serialize(gps);
        auto res = Serializer::DeserializeGPSUpdate(json);
        AEGIS_TEST_ASSERT(res.IsOk());
        const auto& d = res.Value();
        AEGIS_TEST_ASSERT(d.unitId == "AMB-01");
        AEGIS_TEST_ASSERT(d.timestamp == 1718000000234ULL);
        AEGIS_TEST_ASSERT(std::abs(d.speed - 65.5f) < 0.01f);
        AEGIS_TEST_ASSERT(std::abs(d.bearing - 88.2f) < 0.01f);
        AEGIS_TEST_ASSERT(d.fixQuality == 2);
    }

    // 3. VitalsUpdateMsg
    {
        VitalsUpdateMsg vit;
        vit.unitId = "AMB-01";
        vit.patientId = "PAT-8812";
        vit.timestamp = 1718000000345ULL;
        vit.heartRate = 112;
        vit.spO2 = 94;
        vit.systolicBP = 145;
        vit.diastolicBP = 92;
        vit.respirationRate = 22;
        vit.gcsScore = 14;

        std::string json = Serializer::Serialize(vit);
        auto res = Serializer::DeserializeVitalsUpdate(json);
        AEGIS_TEST_ASSERT(res.IsOk());
        const auto& d = res.Value();
        AEGIS_TEST_ASSERT(d.unitId == "AMB-01");
        AEGIS_TEST_ASSERT(d.patientId == "PAT-8812");
        AEGIS_TEST_ASSERT(d.heartRate == 112);
        AEGIS_TEST_ASSERT(d.spO2 == 94);
        AEGIS_TEST_ASSERT(d.systolicBP == 145);
        AEGIS_TEST_ASSERT(d.diastolicBP == 92);
        AEGIS_TEST_ASSERT(d.respirationRate == 22);
        AEGIS_TEST_ASSERT(d.gcsScore == 14);
    }

    // 4. TrafficPreemptionRequestMsg
    {
        TrafficPreemptionRequestMsg pre;
        pre.requestId = "PR-001";
        pre.intersectionId = "INT-MG-ROAD-04";
        pre.unitId = "AMB-01";
        pre.corridorPriority = 2;
        pre.requestedPhase = "NORTH_SOUTH_GREEN";
        pre.estimatedArrivalSec = 18;
        pre.holdDurationSec = 30;
        pre.signature = "HMAC_SIG_789";

        std::string json = Serializer::Serialize(pre);
        auto res = Serializer::DeserializeTrafficPreemptionRequest(json);
        AEGIS_TEST_ASSERT(res.IsOk());
        const auto& d = res.Value();
        AEGIS_TEST_ASSERT(d.requestId == "PR-001");
        AEGIS_TEST_ASSERT(d.intersectionId == "INT-MG-ROAD-04");
        AEGIS_TEST_ASSERT(d.corridorPriority == 2);
        AEGIS_TEST_ASSERT(d.requestedPhase == "NORTH_SOUTH_GREEN");
        AEGIS_TEST_ASSERT(d.estimatedArrivalSec == 18);
        AEGIS_TEST_ASSERT(d.holdDurationSec == 30);
        AEGIS_TEST_ASSERT(d.signature == "HMAC_SIG_789");
    }

    // 5. TrafficStatusMsg
    {
        TrafficStatusMsg stat;
        stat.intersectionId = "INT-MG-ROAD-04";
        stat.timestamp = 1718000000456ULL;
        stat.currentMode = "PREEMPTION";
        stat.northSouthPhase = "GREEN";
        stat.eastWestPhase = "RED";
        stat.activePreemptionUnit = "AMB-01";
        stat.wdtHealthy = true;

        std::string json = Serializer::Serialize(stat);
        auto res = Serializer::DeserializeTrafficStatus(json);
        AEGIS_TEST_ASSERT(res.IsOk());
        const auto& d = res.Value();
        AEGIS_TEST_ASSERT(d.intersectionId == "INT-MG-ROAD-04");
        AEGIS_TEST_ASSERT(d.currentMode == "PREEMPTION");
        AEGIS_TEST_ASSERT(d.northSouthPhase == "GREEN");
        AEGIS_TEST_ASSERT(d.eastWestPhase == "RED");
        AEGIS_TEST_ASSERT(d.activePreemptionUnit == "AMB-01");
        AEGIS_TEST_ASSERT(d.wdtHealthy == true);
    }

    // 6. HospitalAlertMsg
    {
        HospitalAlertMsg alert;
        alert.alertId = "HA-4401";
        alert.unitId = "AMB-01";
        alert.patientId = "PAT-8812";
        alert.severity = "CRITICAL";
        alert.alarmType = "SPO2_DROP";
        alert.triggeredValue = "88%";
        alert.timestamp = 1718000000567ULL;

        std::string json = Serializer::Serialize(alert);
        auto res = Serializer::DeserializeHospitalAlert(json);
        AEGIS_TEST_ASSERT(res.IsOk());
        const auto& d = res.Value();
        AEGIS_TEST_ASSERT(d.alertId == "HA-4401");
        AEGIS_TEST_ASSERT(d.severity == "CRITICAL");
        AEGIS_TEST_ASSERT(d.alarmType == "SPO2_DROP");
        AEGIS_TEST_ASSERT(d.triggeredValue == "88%");
    }

    // 7. HospitalCommandMsg
    {
        HospitalCommandMsg cmd;
        cmd.commandId = "CMD-102";
        cmd.unitId = "AMB-01";
        cmd.physicianName = "Dr. Sharma";
        cmd.instruction = "Administer 100mg Aspirin";
        cmd.destinationBay = "TRAUMA_BAY_1";
        cmd.timestamp = 1718000000678ULL;

        std::string json = Serializer::Serialize(cmd);
        auto res = Serializer::DeserializeHospitalCommand(json);
        AEGIS_TEST_ASSERT(res.IsOk());
        const auto& d = res.Value();
        AEGIS_TEST_ASSERT(d.commandId == "CMD-102");
        AEGIS_TEST_ASSERT(d.physicianName == "Dr. Sharma");
        AEGIS_TEST_ASSERT(d.instruction == "Administer 100mg Aspirin");
        AEGIS_TEST_ASSERT(d.destinationBay == "TRAUMA_BAY_1");
    }

    // 8. PriorityDecisionMsg
    {
        PriorityDecisionMsg dec;
        dec.decisionId = "DEC-771";
        dec.contestedResourceId = "INT-MG-ROAD-04";
        dec.grantedUnitId = "AMB-01";
        dec.yieldingUnitId = "AMB-02";
        dec.grantedPriorityScore = 92.5f;
        dec.yieldingPriorityScore = 68.0f;
        dec.yieldAdvisorySpeedKmH = 25.0f;

        std::string json = Serializer::Serialize(dec);
        auto res = Serializer::DeserializePriorityDecision(json);
        AEGIS_TEST_ASSERT(res.IsOk());
        const auto& d = res.Value();
        AEGIS_TEST_ASSERT(d.decisionId == "DEC-771");
        AEGIS_TEST_ASSERT(d.grantedUnitId == "AMB-01");
        AEGIS_TEST_ASSERT(d.yieldingUnitId == "AMB-02");
        AEGIS_TEST_ASSERT(std::abs(d.grantedPriorityScore - 92.5f) < 0.01f);
        AEGIS_TEST_ASSERT(std::abs(d.yieldAdvisorySpeedKmH - 25.0f) < 0.01f);
    }

    // 9. HeartbeatMsg
    {
        HeartbeatMsg hb;
        hb.nodeId = "NODE-COORD-01";
        hb.timestamp = 1718000000789ULL;
        hb.cpuUsagePct = 14.5f;
        hb.ramUsageMb = 82.4f;
        hb.processStatus = "HEALTHY";
        hb.sequenceNumber = 104;

        std::string json = Serializer::Serialize(hb);
        auto res = Serializer::DeserializeHeartbeat(json);
        AEGIS_TEST_ASSERT(res.IsOk());
        const auto& d = res.Value();
        AEGIS_TEST_ASSERT(d.nodeId == "NODE-COORD-01");
        AEGIS_TEST_ASSERT(std::abs(d.cpuUsagePct - 14.5f) < 0.01f);
        AEGIS_TEST_ASSERT(d.sequenceNumber == 104);
        AEGIS_TEST_ASSERT(d.processStatus == "HEALTHY");
    }

    // 10. FaultEventMsg
    {
        FaultEventMsg flt;
        flt.faultId = "FLT-999";
        flt.originModule = "TRAFFIC_CONTROLLER";
        flt.errorCode = "WDT_TIMEOUT";
        flt.severity = "HIGH";
        flt.description = "Hardware watchdog tick timed out";
        flt.timestamp = 1718000000890ULL;

        std::string json = Serializer::Serialize(flt);
        auto res = Serializer::DeserializeFaultEvent(json);
        AEGIS_TEST_ASSERT(res.IsOk());
        const auto& d = res.Value();
        AEGIS_TEST_ASSERT(d.faultId == "FLT-999");
        AEGIS_TEST_ASSERT(d.errorCode == "WDT_TIMEOUT");
        AEGIS_TEST_ASSERT(d.description == "Hardware watchdog tick timed out");
    }

    // Invalid JSON test
    {
        auto errRes = Serializer::DeserializeEmergencyRequest("{ invalid json }");
        AEGIS_TEST_ASSERT(errRes.IsErr());
        AEGIS_TEST_ASSERT(errRes.Error() == common::ErrorCode::ERR_INVALID_MESSAGE);
    }

    std::cout << "PASSED" << std::endl;
}

static void TestMessageRouterWildcards() {
    std::cout << "[TEST] 3. MessageRouter Exact and Wildcard (+, #) Matching... ";

    // Static matching checks
    AEGIS_TEST_ASSERT(MessageRouter::MatchTopic("aegis/ambulance/AMB-01/gps", "aegis/ambulance/AMB-01/gps"));
    AEGIS_TEST_ASSERT(!MessageRouter::MatchTopic("aegis/ambulance/AMB-01/gps", "aegis/ambulance/AMB-02/gps"));

    // Single-level wildcard '+'
    AEGIS_TEST_ASSERT(MessageRouter::MatchTopic("aegis/ambulance/+/gps", "aegis/ambulance/AMB-01/gps"));
    AEGIS_TEST_ASSERT(MessageRouter::MatchTopic("aegis/ambulance/+/gps", "aegis/ambulance/AMB-99/gps"));
    AEGIS_TEST_ASSERT(!MessageRouter::MatchTopic("aegis/ambulance/+/gps", "aegis/ambulance/AMB-01/vitals"));
    AEGIS_TEST_ASSERT(!MessageRouter::MatchTopic("aegis/ambulance/+/gps", "aegis/ambulance/AMB-01/extra/gps"));

    // Multi-level wildcard '#'
    AEGIS_TEST_ASSERT(MessageRouter::MatchTopic("aegis/hospital/#", "aegis/hospital/alerts"));
    AEGIS_TEST_ASSERT(MessageRouter::MatchTopic("aegis/hospital/#", "aegis/hospital/commands/trauma/urgent"));
    AEGIS_TEST_ASSERT(!MessageRouter::MatchTopic("aegis/hospital/#", "aegis/traffic/status"));

    // Dispatch verification
    MessageRouter router;
    int exactMatchCount = 0;
    int wildcardPlusCount = 0;
    int wildcardHashCount = 0;

    router.Subscribe("aegis/ambulance/AMB-01/gps", [&](const std::string&, const std::string&) {
        exactMatchCount++;
    });

    router.Subscribe("aegis/ambulance/+/gps", [&](const std::string&, const std::string&) {
        wildcardPlusCount++;
    });

    router.Subscribe("aegis/ambulance/#", [&](const std::string&, const std::string&) {
        wildcardHashCount++;
    });

    // Publish to AMB-01 GPS: matches exact, +, #
    router.RouteMessage("aegis/ambulance/AMB-01/gps", "{\"lat\":12.9}");
    AEGIS_TEST_ASSERT(exactMatchCount == 1);
    AEGIS_TEST_ASSERT(wildcardPlusCount == 1);
    AEGIS_TEST_ASSERT(wildcardHashCount == 1);

    // Publish to AMB-02 GPS: matches +, # (not exact)
    router.RouteMessage("aegis/ambulance/AMB-02/gps", "{\"lat\":12.8}");
    AEGIS_TEST_ASSERT(exactMatchCount == 1);
    AEGIS_TEST_ASSERT(wildcardPlusCount == 2);
    AEGIS_TEST_ASSERT(wildcardHashCount == 2);

    // Publish to AMB-02 vitals: matches # only
    router.RouteMessage("aegis/ambulance/AMB-02/vitals", "{\"hr\":80}");
    AEGIS_TEST_ASSERT(exactMatchCount == 1);
    AEGIS_TEST_ASSERT(wildcardPlusCount == 2);
    AEGIS_TEST_ASSERT(wildcardHashCount == 3);

    // Clear subscriptions
    router.ClearSubscriptions();
    router.RouteMessage("aegis/ambulance/AMB-01/gps", "{\"lat\":12.9}");
    AEGIS_TEST_ASSERT(exactMatchCount == 1); // Not incremented

    std::cout << "PASSED" << std::endl;
}

static void TestAuthenticationManager() {
    std::cout << "[TEST] 4. AuthenticationManager Tokens, RBAC, HMAC & mTLS... ";

    AuthenticationManager auth;
    auth.SetSharedHmacSecret("TEST_SECRET_KEY_123");

    // Register tokens
    auth.RegisterApiToken("token-para-001", UserRole::PARAMEDIC, 3600);
    auth.RegisterApiToken("token-admin-999", UserRole::SYSTEM_ADMIN, 3600);

    // Validate valid token
    auto valRes = auth.ValidateToken("Bearer token-para-001");
    AEGIS_TEST_ASSERT(valRes.IsOk());
    AEGIS_TEST_ASSERT(valRes.Value().role == UserRole::PARAMEDIC);
    AEGIS_TEST_ASSERT(!valRes.Value().isExpired);

    // Validate invalid token
    auto invalidRes = auth.ValidateToken("Bearer invalid-token-xyz");
    AEGIS_TEST_ASSERT(invalidRes.IsErr());
    AEGIS_TEST_ASSERT(invalidRes.Error() == common::ErrorCode::ERR_INVALID_MESSAGE);

    // Authorize RBAC
    AEGIS_TEST_ASSERT(auth.AuthorizeRole(valRes.Value(), UserRole::PARAMEDIC));
    AEGIS_TEST_ASSERT(!auth.AuthorizeRole(valRes.Value(), UserRole::SYSTEM_ADMIN));

    auto adminRes = auth.ValidateToken("token-admin-999");
    AEGIS_TEST_ASSERT(adminRes.IsOk());
    AEGIS_TEST_ASSERT(auth.AuthorizeRole(adminRes.Value(), UserRole::SYSTEM_ADMIN));

    // HMAC Sign & Verify
    std::string payload = "{\"unitId\":\"AMB-01\",\"speed\":75.0}";
    std::string sig = auth.SignPayload(payload);
    AEGIS_TEST_ASSERT(!sig.empty());
    AEGIS_TEST_ASSERT(auth.VerifySignature(payload, sig));
    AEGIS_TEST_ASSERT(!auth.VerifySignature("{\"unitId\":\"AMB-01\",\"speed\":85.0}", sig));

    // mTLS certificate hook
    AEGIS_TEST_ASSERT(auth.VerifyNodeCertificate("CERT_DATA", "CA_DATA"));

    std::cout << "PASSED" << std::endl;
}

static void TestConnectionManager() {
    std::cout << "[TEST] 5. ConnectionManager State Transitions & Ping... ";

    ConnectionManager mgr;
    AEGIS_TEST_ASSERT(mgr.GetState() == ConnectionState::DISCONNECTED);
    AEGIS_TEST_ASSERT(!mgr.IsConnected());

    ConnectionState lastObserved = ConnectionState::DISCONNECTED;
    mgr.SetStateCallback([&](ConnectionState st) {
        lastObserved = st;
    });

    mgr.SetState(ConnectionState::CONNECTING);
    AEGIS_TEST_ASSERT(mgr.GetState() == ConnectionState::CONNECTING);
    AEGIS_TEST_ASSERT(lastObserved == ConnectionState::CONNECTING);
    AEGIS_TEST_ASSERT(!mgr.IsConnected());

    mgr.SetState(ConnectionState::CONNECTED);
    AEGIS_TEST_ASSERT(mgr.GetState() == ConnectionState::CONNECTED);
    AEGIS_TEST_ASSERT(mgr.IsConnected());
    AEGIS_TEST_ASSERT(lastObserved == ConnectionState::CONNECTED);

    mgr.RecordKeepAlivePing();
    AEGIS_TEST_ASSERT(mgr.GetLastPingTimeMs() > 0);

    mgr.SetState(ConnectionState::DISCONNECTED);
    AEGIS_TEST_ASSERT(!mgr.IsConnected());

    std::cout << "PASSED" << std::endl;
}

static void TestHeartbeatManager() {
    std::cout << "[TEST] 6. HeartbeatManager Generation & Staleness Detection... ";

    HeartbeatManager hbMgr("NODE-AMB-01", 50); // 50ms timeout threshold for rapid testing

    // Generate local heartbeat
    HeartbeatMsg hb1 = hbMgr.GenerateHeartbeat(15.2f, 120.0f, "HEALTHY");
    AEGIS_TEST_ASSERT(hb1.nodeId == "NODE-AMB-01");
    AEGIS_TEST_ASSERT(hb1.sequenceNumber == 1);
    AEGIS_TEST_ASSERT(hb1.processStatus == "HEALTHY");

    HeartbeatMsg hb2 = hbMgr.GenerateHeartbeat(18.0f, 122.0f, "HEALTHY");
    AEGIS_TEST_ASSERT(hb2.sequenceNumber == 2);

    // Record peer heartbeats
    HeartbeatMsg peerHb;
    peerHb.nodeId = "NODE-TRAFFIC-04";
    peerHb.sequenceNumber = 10;
    peerHb.timestamp = common::GetUtcMilliseconds();
    peerHb.processStatus = "HEALTHY";

    hbMgr.RecordPeerHeartbeat(peerHb);
    AEGIS_TEST_ASSERT(hbMgr.IsPeerAlive("NODE-TRAFFIC-04"));
    AEGIS_TEST_ASSERT(hbMgr.GetStalePeers().empty());

    // Wait for timeout to expire (> 50ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    AEGIS_TEST_ASSERT(!hbMgr.IsPeerAlive("NODE-TRAFFIC-04"));

    auto stalePeers = hbMgr.GetStalePeers();
    AEGIS_TEST_ASSERT(stalePeers.size() == 1);
    AEGIS_TEST_ASSERT(stalePeers[0] == "NODE-TRAFFIC-04");

    std::cout << "PASSED" << std::endl;
}

static void TestSocketCommunication() {
    std::cout << "[TEST] 7. SocketClient & SocketServer UDP Communication... ";

    const uint16_t testPort = 19876;
    SocketServer server;
    auto bindRes = server.BindUdp(testPort);
    AEGIS_TEST_ASSERT(bindRes.IsOk());
    AEGIS_TEST_ASSERT(server.IsBound());

    SocketClient client;
    auto openRes = client.OpenUdp(false);
    AEGIS_TEST_ASSERT(openRes.IsOk());
    AEGIS_TEST_ASSERT(client.IsOpen());

    // Send packet from client to server loopback
    const std::string testPacket = "AEGIS_V2V_BEACON_PAYLOAD";
    auto sendRes = client.SendTo("127.0.0.1", testPort, testPacket);
    AEGIS_TEST_ASSERT(sendRes.IsOk());

    // Poll server for incoming packet
    bool packetReceived = false;
    std::string receivedData;
    std::string senderIp;

    bool polled = server.PollOnce(200, [&](const std::string& ip, uint16_t, const std::string& payload) {
        packetReceived = true;
        senderIp = ip;
        receivedData = payload;
    });

    AEGIS_TEST_ASSERT(polled);
    AEGIS_TEST_ASSERT(packetReceived);
    AEGIS_TEST_ASSERT(receivedData == testPacket);
    AEGIS_TEST_ASSERT(!senderIp.empty());

    // Cleanup
    client.Close();
    server.Close();
    AEGIS_TEST_ASSERT(!client.IsOpen());
    AEGIS_TEST_ASSERT(!server.IsBound());

    std::cout << "PASSED" << std::endl;
}

static void TestMqttClientPubSubAndOfflineBuffering() {
    std::cout << "[TEST] 8. MqttClient Pub/Sub, QoS & Offline Buffering... ";

    MqttClient client;
    client.SetClientId("AMB-TEST-01");

    LastWill lwt{"aegis/ambulance/AMB-TEST-01/status", "OFFLINE", 1, true};
    client.SetLastWill(lwt);

    int receivedCount = 0;
    std::string lastReceivedTopic;
    std::string lastReceivedPayload;

    client.Subscribe("aegis/ambulance/+/status", [&](const std::string& topic, const std::string& payload) {
        receivedCount++;
        lastReceivedTopic = topic;
        lastReceivedPayload = payload;
    });

    // 1. Connected state publishing
    AEGIS_TEST_ASSERT(client.Connect("127.0.0.1", 1883, false));
    AEGIS_TEST_ASSERT(client.IsConnected());

    bool pubRes = client.Publish("aegis/ambulance/AMB-01/status", "ONLINE", 1);
    AEGIS_TEST_ASSERT(pubRes);
    AEGIS_TEST_ASSERT(receivedCount == 1);
    AEGIS_TEST_ASSERT(lastReceivedTopic == "aegis/ambulance/AMB-01/status");
    AEGIS_TEST_ASSERT(lastReceivedPayload == "ONLINE");

    // 2. Offline buffering when disconnected
    client.Disconnect();
    AEGIS_TEST_ASSERT(!client.IsConnected());
    AEGIS_TEST_ASSERT(client.GetBufferedMessageCount() == 0);

    // Publishing when offline stores in circular buffer
    pubRes = client.Publish("aegis/ambulance/AMB-01/status", "OFFLINE_QUEUED_1", 1);
    AEGIS_TEST_ASSERT(!pubRes); // Returns false when disconnected
    pubRes = client.Publish("aegis/ambulance/AMB-01/status", "OFFLINE_QUEUED_2", 1);
    AEGIS_TEST_ASSERT(!pubRes);

    AEGIS_TEST_ASSERT(client.GetBufferedMessageCount() == 2);
    AEGIS_TEST_ASSERT(receivedCount == 1); // Not delivered yet

    // 3. Reconnect flushes offline buffer
    AEGIS_TEST_ASSERT(client.Reconnect());
    AEGIS_TEST_ASSERT(client.IsConnected());
    AEGIS_TEST_ASSERT(client.GetBufferedMessageCount() == 0); // Flushed
    AEGIS_TEST_ASSERT(receivedCount == 3); // 2 buffered messages delivered!
    AEGIS_TEST_ASSERT(lastReceivedPayload == "OFFLINE_QUEUED_2");

    std::cout << "PASSED" << std::endl;
}

static void TestNetworkManagerFacade() {
    std::cout << "[TEST] 9. NetworkManager Unified Facade Lifecycle... ";

    NetworkManager netMgr;
    AEGIS_TEST_ASSERT(!netMgr.IsConnected());

    // Initialize facade
    bool initOk = netMgr.Initialize("127.0.0.1", 1883, false, "COORD-CENTRAL");
    AEGIS_TEST_ASSERT(initOk);
    AEGIS_TEST_ASSERT(netMgr.IsConnected());

    // Subscribe via facade
    int facadeReceived = 0;
    std::string receivedTopic;
    netMgr.Subscribe("aegis/traffic/#", [&](const std::string& topic, const std::string&) {
        facadeReceived++;
        receivedTopic = topic;
    });

    // Publish via facade
    bool pubOk = netMgr.Publish("aegis/traffic/INT-01/status", "{\"phase\":\"GREEN\"}", 1);
    AEGIS_TEST_ASSERT(pubOk);
    AEGIS_TEST_ASSERT(facadeReceived == 1);
    AEGIS_TEST_ASSERT(receivedTopic == "aegis/traffic/INT-01/status");

    // Access Auth subsystem
    netMgr.Auth().RegisterApiToken("sys-token", UserRole::DISPATCHER, 600);
    auto authRes = netMgr.Auth().ValidateToken("sys-token");
    AEGIS_TEST_ASSERT(authRes.IsOk());
    AEGIS_TEST_ASSERT(authRes.Value().role == UserRole::DISPATCHER);

    // Access Heartbeat subsystem
    HeartbeatMsg hb = netMgr.Heartbeat().GenerateHeartbeat(5.0f, 40.0f, "HEALTHY");
    AEGIS_TEST_ASSERT(hb.nodeId == "COORD-CENTRAL");
    AEGIS_TEST_ASSERT(hb.sequenceNumber == 1);

    // Send V2X beacon
    bool v2xSent = netMgr.SendV2xBeacon(19878, "AEGIS_EMERGENCY_BROADCAST");
    AEGIS_TEST_ASSERT(v2xSent);

    // Shutdown
    netMgr.Shutdown();
    AEGIS_TEST_ASSERT(!netMgr.IsConnected());

    std::cout << "PASSED" << std::endl;
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " Running SMART AEGIS Communication Infrastructure Test Suite" << std::endl;
    std::cout << "==========================================================" << std::endl;

    TestRetryPolicy();
    TestSerializerRoundTrip();
    TestMessageRouterWildcards();
    TestAuthenticationManager();
    TestConnectionManager();
    TestHeartbeatManager();
    TestSocketCommunication();
    TestMqttClientPubSubAndOfflineBuffering();
    TestNetworkManagerFacade();

    std::cout << "==========================================================" << std::endl;
    std::cout << " All 9 Communication Infrastructure Test Suites Passed! " << std::endl;
    std::cout << "==========================================================" << std::endl;
    return 0;
}
