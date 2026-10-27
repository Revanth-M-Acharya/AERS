/**
 * @file test_event_logger.cpp
 * @brief Unit tests for EventLogger and SimulationRecorder in SMART AEGIS.
 */

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <thread>
#include <fstream>
#include <cstdio>

#include "logger/EventLogger.h"
#include "logger/SimulationRecorder.h"

#define AEGIS_TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "\n[FAIL] Assertion failed: (" #cond ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::abort(); \
        } \
    } while (false)

using namespace aegis::logger;

// -----------------------------------------------------------------------------
// Test 1: Log All 13 Significant System Events
// -----------------------------------------------------------------------------
static void TestAllEventTypesLogged() {
    std::cout << "[TEST 1] Testing All 13 Significant System Event Types..." << std::endl;

    EventLogger logger;
    logger.Clear();
    AEGIS_TEST_ASSERT(logger.GetEventCount() == 0);

    // 1. Patient Created
    logger.LogPatientCreated("PT-101", "Severe acute cardiac arrest patient intake", EventSeverity::CRITICAL, "AMB-01");

    // 2. Ambulance Registered
    logger.LogAmbulanceRegistered("AMB-01", "ALS Unit registered with central fleet", EventSeverity::INFO, "INT_A");

    // 3. Priority Calculated
    logger.LogPriorityCalculated("AMB-01", "PT-101", "Calculated CRITICAL priority score: 95", EventSeverity::INFO);

    // 4. Conflict Detected
    logger.LogConflictDetected("INT_A", "Contention between AMB-01 and AMB-02 within 10s window", "AMB-01", EventSeverity::WARNING);

    // 5. Conflict Resolved
    logger.LogConflictResolved("INT_A", "AMB-01", "PriorityScheduler arbitrated AMB-01 winner over AMB-02", EventSeverity::INFO);

    // 6. Green Corridor Activated
    logger.LogGreenCorridorActivated("CORR-99", "AMB-01", "INT_A", "Corridor preemption active along route", EventSeverity::INFO);

    // 7. Green Corridor Completed
    logger.LogGreenCorridorCompleted("CORR-99", "AMB-01", "Mission successfully completed at hospital", EventSeverity::INFO);

    // 8. Manual Override
    logger.LogManualOverride("Operator manual Force Green applied", "AMB-01", "INT_B", EventSeverity::WARNING);

    // 9. Node Failure
    logger.LogNodeFailure("INT_C", "Roadside node missed 3 heartbeats - marked offline", EventSeverity::ERROR);

    // 10. Node Recovery
    logger.LogNodeRecovery("INT_C", "Roadside node re-established TCP connection and synced", EventSeverity::INFO);

    // 11. Heartbeats
    logger.LogHeartbeat("INT_A", "Periodic keepalive signal ok", EventSeverity::INFO);

    // 12. Registration
    logger.LogRegistration("INT_B", "New roadside intersection registered with coordinator", EventSeverity::INFO);

    // 13. Traffic Command
    logger.LogTrafficCommand("INT_A", "PREEMPT_GREEN command dispatched to controller", "AMB-01", EventSeverity::INFO);

    AEGIS_TEST_ASSERT(logger.GetEventCount() == 13);

    // Verify presence of all 13 types
    AEGIS_TEST_ASSERT(logger.GetEventsByType(EventType::PATIENT_CREATED).size() == 1);
    AEGIS_TEST_ASSERT(logger.GetEventsByType(EventType::AMBULANCE_REGISTERED).size() == 1);
    AEGIS_TEST_ASSERT(logger.GetEventsByType(EventType::PRIORITY_CALCULATED).size() == 1);
    AEGIS_TEST_ASSERT(logger.GetEventsByType(EventType::CONFLICT_DETECTED).size() == 1);
    AEGIS_TEST_ASSERT(logger.GetEventsByType(EventType::CONFLICT_RESOLVED).size() == 1);
    AEGIS_TEST_ASSERT(logger.GetEventsByType(EventType::GREEN_CORRIDOR_ACTIVATED).size() == 1);
    AEGIS_TEST_ASSERT(logger.GetEventsByType(EventType::GREEN_CORRIDOR_COMPLETED).size() == 1);
    AEGIS_TEST_ASSERT(logger.GetEventsByType(EventType::MANUAL_OVERRIDE).size() == 1);
    AEGIS_TEST_ASSERT(logger.GetEventsByType(EventType::NODE_FAILURE).size() == 1);
    AEGIS_TEST_ASSERT(logger.GetEventsByType(EventType::NODE_RECOVERY).size() == 1);
    AEGIS_TEST_ASSERT(logger.GetEventsByType(EventType::HEARTBEAT).size() == 1);
    AEGIS_TEST_ASSERT(logger.GetEventsByType(EventType::REGISTRATION).size() == 1);
    AEGIS_TEST_ASSERT(logger.GetEventsByType(EventType::TRAFFIC_COMMAND).size() == 1);

    std::cout << "  -> All 13 system event types logged and categorized properly." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 2: Event Fields & Context Integrity
// -----------------------------------------------------------------------------
static void TestEventFieldsAndContext() {
    std::cout << "[TEST 2] Testing Event Fields & Context Integrity..." << std::endl;

    EventLogger logger;
    logger.Clear();

    logger.Log(EventType::CONFLICT_DETECTED,
               EventSeverity::WARNING,
               "Junction conflict predicted at INT_CENTRAL",
               "AMB-LEAD",
               "PT-999",
               "INT_CENTRAL",
               {{"clearanceSec", "15"}, {"distanceMeters", "250"}});

    auto events = logger.GetAllEvents();
    AEGIS_TEST_ASSERT(events.size() == 1);

    const auto& ev = events[0];
    AEGIS_TEST_ASSERT(!ev.eventId.empty());
    AEGIS_TEST_ASSERT(ev.timestampEpochMs > 0);
    AEGIS_TEST_ASSERT(!ev.timestampIso8601.empty());
    AEGIS_TEST_ASSERT(ev.type == EventType::CONFLICT_DETECTED);
    AEGIS_TEST_ASSERT(ev.severity == EventSeverity::WARNING);
    AEGIS_TEST_ASSERT(ev.description == "Junction conflict predicted at INT_CENTRAL");
    AEGIS_TEST_ASSERT(ev.relatedAmbulance == "AMB-LEAD");
    AEGIS_TEST_ASSERT(ev.relatedPatient == "PT-999");
    AEGIS_TEST_ASSERT(ev.relatedIntersection == "INT_CENTRAL");
    AEGIS_TEST_ASSERT(ev.metadata.at("clearanceSec") == "15");

    std::string str = ev.ToString();
    AEGIS_TEST_ASSERT(str.find("CONFLICT_DETECTED") != std::string::npos);
    AEGIS_TEST_ASSERT(str.find("AMB-LEAD") != std::string::npos);
    AEGIS_TEST_ASSERT(str.find("PT-999") != std::string::npos);
    AEGIS_TEST_ASSERT(str.find("INT_CENTRAL") != std::string::npos);

    std::cout << "  -> Complete context fields validated." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 3: Event Querying and Filtering
// -----------------------------------------------------------------------------
static void TestQueriesAndFiltering() {
    std::cout << "[TEST 3] Testing Event Querying and Filtering..." << std::endl;

    EventLogger logger;
    logger.Clear();

    logger.LogPatientCreated("PT-A", "Patient A intake", EventSeverity::INFO, "AMB-1");
    logger.LogPatientCreated("PT-B", "Patient B intake", EventSeverity::CRITICAL, "AMB-2");
    logger.LogNodeFailure("INT_1", "Hardware reboot", EventSeverity::ERROR);
    logger.LogTrafficCommand("INT_1", "Green hold", "AMB-1", EventSeverity::INFO);
    logger.LogTrafficCommand("INT_2", "Green hold", "AMB-2", EventSeverity::INFO);

    // Filter by Ambulance
    auto amb1Events = logger.GetEventsForAmbulance("AMB-1");
    AEGIS_TEST_ASSERT(amb1Events.size() == 2);

    // Filter by Patient
    auto ptBEvents = logger.GetEventsForPatient("PT-B");
    AEGIS_TEST_ASSERT(ptBEvents.size() == 1);

    // Filter by Intersection
    auto int1Events = logger.GetEventsForIntersection("INT_1");
    AEGIS_TEST_ASSERT(int1Events.size() == 2);

    // Filter by Severity
    auto critEvents = logger.GetEventsBySeverity(EventSeverity::CRITICAL);
    AEGIS_TEST_ASSERT(critEvents.size() == 1);

    auto errEvents = logger.GetEventsBySeverity(EventSeverity::ERROR);
    AEGIS_TEST_ASSERT(errEvents.size() == 1);

    std::cout << "  -> Multi-attribute query filters verified." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 4: JSON Export and File Output
// -----------------------------------------------------------------------------
static void TestJsonExportAndSerialization() {
    std::cout << "[TEST 4] Testing JSON Export and File Output..." << std::endl;

    EventLogger logger;
    logger.Clear();

    logger.LogAmbulanceRegistered("AMB-TEST", "Unit online", EventSeverity::INFO, "INT_MAIN");
    logger.LogManualOverride("Dispatcher forced green", "AMB-TEST", "INT_MAIN", EventSeverity::WARNING);

    std::string jsonStr = logger.ExportJson();
    AEGIS_TEST_ASSERT(!jsonStr.empty());
    AEGIS_TEST_ASSERT(jsonStr.front() == '[');
    AEGIS_TEST_ASSERT(jsonStr.back() == ']');
    AEGIS_TEST_ASSERT(jsonStr.find("\"type\":\"AMBULANCE_REGISTERED\"") != std::string::npos);
    AEGIS_TEST_ASSERT(jsonStr.find("\"type\":\"MANUAL_OVERRIDE\"") != std::string::npos);
    AEGIS_TEST_ASSERT(jsonStr.find("\"relatedAmbulance\":\"AMB-TEST\"") != std::string::npos);
    AEGIS_TEST_ASSERT(jsonStr.find("\"relatedIntersection\":\"INT_MAIN\"") != std::string::npos);

    // Test Export to File
    const std::string tempFile = "/tmp/aegis_test_events.json";
    bool fileSaved = logger.ExportJsonToFile(tempFile);
    AEGIS_TEST_ASSERT(fileSaved);

    std::ifstream ifs(tempFile);
    AEGIS_TEST_ASSERT(ifs.is_open());
    std::string fileContent((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();
    std::remove(tempFile.c_str());

    AEGIS_TEST_ASSERT(fileContent == jsonStr);

    std::cout << "  -> JSON array formatting and file persistence validated." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 5: Event Listener Observer Pattern
// -----------------------------------------------------------------------------
static void TestEventListenerPattern() {
    std::cout << "[TEST 5] Testing Event Listener Observer Pattern..." << std::endl;

    EventLogger logger;
    logger.Clear();
    logger.ClearListeners();

    size_t notificationCount = 0;
    std::string lastEventDescription = "";

    logger.RegisterListener([&](const SystemEvent& ev) {
        ++notificationCount;
        lastEventDescription = ev.description;
    });

    logger.LogHeartbeat("INT_NODE_A", "Heartbeat tick 1");
    AEGIS_TEST_ASSERT(notificationCount == 1);
    AEGIS_TEST_ASSERT(lastEventDescription == "Heartbeat tick 1");

    logger.LogHeartbeat("INT_NODE_B", "Heartbeat tick 2");
    AEGIS_TEST_ASSERT(notificationCount == 2);
    AEGIS_TEST_ASSERT(lastEventDescription == "Heartbeat tick 2");

    logger.ClearListeners();
    logger.LogHeartbeat("INT_NODE_C", "Heartbeat tick 3");
    AEGIS_TEST_ASSERT(notificationCount == 2); // No new notification after clear

    std::cout << "  -> Synchronous event listener dispatch verified." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 6: SimulationRecorder Operations & Mission Playback
// -----------------------------------------------------------------------------
static void TestSimulationRecorderOperations() {
    std::cout << "[TEST 6] Testing SimulationRecorder Operations & Mission Playback..." << std::endl;

    EventLogger logger;
    logger.Clear();
    logger.ClearListeners();

    SimulationRecorder recorder("MISSION_SIM_404");
    AEGIS_TEST_ASSERT(!recorder.IsRecording());

    // 1. Start Recording
    recorder.StartRecording("MISSION_SIM_404");
    AEGIS_TEST_ASSERT(recorder.IsRecording());
    AEGIS_TEST_ASSERT(recorder.GetSimulationId() == "MISSION_SIM_404");
    AEGIS_TEST_ASSERT(recorder.GetStartTimeEpochMs() > 0);

    // 2. Attach to EventLogger
    recorder.AttachToLogger(logger);

    // 3. Log events through EventLogger
    logger.LogPatientCreated("PT-SIM-01", "Simulated cardiac incident", EventSeverity::CRITICAL, "AMB-SIM-01");
    logger.LogGreenCorridorActivated("CORR-SIM-01", "AMB-SIM-01", "INT_A", "Corridor active");

    // 4. Record simulation snapshots (frames)
    recorder.RecordSnapshot(0, "T0: Initial positions",
                           {{"AMB-SIM-01", "INT_A"}},
                           {{"INT_A", "GREEN"}, {"INT_B", "RED"}},
                           {"CORR-SIM-01"});

    recorder.RecordSnapshot(5000, "T+5s: Ambulance transiting INT_A",
                           {{"AMB-SIM-01", "TRANSIT_A_B"}},
                           {{"INT_A", "YELLOW"}, {"INT_B", "GREEN"}},
                           {"CORR-SIM-01"});

    // 5. Stop Recording
    recorder.StopRecording();
    AEGIS_TEST_ASSERT(!recorder.IsRecording());
    AEGIS_TEST_ASSERT(recorder.GetStopTimeEpochMs() >= recorder.GetStartTimeEpochMs());

    // Verify Counts
    AEGIS_TEST_ASSERT(recorder.GetEventCount() == 2);
    AEGIS_TEST_ASSERT(recorder.GetFrameCount() == 2);

    // 6. JSON Export
    std::string recJson = recorder.ExportRecordingJson();
    AEGIS_TEST_ASSERT(recJson.find("\"simulationId\":\"MISSION_SIM_404\"") != std::string::npos);
    AEGIS_TEST_ASSERT(recJson.find("\"eventCount\":2") != std::string::npos);
    AEGIS_TEST_ASSERT(recJson.find("\"frameCount\":2") != std::string::npos);
    AEGIS_TEST_ASSERT(recJson.find("\"events\":[") != std::string::npos);
    AEGIS_TEST_ASSERT(recJson.find("\"frames\":[") != std::string::npos);

    // 7. File export
    const std::string simFile = "/tmp/aegis_test_sim.json";
    bool fileSaved = recorder.ExportRecordingToFile(simFile);
    AEGIS_TEST_ASSERT(fileSaved);
    std::remove(simFile.c_str());

    // 8. Reset
    recorder.Reset();
    AEGIS_TEST_ASSERT(recorder.GetEventCount() == 0);
    AEGIS_TEST_ASSERT(recorder.GetFrameCount() == 0);

    std::cout << "  -> SimulationRecorder session lifecycle and timeline capture verified." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 7: Multi-threaded Concurrent Logging
// -----------------------------------------------------------------------------
static void TestThreadSafety() {
    std::cout << "[TEST 7] Testing Multi-threaded Concurrent Event Logging..." << std::endl;

    EventLogger logger;
    logger.Clear();

    const size_t numThreads = 8;
    const size_t logsPerThread = 50;

    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    for (size_t t = 0; t < numThreads; ++t) {
        threads.emplace_back([&logger, t]() {
            for (size_t i = 0; i < logsPerThread; ++i) {
                std::string ambId = "AMB_T" + std::to_string(t);
                std::string intId = "INT_T" + std::to_string(t);
                logger.LogHeartbeat(intId, "Concurrent heartbeat from thread " + std::to_string(t));
                logger.LogTrafficCommand(intId, "Concurrent command", ambId);
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    AEGIS_TEST_ASSERT(logger.GetEventCount() == numThreads * logsPerThread * 2);

    std::cout << "  -> Multi-threaded concurrency (" << (numThreads * logsPerThread * 2) << " events) verified safely." << std::endl;
}

// -----------------------------------------------------------------------------
// Main Test Runner
// -----------------------------------------------------------------------------
int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS - Event Logging & Simulation Recorder Tests " << std::endl;
    std::cout << "==========================================================" << std::endl;

    TestAllEventTypesLogged();
    TestEventFieldsAndContext();
    TestQueriesAndFiltering();
    TestJsonExportAndSerialization();
    TestEventListenerPattern();
    TestSimulationRecorderOperations();
    TestThreadSafety();

    std::cout << "==========================================================" << std::endl;
    std::cout << " [PASS] All 7 Event Logger Test Suites Passed!            " << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
