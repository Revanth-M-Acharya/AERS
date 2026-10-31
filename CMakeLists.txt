cmake_minimum_required(VERSION 3.20)
project(SMART_AEGIS 
    VERSION 2.5.0 
    DESCRIPTION "SMART AEGIS - Distributed Smart Ambulance Emergency Routing & Traffic Corridor Preemption System"
    LANGUAGES C CXX
)

# -----------------------------------------------------------------------------
# 1. C++ Standard & Global Target Directories
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type: Debug, Release, RelWithDebInfo" FORCE)
endif()

# Output binaries in build/bin and libraries in build/lib
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

message(STATUS "SMART AEGIS - Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "SMART AEGIS - Target System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "SMART AEGIS - Output Directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# -----------------------------------------------------------------------------
# 2. Compiler Options & Platform Detection
# -----------------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "QNX" OR DEFINED QNX_TARGET_ARCH)
    set(AEGIS_PLATFORM_QNX TRUE)
    add_compile_definitions(AEGIS_TARGET_QNX=1 __QNX__=1 __QNXNTO__=1 _QNX_SOURCE=1)
    message(STATUS "SMART AEGIS - Configuring for QNX Neutrino RTOS 8.0 target")
else()
    set(AEGIS_PLATFORM_QNX FALSE)
    add_compile_definitions(AEGIS_TARGET_POSIX=1)
    message(STATUS "SMART AEGIS - Configuring for Desktop Host (macOS/Linux/Windows)")
endif()

if(MSVC)
    add_compile_options(/W4 /permissive- /EHsc)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(
        -Wall
        -Wextra
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Wnon-virtual-dtor
        -Wpedantic
    )
endif()

# -----------------------------------------------------------------------------
# 3. Dependencies
# -----------------------------------------------------------------------------
find_package(Threads REQUIRED)

set(PLATFORM_LIBS Threads::Threads)
if(WIN32)
    list(APPEND PLATFORM_LIBS ws2_32)
elseif(AEGIS_PLATFORM_QNX)
    list(APPEND PLATFORM_LIBS socket)
endif()

# Optional Qt6 Desktop GUI Support
find_package(Qt6 COMPONENTS Core Widgets Gui QUIET)
if(Qt6_FOUND)
    message(STATUS "SMART AEGIS - Qt6 detected. Enabling Qt desktop simulator target 'gui'.")
    set(CMAKE_AUTOMOC ON)
else()
    message(STATUS "SMART AEGIS - Qt6 not detected. Target 'gui' will build in headless console mode.")
endif()

# -----------------------------------------------------------------------------
# 4. Source Inventory (smart_aegis_core)
# -----------------------------------------------------------------------------
set(CORE_SOURCES
    src/common/geo_utils.cpp
    src/common/time_utils.cpp
    src/common/uuid_utils.cpp
    src/common/error.cpp
    src/common/event.cpp
    src/common/timer.cpp
    src/common/json_helper.cpp
    src/common/validator.cpp
    src/config/config_manager.cpp
    src/config/NodeConfig.cpp
    src/config/IntersectionsConfig.cpp
    src/gpio/SimulationGpioController.cpp
    src/gpio/QnxHardwareGpioController.cpp
    src/protocol/AegisProtocol.cpp
    src/intersection_service/IntersectionService.cpp
    src/coordinator_client/CoordinatorClientManager.cpp
    src/logger/logger.cpp
    src/logger/EventLogger.cpp
    src/logger/SimulationRecorder.cpp
    src/safety/watchdog.cpp
    src/safety/safety_monitor.cpp
    src/network/retry_policy.cpp
    src/network/serializer.cpp
    src/network/message_router.cpp
    src/network/authentication_manager.cpp
    src/network/connection_manager.cpp
    src/network/socket_client.cpp
    src/network/socket_server.cpp
    src/network/heartbeat_manager.cpp
    src/network/simulation_transport.cpp
    src/network/mosquitto_transport.cpp
    src/network/mqtt_client.cpp
    src/network/network_manager.cpp
    src/communication/Message.cpp
    src/communication/SocketServer.cpp
    src/communication/SocketClient.cpp
    src/priority_scheduler/priority_scheduler.cpp
    src/route_planner/route_planner.cpp
    src/coordinator/emergency_call_handler.cpp
    src/coordinator/incident_manager.cpp
    src/coordinator/ambulance_registry.cpp
    src/coordinator/fleet_manager.cpp
    src/coordinator/dispatch_engine.cpp
    src/coordinator/dynamic_green_corridor.cpp
    src/coordinator/coordinator.cpp
    src/coordinator/EventQueue.cpp
    src/coordinator/IntersectionRegistry.cpp
    src/coordinator/CoordinatorManager.cpp
    src/coordinator/IntersectionState.cpp
    src/coordinator/GreenCorridor.cpp
    src/coordinator/TrafficCommandManager.cpp
    src/coordinator/NodeRegistrationManager.cpp
    src/ambulance/gps_tracker.cpp
    src/ambulance/vitals_monitor.cpp
    src/ambulance/v2v_module.cpp
    src/ambulance/ambulance_node.cpp
    src/ambulance/ambulance_mission_simulator.cpp
    src/ambulance/AmbulanceProfile.cpp
    src/ambulance/AmbulanceRegistry.cpp
    src/traffic_controller/gpio_interface.cpp
    src/traffic_controller/signal_state_machine.cpp
    src/traffic_controller/preemption_manager.cpp
    src/traffic_controller/recovery_manager.cpp
    src/traffic_controller/traffic_controller.cpp
    src/traffic_controller/traffic_signal_node.cpp
    src/hospital_dashboard/hospital_dashboard_backend.cpp
    src/traffic_dashboard/traffic_dashboard_backend.cpp
    src/intersection/SignalController.cpp
    src/intersection/HeartbeatManager.cpp
    src/intersection/IntersectionNode.cpp
    src/system/SystemManager.cpp
    src/medical/PatientProfile.cpp
    src/medical/PriorityEngine.cpp
    src/medical/MedicalDatabase.cpp
    src/scheduler/ConflictDetector.cpp
    src/scheduler/PriorityScheduler.cpp
    src/scheduler/MissionManager.cpp
    src/scheduler/ConflictDetectionEngine.cpp
    src/scheduler/IntelligentPriorityScheduler.cpp
    src/scheduler/ConflictResolutionManager.cpp
    src/scheduler/ManualOverrideManager.cpp
    src/scheduler/AmbulanceAwarenessService.cpp
    src/scheduler/MultiAmbulanceSimulation.cpp
    src/control/ManualOverride.cpp
    src/control/EmergencyOperatorConsole.cpp
    src/traffic_signal/TrafficSignalNode.cpp
    src/green_corridor/EmergencyCorridorPlanner.cpp
    src/green_wave/GreenWaveCoordinator.cpp
    src/orchestrator/EmergencyOrchestrator.cpp
    src/replanning/DynamicRouteReplanner.cpp
    src/multi_corridor/MultiCorridorCoordinator.cpp
    src/distributed_signal/DistributedSignalSynchronizer.cpp
    src/predictive_traffic/PredictiveTrafficEngine.cpp
    src/fleet_management/FleetManagementEngine.cpp
    src/hospital_selection/HospitalSelectionEngine.cpp
    src/hospital_management/HospitalResourceEngine.cpp
    src/mission_execution/MissionExecutionEngine.cpp
    src/incident_command/IncidentCommandEngine.cpp
    src/traffic_network_orchestrator/TrafficNetworkOrchestrator.cpp
    src/distributed_consensus/DistributedConsensusEngine.cpp
    src/rpi_cluster/RaspberryPiClusterManager.cpp
    src/system_diagnostics/SystemDiagnosticsEngine.cpp
    src/hardware_abstraction/HardwareAbstractionLayer.cpp
    src/qnx_driver_framework/QNXDeviceDriverFramework.cpp
    src/bsp/RaspberryPi4BSP.cpp
    src/runtime/RuntimeEventFlow.cpp
    src/runtime/CoordinatorRuntime.cpp
    src/runtime/TrafficNodeRuntime.cpp
    src/runtime/DistributedRuntimeOrchestrator.cpp
    src/deployment/NodeConfiguration.cpp
    src/deployment/ConfigurationLoader.cpp
    src/deployment/DeploymentValidator.cpp
    src/deployment/RuntimeConfiguration.cpp
    src/deployment/NetworkDiscovery.cpp
    src/deployment/BootManager.cpp
    src/deployment/StartupManager.cpp
    src/deployment/DeploymentGpioDriver.cpp
    src/deployment/DeploymentLogger.cpp
    src/deployment/HeartbeatSupervisor.cpp
    src/deployment/GracefulShutdownManager.cpp
    src/deployment/DeploymentManager.cpp
    src/deployment/ClusterLauncher.cpp
    src/simulator/CityClock.cpp
    src/simulator/RoadGraph.cpp
    src/simulator/VirtualVehicle.cpp
    src/simulator/VirtualAmbulance.cpp
    src/simulator/VirtualIntersection.cpp
    src/simulator/VirtualHospital.cpp
    src/simulator/EmergencyGenerator.cpp
    src/simulator/CityModel.cpp
    src/hardware/HardwareConfiguration.cpp
    src/hardware/HardwareEventLogger.cpp
    src/hardware/RelayController.cpp
    src/hardware/TrafficLightController.cpp
    src/hardware/IntersectionHardware.cpp
    src/hardware/WirelessNodeManager.cpp
    src/hardware/HardwareHealthMonitor.cpp
    src/hardware/HardwareDiagnostics.cpp
    src/hardware/HardwareController.cpp
    src/hardware/OpticalSensorFilter.cpp
    src/hardware/GpioReservationManager.cpp
    src/hardware/HardwareWatchdog.cpp
    src/hardware/WirelessQualityMonitor.cpp
    src/hardware/BootManager.cpp
    src/hardware/ShutdownManager.cpp
    src/corridor/CorridorDeadlineMonitor.cpp
    src/corridor/EmergencyTimeoutManager.cpp
    src/corridor/DynamicCorridorReleaseManager.cpp
    src/corridor/RouteProgressTracker.cpp
    src/corridor/SignalTimelineGenerator.cpp
    src/cluster/ClusterHealthMonitor.cpp
    src/recovery/FaultRecoveryManager.cpp
    src/qnx/QnxDevctl.cpp
    src/qnx/QnxMessagePassing.cpp
    src/qnx/QnxPulseManager.cpp
    src/qnx/QnxTimerService.cpp
    src/qnx/QnxThreadScheduler.cpp
    src/qnx/QnxInterruptManager.cpp
    src/qnx/QnxResourceManager.cpp
    src/qnx/QnxDiagnostics.cpp
    network/message.cpp
    network/tcp_server.cpp
    network/tcp_client.cpp
    common/utils/TimeUtils.cpp
    common/utils/ThreadUtils.cpp
    common/protocol/Packet.cpp
    common/config/ServerConfig.cpp
    common/network/TcpSocket.cpp
    coordinator/logger/CoordinatorLogger.cpp
    coordinator/server/ClientSession.cpp
    coordinator/server/CoordinatorServer.cpp
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
    coordinator/intelligence/graph/RoadGraph.cpp
    coordinator/intelligence/planner/DijkstraRoutePlanner.cpp
    coordinator/intelligence/eta/ContinuousETAEngine.cpp
    coordinator/intelligence/scheduler/PriorityScheduler.cpp
    coordinator/intelligence/corridor/GreenCorridorPlanner.cpp
    coordinator/intelligence/execution/ExecutionPlanGenerator.cpp
    coordinator/intelligence/safety/SafetyPolicyEngine.cpp
    coordinator/intelligence/config/IntelligenceConfig.cpp
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
    digital_twin/CityModel.cpp
    digital_twin/SimulationClock.cpp
    digital_twin/TelemetryCollector.cpp
    digital_twin/DashboardEventBridge.cpp
    digital_twin/MissionTimeline.cpp
    digital_twin/MissionReplayEngine.cpp
    digital_twin/ScenarioPlaybackController.cpp
    digital_twin/SimulationScenarioManager.cpp
    digital_twin/visualizers/AmbulanceVisualizer.cpp
    digital_twin/visualizers/TrafficSignalVisualizer.cpp
    digital_twin/visualizers/CorridorVisualizer.cpp
    digital_twin/visualizers/CameraVisualizer.cpp
    digital_twin/visualizers/HospitalVisualizer.cpp
    digital_twin/visualizers/CoordinatorVisualizer.cpp
    digital_twin/PerformanceMonitor.cpp
    digital_twin/StatisticsEngine.cpp
    digital_twin/BenchmarkRecorder.cpp
    digital_twin/ReportExporter.cpp
    digital_twin/DashboardController.cpp
    digital_twin/DigitalTwinEngine.cpp
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
    hardware_integration/gpio/SymbolicGpioHAL.cpp
    hardware_integration/led/PrototypeLedDriver.cpp
    hardware_integration/relay/ProductionRelayDriver.cpp
    hardware_integration/sensor/IRSensorDriver.cpp
    hardware_integration/config/NodeConfigManager.cpp
    hardware_integration/commissioning/CommissioningManager.cpp
    hardware_integration/cluster/ClusterSupervisor.cpp
    hardware_integration/validation/DeploymentValidator.cpp
    hardware_integration/logging/CentralizedDeploymentLogger.cpp
    hardware_integration/demo/BuzzerController.cpp
    hardware_integration/demo/EmergencySwitchDriver.cpp
    hardware_integration/demo/PoliceOverrideSwitchDriver.cpp
    hardware_integration/demo/OledV2XDisplay.cpp
    hardware_integration/demo/Intersection2PriorityDemonstrator.cpp
    hardware_integration/demo/PoliceReplanningCoordinator.cpp
    hardware_integration/demo/HardwareDemoOrchestrator.cpp
    command_center/fleet/EmsFleetManager.cpp
    command_center/tracking/LiveAmbulanceTracker.cpp
    command_center/mission/MissionDatabase.cpp
    command_center/hospital/HospitalOperationsDashboard.cpp
    command_center/traffic/TrafficOperationsDashboard.cpp
    command_center/cluster/ClusterMonitor.cpp
    command_center/health/SystemHealthMonitor.cpp
    command_center/analytics/RealTimeAnalyticsEngine.cpp
    command_center/replay/MissionReplayController.cpp
    command_center/dashboard/CommandCenterConsole.cpp
    command_center/bridge/CommandCenterEventBridge.cpp
    command_center/CommandCenterFacade.cpp
    src/simulation/SimulationRunner.cpp
)

# -----------------------------------------------------------------------------
# 5. Core Library Target: smart_aegis_core
# -----------------------------------------------------------------------------
add_library(smart_aegis_core STATIC ${CORE_SOURCES})

target_include_directories(smart_aegis_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/network
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(smart_aegis_core PUBLIC ${PLATFORM_LIBS})

# -----------------------------------------------------------------------------
# 6. Executable Targets
# -----------------------------------------------------------------------------

# [Target 1] Central Coordinator Daemon
add_executable(coordinator src/apps/coordinator_main.cpp)
target_link_libraries(coordinator PRIVATE smart_aegis_core)

# [Target 2] Roadside Intersection Node
add_executable(intersection src/apps/intersection_main.cpp)
target_link_libraries(intersection PRIVATE smart_aegis_core)

# [Target 3] Desktop Simulator (Console Dashboard Runner)
add_executable(simulation src/apps/simulation_main.cpp)
target_link_libraries(simulation PRIVATE smart_aegis_core)

# [Target 4] Deployment & Health Validator
add_executable(validator src/apps/validator_main.cpp)
target_link_libraries(validator PRIVATE smart_aegis_core)

# [Target 5] Qt6 Desktop GUI Simulator
if(Qt6_FOUND)
    add_executable(gui src/apps/gui_main.cpp)
    target_compile_definitions(gui PRIVATE HAS_QT6=1)
    target_link_libraries(gui PRIVATE smart_aegis_core Qt6::Core Qt6::Widgets Qt6::Gui)
else()
    add_executable(gui src/apps/gui_main.cpp)
    target_link_libraries(gui PRIVATE smart_aegis_core)
endif()

# [Target 6] Hardware Deployment Target (Custom Target)
add_custom_target(hardware
    DEPENDS coordinator intersection validator
    COMMENT "SMART AEGIS - QNX Physical Hardware targets compiled."
)

# -----------------------------------------------------------------------------
# 7. Automated Verification Test Suite
# -----------------------------------------------------------------------------
enable_testing()

add_executable(test_smart_aegis_system tests/unit/test_smart_aegis_system.cpp)
target_link_libraries(test_smart_aegis_system PRIVATE smart_aegis_core)
add_test(NAME ComprehensiveSystemTest COMMAND test_smart_aegis_system)

# -----------------------------------------------------------------------------
# 8. Installation Rules
# -----------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS coordinator intersection simulation validator smart_aegis_core
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(TARGET gui)
    install(TARGETS gui RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

install(DIRECTORY config/ DESTINATION etc/aegis/config)
install(DIRECTORY scripts/ 
    DESTINATION share/aegis/scripts 
    FILE_PERMISSIONS 
        OWNER_READ OWNER_WRITE OWNER_EXECUTE 
        GROUP_READ GROUP_EXECUTE 
        WORLD_READ WORLD_EXECUTE
)
