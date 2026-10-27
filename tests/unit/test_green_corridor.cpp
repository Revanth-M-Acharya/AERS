/**
 * @file test_green_corridor.cpp
 * @brief Unit tests for the Coordinator Decision Engine.
 *
 * Verifies:
 * - Intersection state updates & queries
 * - Green corridor creation, lifecycle, and progression
 * - TrafficCommandManager packet generation
 * - CoordinatorManager subsystem ownership and corridor activation
 */

#include <iostream>
#include <cassert>
#include <vector>
#include <string>

#include "coordinator/IntersectionState.h"
#include "coordinator/GreenCorridor.h"
#include "coordinator/TrafficCommandManager.h"
#include "coordinator/CoordinatorManager.h"

using namespace aegis::coordinator;
using namespace aegis::communication;

void TestIntersectionStateUpdates() {
    std::cout << "[TEST] 1. IntersectionState Operations & Status Tracking..." << std::endl;

    IntersectionState stateMgr;
    assert(stateMgr.GetTotalCount() == 0);
    assert(stateMgr.GetOnlineCount() == 0);

    // Register intersections
    assert(stateMgr.RegisterIntersection("INT_101", 10));
    assert(stateMgr.RegisterIntersection("INT_102", 20));
    assert(stateMgr.GetTotalCount() == 2);
    assert(stateMgr.GetOnlineCount() == 2);
    assert(stateMgr.HasIntersection("INT_101"));

    // Verify initial states default to RED
    IntersectionStatus status1;
    assert(stateMgr.GetState("INT_101", status1));
    assert(status1.intersectionId == "INT_101");
    assert(status1.signalState == SignalState::RED);
    assert(status1.isOnline == true);
    assert(status1.currentPriority == 10);

    // Update signal state to GREEN
    uint64_t now = IntersectionState::CurrentTimestampMs();
    assert(stateMgr.UpdateState("INT_101", SignalState::GREEN, now, 50));
    assert(stateMgr.GetState("INT_101", status1));
    assert(status1.signalState == SignalState::GREEN);
    assert(status1.currentPriority == 50);
    assert(status1.lastHeartbeat == now);

    // Update signal state to YELLOW
    assert(stateMgr.UpdateState("INT_101", SignalState::YELLOW));
    assert(stateMgr.GetState("INT_101", status1));
    assert(status1.signalState == SignalState::YELLOW);

    // Offline / Online transitions
    assert(stateMgr.SetOffline("INT_101"));
    assert(stateMgr.GetState("INT_101", status1));
    assert(status1.isOnline == false);
    assert(stateMgr.GetOnlineCount() == 1);

    assert(stateMgr.SetOnline("INT_101"));
    assert(stateMgr.GetState("INT_101", status1));
    assert(status1.isOnline == true);
    assert(stateMgr.GetOnlineCount() == 2);

    // String enum helpers
    assert(std::string(SignalStateToString(SignalState::RED)) == "RED");
    assert(std::string(SignalStateToString(SignalState::YELLOW)) == "YELLOW");
    assert(std::string(SignalStateToString(SignalState::GREEN)) == "GREEN");

    assert(StringToSignalState("RED") == SignalState::RED);
    assert(StringToSignalState("YELLOW") == SignalState::YELLOW);
    assert(StringToSignalState("GREEN") == SignalState::GREEN);

    std::cout << "  ✓ Intersection state tracking and transitions verified." << std::endl;
}

void TestGreenCorridorLifecycle() {
    std::cout << "[TEST] 2. GreenCorridor Creation, Progression, and Lifecycle..." << std::endl;

    GreenCorridor corridor;
    assert(corridor.IsCreated());
    assert(!corridor.IsActive());

    std::vector<std::string> route = {"INT_A", "INT_B", "INT_C", "INT_D"};
    assert(corridor.CreateCorridor("CORR_MISSION_01", route));
    assert(corridor.GetCorridorId() == "CORR_MISSION_01");
    assert(corridor.GetTotalIntersections() == 4);
    assert(corridor.GetStatus() == CorridorStatus::CREATED);
    assert(corridor.CurrentIntersection() == "INT_A");
    assert(corridor.NextIntersection() == "INT_B");

    // Activate
    assert(corridor.Activate());
    assert(corridor.IsActive());
    assert(corridor.GetStatus() == CorridorStatus::ACTIVE);
    assert(corridor.GetCurrentIndex() == 0);
    assert(corridor.CurrentIntersection() == "INT_A");
    assert(corridor.NextIntersection() == "INT_B");

    // Progression along intersections
    assert(corridor.AdvanceToNextIntersection()); // Advance to INT_B
    assert(corridor.GetCurrentIndex() == 1);
    assert(corridor.CurrentIntersection() == "INT_B");
    assert(corridor.NextIntersection() == "INT_C");

    assert(corridor.AdvanceToNextIntersection()); // Advance to INT_C
    assert(corridor.GetCurrentIndex() == 2);
    assert(corridor.CurrentIntersection() == "INT_C");
    assert(corridor.NextIntersection() == "INT_D");

    assert(corridor.AdvanceToNextIntersection()); // Advance to INT_D (last)
    assert(corridor.GetCurrentIndex() == 3);
    assert(corridor.CurrentIntersection() == "INT_D");
    assert(corridor.NextIntersection() == "");

    // Advance past end -> auto-completes
    assert(!corridor.AdvanceToNextIntersection());
    assert(corridor.IsCompleted());
    assert(corridor.GetStatus() == CorridorStatus::COMPLETED);

    // Test explicit completion on a fresh corridor
    GreenCorridor corridor2("CORR_MISSION_02", {"INT_X", "INT_Y"});
    assert(corridor2.Activate());
    assert(corridor2.Complete());
    assert(corridor2.IsCompleted());

    // Test cancellation
    GreenCorridor corridor3("CORR_MISSION_03", {"INT_P", "INT_Q"});
    assert(corridor3.Activate());
    assert(corridor3.Cancel());
    assert(corridor3.IsCancelled());
    assert(!corridor3.IsActive());

    // Enum strings
    assert(std::string(CorridorStatusToString(CorridorStatus::CREATED)) == "CREATED");
    assert(std::string(CorridorStatusToString(CorridorStatus::ACTIVE)) == "ACTIVE");
    assert(std::string(CorridorStatusToString(CorridorStatus::COMPLETED)) == "COMPLETED");
    assert(std::string(CorridorStatusToString(CorridorStatus::CANCELLED)) == "CANCELLED");

    std::cout << "  ✓ GreenCorridor creation, step-by-step advance, completion, and cancellation verified." << std::endl;
}

void TestTrafficCommandManager() {
    std::cout << "[TEST] 3. TrafficCommandManager Packet Generation..." << std::endl;

    TrafficCommandManager cmdMgr("COORDINATOR_TEST");
    assert(cmdMgr.GetCoordinatorId() == "COORDINATOR_TEST");

    // 1. SET_GREEN
    Message greenMsg = cmdMgr.CreateGreenCommand("INT_101", 45);
    assert(greenMsg.GetType() == MessageType::COMMAND);
    assert(greenMsg.GetSenderId() == "COORDINATOR_TEST");
    std::string action, target;
    assert(TrafficCommandManager::ParseCommand(greenMsg, action, target));
    assert(action == TrafficCommands::SET_GREEN);
    assert(target == "INT_101");

    // 2. SET_RED
    Message redMsg = cmdMgr.CreateRedCommand("INT_102");
    assert(redMsg.GetType() == MessageType::COMMAND);
    assert(TrafficCommandManager::ParseCommand(redMsg, action, target));
    assert(action == TrafficCommands::SET_RED);
    assert(target == "INT_102");

    // 3. SET_NORMAL
    Message normalMsg = cmdMgr.CreateNormalCommand("INT_103");
    assert(normalMsg.GetType() == MessageType::COMMAND);
    assert(TrafficCommandManager::ParseCommand(normalMsg, action, target));
    assert(action == TrafficCommands::SET_NORMAL);
    assert(target == "INT_103");

    // 4. EMERGENCY_MODE_ON
    Message emergOnMsg = cmdMgr.CreateEmergencyOn("INT_104", "CORR_ALPHA");
    assert(emergOnMsg.GetType() == MessageType::COMMAND);
    assert(TrafficCommandManager::ParseCommand(emergOnMsg, action, target));
    assert(action == TrafficCommands::EMERGENCY_MODE_ON);
    assert(target == "INT_104");

    // 5. EMERGENCY_MODE_OFF
    Message emergOffMsg = cmdMgr.CreateEmergencyOff("INT_104", "CORR_ALPHA");
    assert(emergOffMsg.GetType() == MessageType::COMMAND);
    assert(TrafficCommandManager::ParseCommand(emergOffMsg, action, target));
    assert(action == TrafficCommands::EMERGENCY_MODE_OFF);
    assert(target == "INT_104");

    std::cout << "  ✓ TrafficCommandManager generates all 5 supported commands with valid payloads." << std::endl;
}

void TestCoordinatorManagerDecisionEngineIntegration() {
    std::cout << "[TEST] 4. CoordinatorManager Decision Engine & Corridor Preemption..." << std::endl;

    CoordinatorManager coordinator;

    // Verify ownership of subsystems
    assert(coordinator.GetIntersectionState().GetTotalCount() == 0);
    assert(coordinator.GetGreenCorridor().GetTotalIntersections() == 0);
    assert(coordinator.GetTrafficCommandManager().GetCoordinatorId() == "COORDINATOR");

    // Setup corridor route: INT_1 -> INT_2 -> INT_3
    std::vector<std::string> corridorRoute = {"INT_1", "INT_2", "INT_3"};
    assert(coordinator.CreateCorridor("EMERGENCY_CORR_99", corridorRoute));

    // Intersections should now be tracked in IntersectionState
    assert(coordinator.GetIntersectionState().GetTotalCount() == 3);
    assert(coordinator.GetGreenCorridor().GetCorridorId() == "EMERGENCY_CORR_99");

    // Activate Corridor
    assert(coordinator.ActivateCorridor());
    assert(coordinator.GetGreenCorridor().IsActive());

    // Verify Decision Logic:
    // Current intersection (INT_1) must be GREEN
    IntersectionStatus int1Status;
    assert(coordinator.GetIntersectionState().GetState("INT_1", int1Status));
    assert(int1Status.signalState == SignalState::GREEN);

    // Other intersections (INT_2, INT_3) must be RED
    IntersectionStatus int2Status, int3Status;
    assert(coordinator.GetIntersectionState().GetState("INT_2", int2Status));
    assert(int2Status.signalState == SignalState::RED);
    assert(coordinator.GetIntersectionState().GetState("INT_3", int3Status));
    assert(int3Status.signalState == SignalState::RED);

    // Verify generated commands are stored and not transmitted automatically
    size_t cmdCount = coordinator.GetGeneratedCommandsCount();
    assert(cmdCount == 3); // 1 GREEN (INT_1) + 2 RED (INT_2, INT_3)

    auto commands = coordinator.GetGeneratedCommands();
    assert(commands.size() == 3);

    std::string cmd1, target1;
    assert(TrafficCommandManager::ParseCommand(commands[0], cmd1, target1));
    assert(cmd1 == TrafficCommands::SET_GREEN);
    assert(target1 == "INT_1");

    std::string cmd2, target2;
    assert(TrafficCommandManager::ParseCommand(commands[1], cmd2, target2));
    assert(cmd2 == TrafficCommands::SET_RED);
    assert(target2 == "INT_2");

    std::string cmd3, target3;
    assert(TrafficCommandManager::ParseCommand(commands[2], cmd3, target3));
    assert(cmd3 == TrafficCommands::SET_RED);
    assert(target3 == "INT_3");

    // Pop commands and verify buffer clears
    auto popped = coordinator.PopGeneratedCommands();
    assert(popped.size() == 3);
    assert(coordinator.GetGeneratedCommandsCount() == 0);

    // Advance Corridor: ambulance moves from INT_1 to INT_2
    assert(coordinator.AdvanceCorridor());
    assert(coordinator.GetGreenCorridor().CurrentIntersection() == "INT_2");

    // INT_2 is now GREEN, INT_1 and INT_3 are RED
    assert(coordinator.GetIntersectionState().GetState("INT_2", int2Status));
    assert(int2Status.signalState == SignalState::GREEN);
    assert(coordinator.GetIntersectionState().GetState("INT_1", int1Status));
    assert(int1Status.signalState == SignalState::RED);
    assert(coordinator.GetIntersectionState().GetState("INT_3", int3Status));
    assert(int3Status.signalState == SignalState::RED);

    // Verify new commands generated
    assert(coordinator.GetGeneratedCommandsCount() >= 2);
    coordinator.ClearGeneratedCommands();
    assert(coordinator.GetGeneratedCommandsCount() == 0);

    // Complete Corridor
    assert(coordinator.CompleteCorridor());
    assert(coordinator.GetGreenCorridor().IsCompleted());

    // Normal commands generated for all intersections upon completion
    assert(coordinator.GetGeneratedCommandsCount() == 3);
    auto completeCmds = coordinator.PopGeneratedCommands();
    for (const auto& msg : completeCmds) {
        std::string act, tgt;
        assert(TrafficCommandManager::ParseCommand(msg, act, tgt));
        assert(act == TrafficCommands::SET_NORMAL);
    }

    std::cout << "  ✓ CoordinatorManager decision engine, preemption commands, and state tracking verified." << std::endl;
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Coordinator Decision Engine Test Suite" << std::endl;
    std::cout << "==========================================================" << std::endl;

    TestIntersectionStateUpdates();
    TestGreenCorridorLifecycle();
    TestTrafficCommandManager();
    TestCoordinatorManagerDecisionEngineIntegration();

    std::cout << "==========================================================" << std::endl;
    std::cout << " [ALL PASS] Coordinator Decision Engine Verified 100%!" << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
