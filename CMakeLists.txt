cmake_minimum_required(VERSION 3.20)
project(SMART_AEGIS 
    VERSION 1.0.0 
    DESCRIPTION "SMART AEGIS - Distributed Ambulance Emergency Response System"
    LANGUAGES C CXX
)

# ==============================================================================
# 1. C++ Standard & Global Compilation Flags
# ==============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build Type Default (Supports Debug, Release, Profile)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose Debug, Release, or Profile build" FORCE)
endif()

message(STATUS "SMART AEGIS - Build Configuration: ${CMAKE_BUILD_TYPE}")
message(STATUS "Target System: ${CMAKE_SYSTEM_NAME}")

# ==============================================================================
# 2. Strict Compiler Diagnostic Options
# ==============================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g3 -O0 -DDEBUG -DAEGIS_DEBUG)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Profile")
        add_compile_options(-O2 -g3 -DPROFILE -DAEGIS_PROFILE)
    else()
        add_compile_options(-O3 -DNDEBUG)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "QCC")
    # QNX Neutrino Compiler Options (SDP 8.0 / aarch64le default)
    if(NOT DEFINED QCC_TARGET_SPEC)
        set(QCC_TARGET_SPEC "gcc_ntoaarch64le")
    endif()
    add_compile_options(-V${QCC_TARGET_SPEC}_cpp -Y_gpp -lang-c++ -w8)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0 -DAEGIS_DEBUG)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Profile")
        add_compile_options(-O2 -g -p -DPROFILE -DAEGIS_PROFILE)
    else()
        add_compile_options(-O3 -DNDEBUG)
    endif()
endif()

# ==============================================================================
# 3. Platform & Target Options
# ==============================================================================
option(BUILD_TESTS "Build automated test suite" ON)
option(BUILD_SIMULATORS "Build simulation tools" ON)
option(ENABLE_QNX_TARGET "Target QNX Neutrino RTOS environment" OFF)

if(ENABLE_QNX_TARGET OR CMAKE_SYSTEM_NAME STREQUAL "QNX")
    add_compile_definitions(AEGIS_TARGET_QNX=1 __QNX__=1 __QNXNTO__=1 _QNX_SOURCE=1)
    message(STATUS "Configured for QNX Neutrino RTOS target environment")
else()
    add_compile_definitions(AEGIS_TARGET_POSIX=1)
    message(STATUS "Configured for POSIX / Linux / macOS host environment")
endif()

# ==============================================================================
# 4. Global Include Directories
# ==============================================================================
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/network
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ==============================================================================
# 5. Core Source Tree Subdirectories & Coordinator TCP Server
# ==============================================================================
set(COORD_P2_SRCS
    common/utils/TimeUtils.cpp
    common/utils/ThreadUtils.cpp
    common/protocol/Packet.cpp
    common/config/ServerConfig.cpp
    common/network/TcpSocket.cpp
    coordinator/logger/CoordinatorLogger.cpp
    coordinator/server/ClientSession.cpp
    coordinator/server/CoordinatorServer.cpp
)
add_library(coordinator_server_lib STATIC ${COORD_P2_SRCS})
target_include_directories(coordinator_server_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(coordinator_tcp_server coordinator/server/main.cpp)
target_link_libraries(coordinator_tcp_server PRIVATE coordinator_server_lib pthread)

# ==============================================================================
# Phase 3 Traffic Signal Node Client
# ==============================================================================
set(TRAFFIC_NODE_P3_SRCS
    traffic_node/config/TrafficNodeConfig.cpp
    traffic_node/logger/TrafficLogger.cpp
    traffic_node/gpio/MockGpioDriver.cpp
    traffic_node/gpio/Rpi4GpioDriver.cpp
    traffic_node/relay_controller/RelayController.cpp
    traffic_node/lamp_controller/LampController.cpp
    traffic_node/signal_controller/TrafficSignalFSM.cpp
    traffic_node/signal_controller/SignalController.cpp
    traffic_node/watchdog/NodeWatchdog.cpp
    traffic_node/client/ConnectionManager.cpp
    traffic_node/client/HeartbeatManager.cpp
    traffic_node/client/TrafficClient.cpp
    traffic_node/protocol/TrafficCommandDispatcher.cpp
)
add_library(traffic_node_client_lib STATIC ${TRAFFIC_NODE_P3_SRCS})
target_link_libraries(traffic_node_client_lib PUBLIC coordinator_server_lib pthread)
target_include_directories(traffic_node_client_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(traffic_signal_node traffic_node/main.cpp)
target_link_libraries(traffic_signal_node PRIVATE traffic_node_client_lib pthread)

# ==============================================================================
# Phase 4 Coordinator Intelligence Engine
# ==============================================================================
set(COORD_INTEL_SRCS
    coordinator/intelligence/graph/RoadGraph.cpp
    coordinator/intelligence/planner/DijkstraRoutePlanner.cpp
    coordinator/intelligence/eta/ContinuousETAEngine.cpp
    coordinator/intelligence/scheduler/PriorityScheduler.cpp
    coordinator/intelligence/corridor/GreenCorridorPlanner.cpp
    coordinator/intelligence/execution/ExecutionPlanGenerator.cpp
    coordinator/intelligence/safety/SafetyPolicyEngine.cpp
    coordinator/intelligence/config/IntelligenceConfig.cpp
)
add_library(coordinator_intel_lib STATIC ${COORD_INTEL_SRCS})
target_link_libraries(coordinator_intel_lib PUBLIC coordinator_server_lib pthread)
target_include_directories(coordinator_intel_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# ==============================================================================
# Phase 5 Coordinator Orchestration & Distributed Runtime
# ==============================================================================
set(COORD_ORCHESTRATION_SRCS
    coordinator/orchestration/state/CoordinatorStateManager.cpp
    coordinator/orchestration/event_bus/EventBus.cpp
    coordinator/orchestration/session/EmergencySessionManager.cpp
    coordinator/orchestration/fsm/CoordinatorFSM.cpp
    coordinator/orchestration/dispatcher/ExecutionDispatcher.cpp
    coordinator/orchestration/sync/NodeClockTracker.cpp
    coordinator/orchestration/monitor/NodeHealthMonitor.cpp
    coordinator/orchestration/monitor/ExecutionMonitor.cpp
    coordinator/orchestration/recovery/RecoveryManager.cpp
    coordinator/orchestration/config/OrchestrationConfig.cpp
)
add_library(coordinator_orchestration_lib STATIC ${COORD_ORCHESTRATION_SRCS})
target_link_libraries(coordinator_orchestration_lib PUBLIC coordinator_intel_lib coordinator_server_lib pthread)
target_include_directories(coordinator_orchestration_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# ==============================================================================
# Phase 6 Emergency Control Center & Hospital Coordination
# ==============================================================================
set(COORD_PHASE6_SRCS
    coordinator/control_center/EmergencyValidator.cpp
    coordinator/control_center/EmergencyPriorityEngine.cpp
    coordinator/control_center/IncidentManager.cpp
    coordinator/control_center/ResourceAllocator.cpp
    coordinator/control_center/DispatcherConsole.cpp
    coordinator/control_center/EmergencyControlCenter.cpp
    coordinator/hospital/HospitalRegistry.cpp
    coordinator/hospital/HospitalSelectionEngine.cpp
    coordinator/hospital/BedAvailabilityManager.cpp
    coordinator/hospital/HospitalNotificationManager.cpp
    coordinator/hospital/DoctorPreparationManager.cpp
    coordinator/hospital/PatientHandoverManager.cpp
)
add_library(coordinator_phase6_lib STATIC ${COORD_PHASE6_SRCS})
target_link_libraries(coordinator_phase6_lib PUBLIC coordinator_orchestration_lib coordinator_intel_lib coordinator_server_lib pthread)
target_include_directories(coordinator_phase6_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# ==============================================================================
# Phase 7 Intelligent Road Situation Awareness (IRSA)
# ==============================================================================
set(COORD_PHASE7_SRCS
    coordinator/perception/RoadConditionEvent.cpp
    coordinator/perception/CameraRegistry.cpp
    coordinator/perception/CameraFeedManager.cpp
    coordinator/perception/VideoStreamManager.cpp
    coordinator/perception/RoadConfidenceEngine.cpp
    coordinator/perception/RoadEventPublisher.cpp
    coordinator/perception/RoadSituationAwarenessEngine.cpp
    coordinator/perception/CoordinatorIntegration.cpp
    coordinator/perception/analytics/TrafficDensityAnalyzer.cpp
    coordinator/perception/analytics/QueueLengthEstimator.cpp
    coordinator/perception/analytics/LaneOccupancyEstimator.cpp
    coordinator/perception/analytics/RoadHealthAnalyzer.cpp
    coordinator/perception/analytics/IncidentDetectionEngine.cpp
    coordinator/perception/analytics/EmergencyVehicleDetectionEngine.cpp
    coordinator/perception/analytics/RoadBlockageAnalyzer.cpp
    coordinator/perception/analytics/CongestionAnalyzer.cpp
    coordinator/perception/analytics/TrafficPredictionEngine.cpp
    coordinator/perception/analytics/CameraHealthMonitor.cpp
    coordinator/perception/analytics/RoadSuitabilityCalculator.cpp
    coordinator/perception/analytics/HistoricalTrafficAnalytics.cpp
    coordinator/perception/digital_twin/PerceptionDigitalTwinBridge.cpp
)
add_library(coordinator_phase7_lib STATIC ${COORD_PHASE7_SRCS})
target_link_libraries(coordinator_phase7_lib PUBLIC coordinator_phase6_lib coordinator_orchestration_lib coordinator_intel_lib coordinator_server_lib pthread)
target_include_directories(coordinator_phase7_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# ==============================================================================
# Phase 8 Integrated Digital Twin, Operations Center & System Simulation
# ==============================================================================
set(COORD_PHASE8_SRCS
    digital_twin/CityModel.cpp
    digital_twin/DigitalTwinEngine.cpp
    digital_twin/SimulationClock.cpp
    digital_twin/SimulationScenarioManager.cpp
    digital_twin/MissionReplayEngine.cpp
    digital_twin/TelemetryCollector.cpp
    digital_twin/DashboardController.cpp
    digital_twin/DashboardEventBridge.cpp
    digital_twin/visualizers/AmbulanceVisualizer.cpp
    digital_twin/visualizers/TrafficSignalVisualizer.cpp
    digital_twin/visualizers/CorridorVisualizer.cpp
    digital_twin/visualizers/CameraVisualizer.cpp
    digital_twin/visualizers/HospitalVisualizer.cpp
    digital_twin/visualizers/CoordinatorVisualizer.cpp
    digital_twin/MissionTimeline.cpp
    digital_twin/PerformanceMonitor.cpp
    digital_twin/StatisticsEngine.cpp
    digital_twin/BenchmarkRecorder.cpp
    digital_twin/ScenarioPlaybackController.cpp
    digital_twin/ReportExporter.cpp
)
add_library(coordinator_phase8_lib STATIC ${COORD_PHASE8_SRCS})
target_link_libraries(coordinator_phase8_lib PUBLIC coordinator_phase7_lib coordinator_phase6_lib coordinator_orchestration_lib coordinator_intel_lib coordinator_server_lib pthread)
target_include_directories(coordinator_phase8_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# ==============================================================================
# Phase 9 Production Readiness, Reliability, Safety & Deployment
# ==============================================================================
set(COORD_PHASE9_SRCS
    production/health/HealthSupervisor.cpp
    production/health/WatchdogManager.cpp
    production/health/RuntimeDiagnostics.cpp
    production/health/HeartbeatSupervisor.cpp
    production/fault_tolerance/NodeRecoveryManager.cpp
    production/fault_tolerance/MissionContinuityManager.cpp
    production/fault_tolerance/LifecycleManager.cpp
    production/communication/ReliableChannel.cpp
    production/communication/PacketValidator.cpp
    production/security/NodeAuthenticator.cpp
    production/security/MessageSecurityGuard.cpp
    production/security/SecurityAuditLogger.cpp
    production/config/ProductionConfigManager.cpp
    production/config/VersionManager.cpp
    production/reliability/DeadlockDetector.cpp
    production/reliability/ResourceLeakMonitor.cpp
    production/reliability/SoakTestRunner.cpp
    production/fault_injection/FaultInjector.cpp
    production/benchmark/ProductionBenchmarkReporter.cpp
    production/ProductionEngine.cpp
)
add_library(coordinator_phase9_lib STATIC ${COORD_PHASE9_SRCS})
target_link_libraries(coordinator_phase9_lib PUBLIC coordinator_phase8_lib coordinator_phase7_lib coordinator_phase6_lib coordinator_orchestration_lib coordinator_intel_lib coordinator_server_lib pthread)
target_include_directories(coordinator_phase9_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(network)
add_subdirectory(src)

# ==============================================================================
# 6. Simulator Tools
# ==============================================================================
if(BUILD_SIMULATORS)
    add_subdirectory(tools/simulator)
endif()

# ==============================================================================
# 7. Automated Test Suites
# ==============================================================================
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
