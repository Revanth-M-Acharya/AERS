/**
 * @file test_coordinator_foundation.cpp
 * @brief Unit tests for CoordinatorManager, IntersectionRegistry, and EventQueue.
 */

#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>

#include "coordinator/EventQueue.h"
#include "coordinator/IntersectionRegistry.h"
#include "coordinator/CoordinatorManager.h"
#include "communication/SocketClient.h"

using namespace aegis::coordinator;
using namespace aegis::communication;

void TestEventQueue() {
    std::cout << "[TEST] 1. EventQueue FIFO, Concurrency & Shutdown..." << std::endl;

    EventQueue queue(100);
    assert(queue.Empty());
    assert(queue.Size() == 0);

    // Push events
    CoordinatorEvent ev1(CoordinatorEventType::MESSAGE_RECEIVED, 1, "INT_01", Message(MessageType::HEARTBEAT, "INT_01", "ALIVE"), 1000, "meta1");
    CoordinatorEvent ev2(CoordinatorEventType::CLIENT_CONNECTED, 2, "INT_02", Message(), 2000, "127.0.0.1:9001");

    queue.PushEvent(ev1);
    assert(!queue.Empty());
    assert(queue.Size() == 1);

    queue.PushEvent(ev2);
    assert(queue.Size() == 2);

    // Pop events in FIFO order
    CoordinatorEvent popped1;
    bool ok1 = queue.PopEvent(popped1, 100);
    assert(ok1);
    assert(popped1.type == CoordinatorEventType::MESSAGE_RECEIVED);
    assert(popped1.sourceId == "INT_01");
    assert(popped1.details == "meta1");

    CoordinatorEvent popped2;
    bool ok2 = queue.PopEvent(popped2, 100);
    assert(ok2);
    assert(popped2.type == CoordinatorEventType::CLIENT_CONNECTED);
    assert(popped2.sourceId == "INT_02");
    assert(popped2.details == "127.0.0.1:9001");

    assert(queue.Empty());

    // Timeout on empty queue
    CoordinatorEvent timeoutEvent;
    auto start = std::chrono::steady_clock::now();
    bool timeoutOk = queue.PopEvent(timeoutEvent, 50);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    assert(!timeoutOk);
    assert(elapsed >= 40);

    // Multi-threaded producer-consumer test
    std::atomic<int> consumeCount{0};
    std::atomic<bool> consumerRunning{true};

    std::thread consumer([&]() {
        while (consumerRunning.load()) {
            CoordinatorEvent ev;
            if (queue.PopEvent(ev, 20)) {
                consumeCount.fetch_add(1);
            }
        }
    });

    constexpr int NUM_PRODUCED = 50;
    std::vector<std::thread> producers;
    for (int i = 0; i < 5; ++i) {
        producers.emplace_back([&, i]() {
            for (int j = 0; j < 10; ++j) {
                CoordinatorEvent e(CoordinatorEventType::MESSAGE_RECEIVED, i * 10 + j, "PROD", Message(), 0, "");
                queue.PushEvent(e);
            }
        });
    }

    for (auto& t : producers) {
        t.join();
    }

    // Give consumer time to drain
    int retries = 50;
    while (consumeCount.load() < NUM_PRODUCED && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    assert(consumeCount.load() == NUM_PRODUCED);

    consumerRunning.store(false);
    consumer.join();

    // Test shutdown unblocks waiting PopEvent
    std::atomic<bool> blockedThreadExited{false};
    std::thread blockedThread([&]() {
        CoordinatorEvent ev;
        bool res = queue.PopEventBlocking(ev);
        assert(!res); // Expected false on shutdown
        blockedThreadExited.store(true);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    queue.Shutdown();
    blockedThread.join();
    assert(blockedThreadExited.load());

    std::cout << "  ✓ EventQueue verified: FIFO ordering, thread-safe queueing, timeouts, and shutdown." << std::endl;
}

void TestIntersectionRegistry() {
    std::cout << "[TEST] 2. IntersectionRegistry Registration & Heartbeat Telemetry..." << std::endl;

    IntersectionRegistry registry;
    assert(registry.GetTotalCount() == 0);
    assert(registry.GetOnlineCount() == 0);

    // Register intersections
    bool reg1 = registry.RegisterIntersection("INT_101", "192.168.1.101", 8080, 10);
    assert(reg1);
    assert(registry.GetTotalCount() == 1);
    assert(registry.GetOnlineCount() == 1);
    assert(registry.HasIntersection("INT_101"));

    bool reg2 = registry.RegisterIntersection("INT_102", "192.168.1.102", 8080, 11);
    assert(reg2);
    assert(registry.GetTotalCount() == 2);
    assert(registry.GetOnlineCount() == 2);

    // Lookup
    IntersectionEntry entry;
    assert(registry.GetIntersection("INT_101", entry));
    assert(entry.intersectionId == "INT_101");
    assert(entry.ipAddress == "192.168.1.101");
    assert(entry.port == 8080);
    assert(entry.isOnline == true);
    assert(entry.clientId == 10);

    // Client ID lookup
    std::string mappedId;
    assert(registry.FindByClientId(10, mappedId));
    assert(mappedId == "INT_101");

    // Optional lookup
    auto optEntry = registry.GetIntersection("INT_102");
    assert(optEntry.has_value());
    assert(optEntry->intersectionId == "INT_102");

    // Non-existent lookup
    assert(!registry.GetIntersection("NON_EXISTENT", entry));
    assert(!registry.GetIntersection("NON_EXISTENT").has_value());

    // GetAllIntersections
    auto all = registry.GetAllIntersections();
    assert(all.size() == 2);

    // Update heartbeat
    uint64_t oldHb = entry.lastHeartbeatTimestamp;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    uint64_t newHb = IntersectionRegistry::CurrentTimestampMs();
    assert(registry.UpdateHeartbeat("INT_101", newHb));

    assert(registry.GetIntersection("INT_101", entry));
    assert(entry.lastHeartbeatTimestamp == newHb);
    assert(entry.lastHeartbeatTimestamp >= oldHb);

    // Heartbeat timeout check
    // Manually register an intersection with ancient timestamp
    IntersectionEntry staleEntry("INT_STALE", "192.168.1.200", 8080, true, 1000, 12);
    assert(registry.RegisterIntersection(staleEntry));
    assert(registry.GetTotalCount() == 3);
    assert(registry.GetOnlineCount() == 3);

    // Check with 5000ms threshold (staleEntry timestamp 1000 is older than now - 5000ms)
    std::vector<std::string> timedOut = registry.CheckHeartbeatTimeouts(5000);
    assert(timedOut.size() == 1);
    assert(timedOut[0] == "INT_STALE");

    assert(registry.GetIntersection("INT_STALE", entry));
    assert(entry.isOnline == false);
    assert(registry.GetOnlineCount() == 2);

    // SetOnlineStatus
    assert(registry.SetOnlineStatus("INT_102", false));
    assert(registry.GetOnlineCount() == 1);

    // RemoveIntersection
    assert(registry.RemoveIntersection("INT_STALE"));
    assert(registry.GetTotalCount() == 2);
    assert(!registry.HasIntersection("INT_STALE"));

    std::cout << "  ✓ IntersectionRegistry verified: CRUD, client ID reverse index, heartbeat monitoring." << std::endl;
}

void TestCoordinatorManagerIntegration() {
    std::cout << "[TEST] 3. CoordinatorManager Lifecycle, Subsystem Ingestion & Dispatch..." << std::endl;

    const uint16_t testPort = 19552;
    CoordinatorManager coordinator;

    // Verify subsystem ownership access
    assert(coordinator.GetIntersectionRegistry().GetTotalCount() == 0);
    assert(coordinator.GetEventQueue().Empty());
    assert(!coordinator.IsRunning());

    // Start coordinator
    assert(coordinator.Start(testPort, "127.0.0.1"));
    assert(coordinator.IsRunning());

    // Connect an intersection node via SocketClient
    SocketClient intersectionClient;
    assert(intersectionClient.Connect("127.0.0.1", testPort, 3000));
    assert(intersectionClient.IsConnected());

    // Give coordinator dispatch loop a short moment
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Send REGISTER message from intersection
    Message regMsg(MessageType::REGISTER, "INT_NORTH_01", "{\"firmware\":\"v1.2.0\",\"lanes\":4}");
    assert(intersectionClient.Send(regMsg));

    // Intersection client should receive ACK back from coordinator
    Message ackFromCoord;
    bool ackOk = intersectionClient.Receive(ackFromCoord, 2000);
    assert(ackOk);
    assert(ackFromCoord.GetType() == MessageType::ACK);
    assert(ackFromCoord.GetSenderId() == "COORDINATOR");
    assert(ackFromCoord.GetPayload() == "REGISTER_OK");

    // Verify intersection is now registered in Coordinator's IntersectionRegistry
    int retries = 50;
    IntersectionEntry regEntry;
    while ((!coordinator.GetIntersectionRegistry().GetIntersection("INT_NORTH_01", regEntry) || !regEntry.isOnline) && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    assert(coordinator.GetIntersectionRegistry().HasIntersection("INT_NORTH_01"));
    assert(regEntry.isOnline == true);
    assert(regEntry.intersectionId == "INT_NORTH_01");

    // Send HEARTBEAT message from intersection
    uint64_t hbTime = Message::CurrentTimestampMs();
    Message hbMsg(MessageType::HEARTBEAT, "INT_NORTH_01", "PING", hbTime);
    assert(intersectionClient.Send(hbMsg));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(coordinator.GetIntersectionRegistry().GetIntersection("INT_NORTH_01", regEntry));
    assert(regEntry.lastHeartbeatTimestamp >= hbTime);

    // Test custom handler registration for COMMAND messages
    std::atomic<bool> commandDispatched{false};
    std::string receivedCommandPayload;
    coordinator.SetHandler(MessageType::COMMAND, [&](int, const Message& cmdMsg) {
        receivedCommandPayload = cmdMsg.GetPayload();
        commandDispatched.store(true);
    });

    Message cmdMsg(MessageType::COMMAND, "INT_NORTH_01", "REQUEST_CORRIDOR_PREEMPTION");
    assert(intersectionClient.Send(cmdMsg));

    retries = 50;
    while (!commandDispatched.load() && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    assert(commandDispatched.load());
    assert(receivedCommandPayload == "REQUEST_CORRIDOR_PREEMPTION");

    // Test coordinator sending message to intersection
    Message coordCmd(MessageType::COMMAND, "COORDINATOR", "SET_STAGE_HOLD_GREEN");
    assert(coordinator.SendMessageToIntersection("INT_NORTH_01", coordCmd));

    Message rxAtClient;
    assert(intersectionClient.Receive(rxAtClient, 2000));
    assert(rxAtClient.GetType() == MessageType::COMMAND);
    assert(rxAtClient.GetPayload() == "SET_STAGE_HOLD_GREEN");

    // Connect a second client to test BroadcastMessage
    SocketClient secondClient;
    assert(secondClient.Connect("127.0.0.1", testPort, 3000));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    Message broadcastMsg(MessageType::COMMAND, "COORDINATOR", "GLOBAL_CLOCK_RESYNC");
    assert(coordinator.BroadcastMessage(broadcastMsg));

    Message bcast1, bcast2;
    assert(intersectionClient.Receive(bcast1, 2000));
    assert(secondClient.Receive(bcast2, 2000));
    assert(bcast1.GetPayload() == "GLOBAL_CLOCK_RESYNC");
    assert(bcast2.GetPayload() == "GLOBAL_CLOCK_RESYNC");

    // Disconnect client and verify online status updates
    intersectionClient.Disconnect();
    secondClient.Disconnect();

    retries = 50;
    while (coordinator.GetIntersectionRegistry().GetOnlineCount() > 0 && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    assert(coordinator.GetIntersectionRegistry().GetIntersection("INT_NORTH_01", regEntry));
    assert(regEntry.isOnline == false);

    // Clean Stop
    coordinator.Stop();
    assert(!coordinator.IsRunning());

    std::cout << "  ✓ CoordinatorManager verified: lifecycle, network ingestion, registration ACK, dispatching, and broadcast." << std::endl;
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Coordinator Core Foundation Test Suite" << std::endl;
    std::cout << "==========================================================" << std::endl;

    TestEventQueue();
    TestIntersectionRegistry();
    TestCoordinatorManagerIntegration();

    std::cout << "==========================================================" << std::endl;
    std::cout << " [ALL PASS] Coordinator Core Foundation Verified 100%!" << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
