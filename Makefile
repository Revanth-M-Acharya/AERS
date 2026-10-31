# ==============================================================================
# SMART AEGIS: Universal Build Automation Makefile
# ==============================================================================

CXX ?= clang++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wnon-virtual-dtor
INCLUDES = -Iinclude -Isrc -Inetwork -I.

BUILD_TYPE ?= Debug
ifeq ($(BUILD_TYPE),Release)
    CXXFLAGS += -O3 -DNDEBUG
else
    CXXFLAGS += -g3 -O0 -DAEGIS_DEBUG
endif

BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin
LIB_DIR = $(BUILD_DIR)/lib
OBJ_DIR = $(BUILD_DIR)/obj

# Library Sources
COMMON_SRCS = src/common/geo_utils.cpp src/common/time_utils.cpp src/common/uuid_utils.cpp \
              src/common/error.cpp src/common/event.cpp src/common/timer.cpp \
              src/common/json_helper.cpp src/common/validator.cpp

CONFIG_SRCS = src/config/config_manager.cpp src/config/NodeConfig.cpp
LOGGER_SRCS = src/logger/logger.cpp src/logger/EventLogger.cpp src/logger/SimulationRecorder.cpp
SAFETY_SRCS = src/safety/watchdog.cpp src/safety/safety_monitor.cpp
NETWORK_SRCS = src/network/retry_policy.cpp src/network/serializer.cpp \
               src/network/message_router.cpp src/network/authentication_manager.cpp \
               src/network/connection_manager.cpp src/network/socket_client.cpp \
               src/network/socket_server.cpp src/network/heartbeat_manager.cpp \
               src/network/simulation_transport.cpp src/network/mosquitto_transport.cpp \
               src/network/mqtt_client.cpp src/network/network_manager.cpp
COMM_MODULE_SRCS = src/communication/Message.cpp src/communication/SocketServer.cpp src/communication/SocketClient.cpp
SCHED_SRCS = src/priority_scheduler/priority_scheduler.cpp
ROUTE_SRCS = src/route_planner/route_planner.cpp
COORD_SRCS = src/coordinator/emergency_call_handler.cpp \
             src/coordinator/incident_manager.cpp \
             src/coordinator/ambulance_registry.cpp \
             src/coordinator/fleet_manager.cpp \
             src/coordinator/dispatch_engine.cpp \
             src/coordinator/dynamic_green_corridor.cpp \
             src/coordinator/coordinator.cpp \
             src/coordinator/EventQueue.cpp \
             src/coordinator/IntersectionRegistry.cpp \
             src/coordinator/CoordinatorManager.cpp \
             src/coordinator/IntersectionState.cpp \
             src/coordinator/GreenCorridor.cpp \
             src/coordinator/TrafficCommandManager.cpp \
             src/coordinator/NodeRegistrationManager.cpp
AMB_SRCS = src/ambulance/gps_tracker.cpp src/ambulance/vitals_monitor.cpp src/ambulance/v2v_module.cpp src/ambulance/ambulance_node.cpp src/ambulance/ambulance_mission_simulator.cpp \
           src/ambulance/AmbulanceProfile.cpp src/ambulance/AmbulanceRegistry.cpp
TRAFFIC_SRCS = src/traffic_controller/gpio_interface.cpp \
               src/traffic_controller/signal_state_machine.cpp \
               src/traffic_controller/preemption_manager.cpp \
               src/traffic_controller/recovery_manager.cpp \
               src/traffic_controller/traffic_controller.cpp \
               src/traffic_controller/traffic_signal_node.cpp
HOSP_SRCS = src/hospital_dashboard/hospital_dashboard_backend.cpp
TRFDASH_SRCS = src/traffic_dashboard/traffic_dashboard_backend.cpp
INTERSECTION_SRCS = src/intersection/SignalController.cpp \
                    src/intersection/HeartbeatManager.cpp \
                    src/intersection/IntersectionNode.cpp
SYSTEM_SRCS = src/system/SystemManager.cpp
MEDICAL_SRCS = src/medical/PatientProfile.cpp src/medical/PriorityEngine.cpp src/medical/MedicalDatabase.cpp
SCHEDULER_SRCS = src/scheduler/ConflictDetector.cpp src/scheduler/PriorityScheduler.cpp \
                 src/scheduler/MissionManager.cpp src/scheduler/ConflictDetectionEngine.cpp \
                 src/scheduler/IntelligentPriorityScheduler.cpp src/scheduler/ConflictResolutionManager.cpp \
                 src/scheduler/ManualOverrideManager.cpp src/scheduler/AmbulanceAwarenessService.cpp \
                 src/scheduler/MultiAmbulanceSimulation.cpp
CONTROL_SRCS = src/control/ManualOverride.cpp src/control/EmergencyOperatorConsole.cpp
TRAFFIC_SIGNAL_SRCS = src/traffic_signal/TrafficSignalNode.cpp
GREEN_CORRIDOR_SRCS = src/green_corridor/EmergencyCorridorPlanner.cpp
GREEN_WAVE_SRCS = src/green_wave/GreenWaveCoordinator.cpp
ORCHESTRATOR_SRCS = src/orchestrator/EmergencyOrchestrator.cpp
REPLANNING_SRCS = src/replanning/DynamicRouteReplanner.cpp
MULTI_CORRIDOR_SRCS = src/multi_corridor/MultiCorridorCoordinator.cpp
DISTRIBUTED_SIGNAL_SRCS = src/distributed_signal/DistributedSignalSynchronizer.cpp
PREDICTIVE_TRAFFIC_SRCS = src/predictive_traffic/PredictiveTrafficEngine.cpp
FLEET_MANAGEMENT_SRCS = src/fleet_management/FleetManagementEngine.cpp
HOSPITAL_SELECTION_SRCS = src/hospital_selection/HospitalSelectionEngine.cpp
HOSPITAL_MANAGEMENT_SRCS = src/hospital_management/HospitalResourceEngine.cpp
MISSION_EXECUTION_SRCS = src/mission_execution/MissionExecutionEngine.cpp
INCIDENT_COMMAND_SRCS = src/incident_command/IncidentCommandEngine.cpp
TRAFFIC_NETWORK_ORCHESTRATOR_SRCS = src/traffic_network_orchestrator/TrafficNetworkOrchestrator.cpp
DISTRIBUTED_CONSENSUS_SRCS = src/distributed_consensus/DistributedConsensusEngine.cpp
RPI_CLUSTER_SRCS = src/rpi_cluster/RaspberryPiClusterManager.cpp
SYSTEM_DIAGNOSTICS_SRCS = src/system_diagnostics/SystemDiagnosticsEngine.cpp
HARDWARE_ABSTRACTION_SRCS = src/hardware_abstraction/HardwareAbstractionLayer.cpp
QNX_DRIVER_FRAMEWORK_SRCS = src/qnx_driver_framework/QNXDeviceDriverFramework.cpp
BSP_SRCS = src/bsp/RaspberryPi4BSP.cpp
RUNTIME_SRCS = src/runtime/RuntimeEventFlow.cpp src/runtime/CoordinatorRuntime.cpp \
               src/runtime/TrafficNodeRuntime.cpp src/runtime/DistributedRuntimeOrchestrator.cpp
DEPLOYMENT_SRCS = src/deployment/NodeConfiguration.cpp \
                  src/deployment/ConfigurationLoader.cpp \
                  src/deployment/DeploymentValidator.cpp \
                  src/deployment/RuntimeConfiguration.cpp \
                  src/deployment/NetworkDiscovery.cpp \
                  src/deployment/BootManager.cpp \
                  src/deployment/StartupManager.cpp \
                  src/deployment/DeploymentGpioDriver.cpp \
                  src/deployment/DeploymentLogger.cpp \
                  src/deployment/HeartbeatSupervisor.cpp \
                  src/deployment/GracefulShutdownManager.cpp \
                  src/deployment/DeploymentManager.cpp \
                  src/deployment/ClusterLauncher.cpp
SIMULATOR_CORE_SRCS = src/simulator/CityClock.cpp src/simulator/RoadGraph.cpp src/simulator/VirtualVehicle.cpp \
                      src/simulator/VirtualAmbulance.cpp src/simulator/VirtualIntersection.cpp \
                      src/simulator/VirtualHospital.cpp src/simulator/EmergencyGenerator.cpp src/simulator/CityModel.cpp
HARDWARE_SRCS = src/hardware/HardwareConfiguration.cpp \
                src/hardware/HardwareEventLogger.cpp \
                src/hardware/RelayController.cpp \
                src/hardware/TrafficLightController.cpp \
                src/hardware/IntersectionHardware.cpp \
                src/hardware/WirelessNodeManager.cpp \
                src/hardware/HardwareHealthMonitor.cpp \
                src/hardware/HardwareDiagnostics.cpp \
                src/hardware/HardwareController.cpp \
                src/hardware/OpticalSensorFilter.cpp \
                src/hardware/GpioReservationManager.cpp \
                src/hardware/HardwareWatchdog.cpp \
                src/hardware/WirelessQualityMonitor.cpp \
                src/hardware/BootManager.cpp \
                src/hardware/ShutdownManager.cpp
PHASE3_SRCS = src/corridor/CorridorDeadlineMonitor.cpp \
              src/corridor/EmergencyTimeoutManager.cpp \
              src/corridor/DynamicCorridorReleaseManager.cpp \
              src/corridor/RouteProgressTracker.cpp \
              src/corridor/SignalTimelineGenerator.cpp \
              src/cluster/ClusterHealthMonitor.cpp \
              src/recovery/FaultRecoveryManager.cpp
PHASE4_SRCS = src/qnx/QnxDevctl.cpp \
              src/qnx/QnxMessagePassing.cpp \
              src/qnx/QnxPulseManager.cpp \
              src/qnx/QnxTimerService.cpp \
              src/qnx/QnxThreadScheduler.cpp \
              src/qnx/QnxInterruptManager.cpp \
              src/qnx/QnxResourceManager.cpp \
              src/qnx/QnxDiagnostics.cpp
QNX_NET_SRCS = network/message.cpp network/tcp_server.cpp network/tcp_client.cpp

# Phase 2 Coordinator TCP Server Sources
COORD_P2_SRCS = common/utils/TimeUtils.cpp \
                common/utils/ThreadUtils.cpp \
                common/protocol/Packet.cpp \
                common/config/ServerConfig.cpp \
                common/network/TcpSocket.cpp \
                coordinator/logger/CoordinatorLogger.cpp \
                coordinator/server/ClientSession.cpp \
                coordinator/server/CoordinatorServer.cpp

ALL_SRCS = $(COMMON_SRCS) $(CONFIG_SRCS) $(LOGGER_SRCS) $(SAFETY_SRCS) $(NETWORK_SRCS) \
           $(COMM_MODULE_SRCS) $(SCHED_SRCS) $(ROUTE_SRCS) $(COORD_SRCS) $(AMB_SRCS) $(TRAFFIC_SRCS) \
           $(INTERSECTION_SRCS) $(SYSTEM_SRCS) $(MEDICAL_SRCS) $(SCHEDULER_SRCS) $(CONTROL_SRCS) $(HOSP_SRCS) $(TRFDASH_SRCS) \
           $(TRAFFIC_SIGNAL_SRCS) $(GREEN_CORRIDOR_SRCS) $(GREEN_WAVE_SRCS) $(ORCHESTRATOR_SRCS) $(REPLANNING_SRCS) $(MULTI_CORRIDOR_SRCS) $(DISTRIBUTED_SIGNAL_SRCS) $(PREDICTIVE_TRAFFIC_SRCS) $(FLEET_MANAGEMENT_SRCS) $(HOSPITAL_SELECTION_SRCS) $(HOSPITAL_MANAGEMENT_SRCS) $(MISSION_EXECUTION_SRCS) $(INCIDENT_COMMAND_SRCS) $(TRAFFIC_NETWORK_ORCHESTRATOR_SRCS) $(DISTRIBUTED_CONSENSUS_SRCS) $(RPI_CLUSTER_SRCS) $(SYSTEM_DIAGNOSTICS_SRCS) $(HARDWARE_ABSTRACTION_SRCS) $(QNX_DRIVER_FRAMEWORK_SRCS) $(BSP_SRCS) $(RUNTIME_SRCS) $(DEPLOYMENT_SRCS) $(SIMULATOR_CORE_SRCS) $(HARDWARE_SRCS) $(PHASE3_SRCS) $(PHASE4_SRCS)

ALL_OBJS = $(patsubst src/%.cpp,$(OBJ_DIR)/%.o,$(ALL_SRCS))
QNX_NET_OBJS = $(patsubst network/%.cpp,$(OBJ_DIR)/qnx_network/%.o,$(QNX_NET_SRCS))
COORD_P2_OBJS = $(OBJ_DIR)/common/utils/TimeUtils.o \
                $(OBJ_DIR)/common/utils/ThreadUtils.o \
                $(OBJ_DIR)/common/protocol/Packet.o \
                $(OBJ_DIR)/common/config/ServerConfig.o \
                $(OBJ_DIR)/common/network/TcpSocket.o \
                $(OBJ_DIR)/coordinator/logger/CoordinatorLogger.o \
                $(OBJ_DIR)/coordinator/server/ClientSession.o \
                $(OBJ_DIR)/coordinator/server/CoordinatorServer.o

# Phase 3 Traffic Signal Node Sources
TRAFFIC_NODE_P3_SRCS = traffic_node/config/TrafficNodeConfig.cpp \
                       traffic_node/logger/TrafficLogger.cpp \
                       traffic_node/gpio/MockGpioDriver.cpp \
                       traffic_node/gpio/Rpi4GpioDriver.cpp \
                       traffic_node/relay_controller/RelayController.cpp \
                       traffic_node/lamp_controller/LampController.cpp \
                       traffic_node/signal_controller/TrafficSignalFSM.cpp \
                       traffic_node/signal_controller/SignalController.cpp \
                       traffic_node/watchdog/NodeWatchdog.cpp \
                       traffic_node/client/ConnectionManager.cpp \
                       traffic_node/client/HeartbeatManager.cpp \
                       traffic_node/client/TrafficClient.cpp \
                       traffic_node/protocol/TrafficCommandDispatcher.cpp

TRAFFIC_NODE_P3_OBJS = $(OBJ_DIR)/traffic_node/config/TrafficNodeConfig.o \
                       $(OBJ_DIR)/traffic_node/logger/TrafficLogger.o \
                       $(OBJ_DIR)/traffic_node/gpio/MockGpioDriver.o \
                       $(OBJ_DIR)/traffic_node/gpio/Rpi4GpioDriver.o \
                       $(OBJ_DIR)/traffic_node/relay_controller/RelayController.o \
                       $(OBJ_DIR)/traffic_node/lamp_controller/LampController.o \
                       $(OBJ_DIR)/traffic_node/signal_controller/TrafficSignalFSM.o \
                       $(OBJ_DIR)/traffic_node/signal_controller/SignalController.o \
                       $(OBJ_DIR)/traffic_node/watchdog/NodeWatchdog.o \
                       $(OBJ_DIR)/traffic_node/client/ConnectionManager.o \
                       $(OBJ_DIR)/traffic_node/client/HeartbeatManager.o \
                       $(OBJ_DIR)/traffic_node/client/TrafficClient.o \
                       $(OBJ_DIR)/traffic_node/protocol/TrafficCommandDispatcher.o

# Phase 4 Coordinator Intelligence Sources
COORD_INTEL_SRCS = coordinator/intelligence/graph/RoadGraph.cpp \
                   coordinator/intelligence/planner/DijkstraRoutePlanner.cpp \
                   coordinator/intelligence/eta/ContinuousETAEngine.cpp \
                   coordinator/intelligence/scheduler/PriorityScheduler.cpp \
                   coordinator/intelligence/corridor/GreenCorridorPlanner.cpp \
                   coordinator/intelligence/execution/ExecutionPlanGenerator.cpp \
                   coordinator/intelligence/safety/SafetyPolicyEngine.cpp \
                   coordinator/intelligence/config/IntelligenceConfig.cpp

COORD_INTEL_OBJS = $(OBJ_DIR)/coordinator/intelligence/graph/RoadGraph.o \
                   $(OBJ_DIR)/coordinator/intelligence/planner/DijkstraRoutePlanner.o \
                   $(OBJ_DIR)/coordinator/intelligence/eta/ContinuousETAEngine.o \
                   $(OBJ_DIR)/coordinator/intelligence/scheduler/PriorityScheduler.o \
                   $(OBJ_DIR)/coordinator/intelligence/corridor/GreenCorridorPlanner.o \
                   $(OBJ_DIR)/coordinator/intelligence/execution/ExecutionPlanGenerator.o \
                   $(OBJ_DIR)/coordinator/intelligence/safety/SafetyPolicyEngine.o \
                   $(OBJ_DIR)/coordinator/intelligence/config/IntelligenceConfig.o

# Phase 5 Coordinator Orchestration Sources
COORD_ORCHESTRATION_SRCS = coordinator/orchestration/state/CoordinatorStateManager.cpp \
                          coordinator/orchestration/event_bus/EventBus.cpp \
                          coordinator/orchestration/session/EmergencySessionManager.cpp \
                          coordinator/orchestration/fsm/CoordinatorFSM.cpp \
                          coordinator/orchestration/dispatcher/ExecutionDispatcher.cpp \
                          coordinator/orchestration/sync/NodeClockTracker.cpp \
                          coordinator/orchestration/monitor/NodeHealthMonitor.cpp \
                          coordinator/orchestration/monitor/ExecutionMonitor.cpp \
                          coordinator/orchestration/recovery/RecoveryManager.cpp \
                          coordinator/orchestration/config/OrchestrationConfig.cpp

COORD_ORCHESTRATION_OBJS = $(OBJ_DIR)/coordinator/orchestration/state/CoordinatorStateManager.o \
                          $(OBJ_DIR)/coordinator/orchestration/event_bus/EventBus.o \
                          $(OBJ_DIR)/coordinator/orchestration/session/EmergencySessionManager.o \
                          $(OBJ_DIR)/coordinator/orchestration/fsm/CoordinatorFSM.o \
                          $(OBJ_DIR)/coordinator/orchestration/dispatcher/ExecutionDispatcher.o \
                          $(OBJ_DIR)/coordinator/orchestration/sync/NodeClockTracker.o \
                          $(OBJ_DIR)/coordinator/orchestration/monitor/NodeHealthMonitor.o \
                          $(OBJ_DIR)/coordinator/orchestration/monitor/ExecutionMonitor.o \
                          $(OBJ_DIR)/coordinator/orchestration/recovery/RecoveryManager.o \
                          $(OBJ_DIR)/coordinator/orchestration/config/OrchestrationConfig.o

# Phase 6 Control Center & Hospital Coordination Sources
COORD_PHASE6_SRCS = coordinator/control_center/EmergencyValidator.cpp \
                    coordinator/control_center/EmergencyPriorityEngine.cpp \
                    coordinator/control_center/IncidentManager.cpp \
                    coordinator/control_center/ResourceAllocator.cpp \
                    coordinator/control_center/DispatcherConsole.cpp \
                    coordinator/control_center/EmergencyControlCenter.cpp \
                    coordinator/hospital/HospitalRegistry.cpp \
                    coordinator/hospital/HospitalSelectionEngine.cpp \
                    coordinator/hospital/BedAvailabilityManager.cpp \
                    coordinator/hospital/HospitalNotificationManager.cpp \
                    coordinator/hospital/DoctorPreparationManager.cpp \
                    coordinator/hospital/PatientHandoverManager.cpp

COORD_PHASE6_OBJS = $(OBJ_DIR)/coordinator/control_center/EmergencyValidator.o \
                    $(OBJ_DIR)/coordinator/control_center/EmergencyPriorityEngine.o \
                    $(OBJ_DIR)/coordinator/control_center/IncidentManager.o \
                    $(OBJ_DIR)/coordinator/control_center/ResourceAllocator.o \
                    $(OBJ_DIR)/coordinator/control_center/DispatcherConsole.o \
                    $(OBJ_DIR)/coordinator/control_center/EmergencyControlCenter.o \
                    $(OBJ_DIR)/coordinator/hospital/HospitalRegistry.o \
                    $(OBJ_DIR)/coordinator/hospital/HospitalSelectionEngine.o \
                    $(OBJ_DIR)/coordinator/hospital/BedAvailabilityManager.o \
                    $(OBJ_DIR)/coordinator/hospital/HospitalNotificationManager.o \
                    $(OBJ_DIR)/coordinator/hospital/DoctorPreparationManager.o \
                    $(OBJ_DIR)/coordinator/hospital/PatientHandoverManager.o

# Phase 7 Intelligent Road Situation Awareness (IRSA) Sources
COORD_PHASE7_SRCS = coordinator/perception/RoadConditionEvent.cpp \
                    coordinator/perception/CameraRegistry.cpp \
                    coordinator/perception/CameraFeedManager.cpp \
                    coordinator/perception/VideoStreamManager.cpp \
                    coordinator/perception/RoadConfidenceEngine.cpp \
                    coordinator/perception/RoadEventPublisher.cpp \
                    coordinator/perception/RoadSituationAwarenessEngine.cpp \
                    coordinator/perception/CoordinatorIntegration.cpp \
                    coordinator/perception/analytics/TrafficDensityAnalyzer.cpp \
                    coordinator/perception/analytics/QueueLengthEstimator.cpp \
                    coordinator/perception/analytics/LaneOccupancyEstimator.cpp \
                    coordinator/perception/analytics/RoadHealthAnalyzer.cpp \
                    coordinator/perception/analytics/IncidentDetectionEngine.cpp \
                    coordinator/perception/analytics/EmergencyVehicleDetectionEngine.cpp \
                    coordinator/perception/analytics/RoadBlockageAnalyzer.cpp \
                    coordinator/perception/analytics/CongestionAnalyzer.cpp \
                    coordinator/perception/analytics/TrafficPredictionEngine.cpp \
                    coordinator/perception/analytics/CameraHealthMonitor.cpp \
                    coordinator/perception/analytics/RoadSuitabilityCalculator.cpp \
                    coordinator/perception/analytics/HistoricalTrafficAnalytics.cpp \
                    coordinator/perception/digital_twin/PerceptionDigitalTwinBridge.cpp

COORD_PHASE7_OBJS = $(OBJ_DIR)/coordinator/perception/RoadConditionEvent.o \
                    $(OBJ_DIR)/coordinator/perception/CameraRegistry.o \
                    $(OBJ_DIR)/coordinator/perception/CameraFeedManager.o \
                    $(OBJ_DIR)/coordinator/perception/VideoStreamManager.o \
                    $(OBJ_DIR)/coordinator/perception/RoadConfidenceEngine.o \
                    $(OBJ_DIR)/coordinator/perception/RoadEventPublisher.o \
                    $(OBJ_DIR)/coordinator/perception/RoadSituationAwarenessEngine.o \
                    $(OBJ_DIR)/coordinator/perception/CoordinatorIntegration.o \
                    $(OBJ_DIR)/coordinator/perception/analytics/TrafficDensityAnalyzer.o \
                    $(OBJ_DIR)/coordinator/perception/analytics/QueueLengthEstimator.o \
                    $(OBJ_DIR)/coordinator/perception/analytics/LaneOccupancyEstimator.o \
                    $(OBJ_DIR)/coordinator/perception/analytics/RoadHealthAnalyzer.o \
                    $(OBJ_DIR)/coordinator/perception/analytics/IncidentDetectionEngine.o \
                    $(OBJ_DIR)/coordinator/perception/analytics/EmergencyVehicleDetectionEngine.o \
                    $(OBJ_DIR)/coordinator/perception/analytics/RoadBlockageAnalyzer.o \
                    $(OBJ_DIR)/coordinator/perception/analytics/CongestionAnalyzer.o \
                    $(OBJ_DIR)/coordinator/perception/analytics/TrafficPredictionEngine.o \
                    $(OBJ_DIR)/coordinator/perception/analytics/CameraHealthMonitor.o \
                    $(OBJ_DIR)/coordinator/perception/analytics/RoadSuitabilityCalculator.o \
                    $(OBJ_DIR)/coordinator/perception/analytics/HistoricalTrafficAnalytics.o \
                    $(OBJ_DIR)/coordinator/perception/digital_twin/PerceptionDigitalTwinBridge.o

# Phase 8 Integrated Digital Twin & Operations Center Sources
PHASE8_DT_SRCS = digital_twin/CityModel.cpp \
                 digital_twin/SimulationClock.cpp \
                 digital_twin/TelemetryCollector.cpp \
                 digital_twin/DashboardEventBridge.cpp \
                 digital_twin/MissionTimeline.cpp \
                 digital_twin/MissionReplayEngine.cpp \
                 digital_twin/ScenarioPlaybackController.cpp \
                 digital_twin/SimulationScenarioManager.cpp \
                 digital_twin/visualizers/AmbulanceVisualizer.cpp \
                 digital_twin/visualizers/TrafficSignalVisualizer.cpp \
                 digital_twin/visualizers/CorridorVisualizer.cpp \
                 digital_twin/visualizers/CameraVisualizer.cpp \
                 digital_twin/visualizers/HospitalVisualizer.cpp \
                 digital_twin/visualizers/CoordinatorVisualizer.cpp \
                 digital_twin/PerformanceMonitor.cpp \
                 digital_twin/StatisticsEngine.cpp \
                 digital_twin/BenchmarkRecorder.cpp \
                 digital_twin/ReportExporter.cpp \
                 digital_twin/DashboardController.cpp \
                 digital_twin/DigitalTwinEngine.cpp

PHASE8_DT_OBJS = $(OBJ_DIR)/digital_twin/CityModel.o \
                 $(OBJ_DIR)/digital_twin/SimulationClock.o \
                 $(OBJ_DIR)/digital_twin/TelemetryCollector.o \
                 $(OBJ_DIR)/digital_twin/DashboardEventBridge.o \
                 $(OBJ_DIR)/digital_twin/MissionTimeline.o \
                 $(OBJ_DIR)/digital_twin/MissionReplayEngine.o \
                 $(OBJ_DIR)/digital_twin/ScenarioPlaybackController.o \
                 $(OBJ_DIR)/digital_twin/SimulationScenarioManager.o \
                 $(OBJ_DIR)/digital_twin/visualizers/AmbulanceVisualizer.o \
                 $(OBJ_DIR)/digital_twin/visualizers/TrafficSignalVisualizer.o \
                 $(OBJ_DIR)/digital_twin/visualizers/CorridorVisualizer.o \
                 $(OBJ_DIR)/digital_twin/visualizers/CameraVisualizer.o \
                 $(OBJ_DIR)/digital_twin/visualizers/HospitalVisualizer.o \
                 $(OBJ_DIR)/digital_twin/visualizers/CoordinatorVisualizer.o \
                 $(OBJ_DIR)/digital_twin/PerformanceMonitor.o \
                 $(OBJ_DIR)/digital_twin/StatisticsEngine.o \
                 $(OBJ_DIR)/digital_twin/BenchmarkRecorder.o \
                 $(OBJ_DIR)/digital_twin/ReportExporter.o \
                 $(OBJ_DIR)/digital_twin/DashboardController.o \
                 $(OBJ_DIR)/digital_twin/DigitalTwinEngine.o

# Phase 9 Production Readiness, Reliability, Safety & Deployment Sources
PHASE9_PROD_SRCS = production/health/HealthSupervisor.cpp \
                   production/health/WatchdogManager.cpp \
                   production/health/RuntimeDiagnostics.cpp \
                   production/health/HeartbeatSupervisor.cpp \
                   production/fault_tolerance/NodeRecoveryManager.cpp \
                   production/fault_tolerance/MissionContinuityManager.cpp \
                   production/fault_tolerance/LifecycleManager.cpp \
                   production/communication/ReliableChannel.cpp \
                   production/communication/PacketValidator.cpp \
                   production/security/NodeAuthenticator.cpp \
                   production/security/MessageSecurityGuard.cpp \
                   production/security/SecurityAuditLogger.cpp \
                   production/config/ProductionConfigManager.cpp \
                   production/config/VersionManager.cpp \
                   production/reliability/DeadlockDetector.cpp \
                   production/reliability/ResourceLeakMonitor.cpp \
                   production/reliability/SoakTestRunner.cpp \
                   production/fault_injection/FaultInjector.cpp \
                   production/benchmark/ProductionBenchmarkReporter.cpp \
                   production/ProductionEngine.cpp

PHASE9_PROD_OBJS = $(OBJ_DIR)/production/health/HealthSupervisor.o \
                   $(OBJ_DIR)/production/health/WatchdogManager.o \
                   $(OBJ_DIR)/production/health/RuntimeDiagnostics.o \
                   $(OBJ_DIR)/production/health/HeartbeatSupervisor.o \
                   $(OBJ_DIR)/production/fault_tolerance/NodeRecoveryManager.o \
                   $(OBJ_DIR)/production/fault_tolerance/MissionContinuityManager.o \
                   $(OBJ_DIR)/production/fault_tolerance/LifecycleManager.o \
                   $(OBJ_DIR)/production/communication/ReliableChannel.o \
                   $(OBJ_DIR)/production/communication/PacketValidator.o \
                   $(OBJ_DIR)/production/security/NodeAuthenticator.o \
                   $(OBJ_DIR)/production/security/MessageSecurityGuard.o \
                   $(OBJ_DIR)/production/security/SecurityAuditLogger.o \
                   $(OBJ_DIR)/production/config/ProductionConfigManager.o \
                   $(OBJ_DIR)/production/config/VersionManager.o \
                   $(OBJ_DIR)/production/reliability/DeadlockDetector.o \
                   $(OBJ_DIR)/production/reliability/ResourceLeakMonitor.o \
                   $(OBJ_DIR)/production/reliability/SoakTestRunner.o \
                   $(OBJ_DIR)/production/fault_injection/FaultInjector.o \
                   $(OBJ_DIR)/production/benchmark/ProductionBenchmarkReporter.o \
                   $(OBJ_DIR)/production/ProductionEngine.o

# Phase 11 Raspberry Pi Cluster Deployment & Hardware Integration Sources
HARDWARE_INTEGRATION_SRCS = hardware_integration/gpio/SymbolicGpioHAL.cpp \
                            hardware_integration/relay/ProductionRelayDriver.cpp \
                            hardware_integration/sensor/IRSensorDriver.cpp \
                            hardware_integration/config/NodeConfigManager.cpp \
                            hardware_integration/commissioning/CommissioningManager.cpp \
                            hardware_integration/cluster/ClusterSupervisor.cpp \
                            hardware_integration/validation/DeploymentValidator.cpp \
                            hardware_integration/logging/CentralizedDeploymentLogger.cpp

HARDWARE_INTEGRATION_OBJS = $(OBJ_DIR)/hardware_integration/gpio/SymbolicGpioHAL.o \
                            $(OBJ_DIR)/hardware_integration/relay/ProductionRelayDriver.o \
                            $(OBJ_DIR)/hardware_integration/sensor/IRSensorDriver.o \
                            $(OBJ_DIR)/hardware_integration/config/NodeConfigManager.o \
                            $(OBJ_DIR)/hardware_integration/commissioning/CommissioningManager.o \
                            $(OBJ_DIR)/hardware_integration/cluster/ClusterSupervisor.o \
                            $(OBJ_DIR)/hardware_integration/validation/DeploymentValidator.o \
                            $(OBJ_DIR)/hardware_integration/logging/CentralizedDeploymentLogger.o

# Phase 12 Hardware Demonstration Sources
HARDWARE_DEMO_SRCS = hardware_integration/demo/BuzzerController.cpp \
                     hardware_integration/demo/EmergencySwitchDriver.cpp \
                     hardware_integration/demo/PoliceOverrideSwitchDriver.cpp \
                     hardware_integration/demo/OledV2XDisplay.cpp \
                     hardware_integration/demo/Intersection2PriorityDemonstrator.cpp \
                     hardware_integration/demo/PoliceReplanningCoordinator.cpp \
                     hardware_integration/demo/HardwareDemoOrchestrator.cpp

HARDWARE_DEMO_OBJS = $(OBJ_DIR)/hardware_integration/demo/BuzzerController.o \
                     $(OBJ_DIR)/hardware_integration/demo/EmergencySwitchDriver.o \
                     $(OBJ_DIR)/hardware_integration/demo/PoliceOverrideSwitchDriver.o \
                     $(OBJ_DIR)/hardware_integration/demo/OledV2XDisplay.o \
                     $(OBJ_DIR)/hardware_integration/demo/Intersection2PriorityDemonstrator.o \
                     $(OBJ_DIR)/hardware_integration/demo/PoliceReplanningCoordinator.o \
                     $(OBJ_DIR)/hardware_integration/demo/HardwareDemoOrchestrator.o

# Phase 13 Operations, Monitoring & Command Center Sources
COMMAND_CENTER_SRCS = command_center/fleet/EmsFleetManager.cpp \
                      command_center/tracking/LiveAmbulanceTracker.cpp \
                      command_center/mission/MissionDatabase.cpp \
                      command_center/hospital/HospitalOperationsDashboard.cpp \
                      command_center/traffic/TrafficOperationsDashboard.cpp \
                      command_center/cluster/ClusterMonitor.cpp \
                      command_center/health/SystemHealthMonitor.cpp \
                      command_center/analytics/RealTimeAnalyticsEngine.cpp \
                      command_center/replay/MissionReplayController.cpp \
                      command_center/dashboard/CommandCenterConsole.cpp \
                      command_center/bridge/CommandCenterEventBridge.cpp \
                      command_center/CommandCenterFacade.cpp

COMMAND_CENTER_OBJS = $(OBJ_DIR)/command_center/fleet/EmsFleetManager.o \
                      $(OBJ_DIR)/command_center/tracking/LiveAmbulanceTracker.o \
                      $(OBJ_DIR)/command_center/mission/MissionDatabase.o \
                      $(OBJ_DIR)/command_center/hospital/HospitalOperationsDashboard.o \
                      $(OBJ_DIR)/command_center/traffic/TrafficOperationsDashboard.o \
                      $(OBJ_DIR)/command_center/cluster/ClusterMonitor.o \
                      $(OBJ_DIR)/command_center/health/SystemHealthMonitor.o \
                      $(OBJ_DIR)/command_center/analytics/RealTimeAnalyticsEngine.o \
                      $(OBJ_DIR)/command_center/replay/MissionReplayController.o \
                      $(OBJ_DIR)/command_center/dashboard/CommandCenterConsole.o \
                      $(OBJ_DIR)/command_center/bridge/CommandCenterEventBridge.o \
                      $(OBJ_DIR)/command_center/CommandCenterFacade.o

STATIC_LIB = $(LIB_DIR)/libaegis.a

# App Executables & Tests
APPS = $(BIN_DIR)/aegis_coordinator $(BIN_DIR)/aegis_ambulance $(BIN_DIR)/aegis_traffic_controller $(BIN_DIR)/aegis_intersection_node
SMOKE_TEST_BIN = $(BIN_DIR)/test_build_smoke
FOUNDATION_TEST_BIN = $(BIN_DIR)/test_common_foundation
COMM_TEST_BIN = $(BIN_DIR)/test_communication_infrastructure
COORD_TEST_BIN = $(BIN_DIR)/test_coordinator_core
COORD_FOUNDATION_TEST_BIN = $(BIN_DIR)/test_coordinator_foundation
GREEN_CORRIDOR_TEST_BIN = $(BIN_DIR)/test_green_corridor
AMB_TEST_BIN = $(BIN_DIR)/test_ambulance_node
TRAFFIC_TEST_BIN = $(BIN_DIR)/test_traffic_signal_node
COMM_MOD_TEST_BIN = $(BIN_DIR)/test_communication_module
INTERSECTION_NODE_TEST_BIN = $(BIN_DIR)/test_intersection_node
SYSTEM_INTEGRATION_TEST_BIN = $(BIN_DIR)/test_system_integration
RPI_DEPLOYMENT_TEST_BIN = $(BIN_DIR)/test_rpi_deployment
MEDICAL_TEST_BIN = $(BIN_DIR)/test_medical_priority
AMB_REG_TEST_BIN = $(BIN_DIR)/test_ambulance_registry
CONFLICT_TEST_BIN = $(BIN_DIR)/test_conflict_detection
CONTROL_TEST_BIN = $(BIN_DIR)/test_manual_override
LOGGER_TEST_BIN = $(BIN_DIR)/test_event_logger
PHASE8_TEST_BIN = $(BIN_DIR)/test_phase8_intelligent_scheduler
COORD_MONITORING_TEST_BIN = $(BIN_DIR)/test_coordinator_monitoring
TRAFFIC_SIGNAL_PHASE6A_TEST_BIN = $(BIN_DIR)/test_traffic_signal_node_phase6a
COORD_TRAFFIC_SIGNAL_PHASE6B_TEST_BIN = $(BIN_DIR)/test_coordinator_traffic_signal_phase6b
EMERGENCY_CORRIDOR_PHASE7A_TEST_BIN = $(BIN_DIR)/test_emergency_corridor_planner_phase7a
GREEN_WAVE_PHASE7B_TEST_BIN = $(BIN_DIR)/test_green_wave_coordinator_phase7b
PRIORITY_SCHEDULER_PHASE8A_TEST_BIN = $(BIN_DIR)/test_priority_scheduler_phase8a
EMERGENCY_ORCHESTRATOR_PHASE8B_TEST_BIN = $(BIN_DIR)/test_emergency_orchestrator_phase8b
DYNAMIC_ROUTE_REPLANNER_PHASE9A_TEST_BIN = $(BIN_DIR)/test_dynamic_route_replanner_phase9a
MULTI_CORRIDOR_COORDINATOR_PHASE9B_TEST_BIN = $(BIN_DIR)/test_multi_corridor_coordinator_phase9b
DISTRIBUTED_SIGNAL_PHASE10A_TEST_BIN = $(BIN_DIR)/test_distributed_signal_phase10a
PREDICTIVE_TRAFFIC_PHASE10B_TEST_BIN = $(BIN_DIR)/test_predictive_traffic_phase10b
FLEET_MANAGEMENT_PHASE11A_TEST_BIN = $(BIN_DIR)/test_fleet_management_phase11a
HOSPITAL_SELECTION_PHASE11B_TEST_BIN = $(BIN_DIR)/test_hospital_selection_phase11b
HOSPITAL_RESOURCE_PHASE11B_TEST_BIN = $(BIN_DIR)/test_hospital_resource_phase11b
MISSION_EXECUTION_PHASE12A_TEST_BIN = $(BIN_DIR)/test_mission_execution_phase12a
INCIDENT_COMMAND_PHASE12B_TEST_BIN = $(BIN_DIR)/test_incident_command_phase12b
TRAFFIC_NETWORK_ORCHESTRATOR_PHASE13A_TEST_BIN = $(BIN_DIR)/test_traffic_network_orchestrator_phase13a
DISTRIBUTED_CONSENSUS_PHASE13B_TEST_BIN = $(BIN_DIR)/test_distributed_consensus_phase13b
RPI_CLUSTER_PHASE14A_TEST_BIN = $(BIN_DIR)/test_rpi_cluster_phase14a
SYSTEM_DIAGNOSTICS_PHASE14B_TEST_BIN = $(BIN_DIR)/test_system_diagnostics_phase14b
HARDWARE_ABSTRACTION_PHASE15A_TEST_BIN = $(BIN_DIR)/test_hardware_abstraction_phase15a
QNX_DRIVER_FRAMEWORK_PHASE15B_TEST_BIN = $(BIN_DIR)/test_qnx_driver_framework_phase15b
BSP_PHASE16A_TEST_BIN = $(BIN_DIR)/test_raspberry_pi4_bsp_phase16a
QNX_NETWORK_TEST_BIN = $(BIN_DIR)/test_qnx_network_layer
DISTRIBUTED_RUNTIME_PHASE16C_TEST_BIN = $(BIN_DIR)/test_distributed_runtime_phase16c
DEPLOYMENT_LAYER_PHASE16D_TEST_BIN = $(BIN_DIR)/test_deployment_layer_phase16d
CITY_SIMULATION_PHASE19_TEST_BIN = $(BIN_DIR)/test_city_simulation_phase19
HARDWARE_INTEGRATION_PHASE20_TEST_BIN = $(BIN_DIR)/test_hardware_integration_phase20
QNX_DEPLOYMENT_PHASE20_5_TEST_BIN = $(BIN_DIR)/test_qnx_deployment_phase20_5
QNX_HARDENING_PHASE20_6_TEST_BIN = $(BIN_DIR)/test_qnx_hardening_phase20_6
QNX_HARDENING_PHASE20_7_TEST_BIN = $(BIN_DIR)/test_qnx_hardening_phase20_7
PHASE1_HARDWARE_TEST_BIN = $(BIN_DIR)/test_phase1_real_qnx_hardware
PHASE2_EMBEDDED_TEST_BIN = $(BIN_DIR)/test_phase2_embedded_production
PHASE3_CORRIDOR_TEST_BIN = $(BIN_DIR)/test_phase3_corridor_operations
PHASE4_QNX_TEST_BIN = $(BIN_DIR)/test_phase4_native_qnx
COORDINATOR_SERVER_BIN = $(BIN_DIR)/coordinator_tcp_server
COORDINATOR_TCP_TEST_BIN = $(BIN_DIR)/test_coordinator_tcp_server
TRAFFIC_SIGNAL_NODE_BIN = $(BIN_DIR)/traffic_signal_node
TRAFFIC_NODE_TEST_BIN = $(BIN_DIR)/test_traffic_node
COORD_INTEL_TEST_BIN = $(BIN_DIR)/test_coordinator_intelligence
COORD_ORCHESTRATION_TEST_BIN = $(BIN_DIR)/test_coordinator_orchestration
CONTROL_CENTER_TEST_BIN = $(BIN_DIR)/test_control_center
ROAD_PERCEPTION_TEST_BIN = $(BIN_DIR)/test_road_perception
DIGITAL_TWIN_TEST_BIN = $(BIN_DIR)/test_digital_twin_operations_center
PRODUCTION_RELIABILITY_TEST_BIN = $(BIN_DIR)/test_production_reliability
SYSTEM_VVE_TEST_BIN = $(BIN_DIR)/test_system_vve
CLUSTER_DEPLOYMENT_TEST_BIN = $(BIN_DIR)/test_cluster_deployment_hardware
PHASE12_DEMO_TEST_BIN = $(BIN_DIR)/test_phase12_hardware_demo
PHASE13_COMMAND_CENTER_TEST_BIN = $(BIN_DIR)/test_phase13_command_center
EXAMPLE_SERVER_BIN = $(BIN_DIR)/example_server
EXAMPLE_CLIENT_BIN = $(BIN_DIR)/example_client

.PHONY: all clean test directories info

all: directories $(STATIC_LIB) $(APPS) $(SMOKE_TEST_BIN) $(FOUNDATION_TEST_BIN) $(COMM_TEST_BIN) $(COMM_MOD_TEST_BIN) $(COORD_TEST_BIN) $(COORD_FOUNDATION_TEST_BIN) $(GREEN_CORRIDOR_TEST_BIN) $(AMB_TEST_BIN) $(TRAFFIC_TEST_BIN) $(INTERSECTION_NODE_TEST_BIN) $(SYSTEM_INTEGRATION_TEST_BIN) $(RPI_DEPLOYMENT_TEST_BIN) $(MEDICAL_TEST_BIN) $(AMB_REG_TEST_BIN) $(CONFLICT_TEST_BIN) $(CONTROL_TEST_BIN) $(LOGGER_TEST_BIN) $(PHASE8_TEST_BIN) $(COORD_MONITORING_TEST_BIN) $(TRAFFIC_SIGNAL_PHASE6A_TEST_BIN) $(COORD_TRAFFIC_SIGNAL_PHASE6B_TEST_BIN) $(EMERGENCY_CORRIDOR_PHASE7A_TEST_BIN) $(GREEN_WAVE_PHASE7B_TEST_BIN) $(PRIORITY_SCHEDULER_PHASE8A_TEST_BIN) $(EMERGENCY_ORCHESTRATOR_PHASE8B_TEST_BIN) $(DYNAMIC_ROUTE_REPLANNER_PHASE9A_TEST_BIN) $(MULTI_CORRIDOR_COORDINATOR_PHASE9B_TEST_BIN) $(DISTRIBUTED_SIGNAL_PHASE10A_TEST_BIN) $(PREDICTIVE_TRAFFIC_PHASE10B_TEST_BIN) $(FLEET_MANAGEMENT_PHASE11A_TEST_BIN) $(HOSPITAL_SELECTION_PHASE11B_TEST_BIN) $(HOSPITAL_RESOURCE_PHASE11B_TEST_BIN) $(MISSION_EXECUTION_PHASE12A_TEST_BIN) $(INCIDENT_COMMAND_PHASE12B_TEST_BIN) $(TRAFFIC_NETWORK_ORCHESTRATOR_PHASE13A_TEST_BIN) $(DISTRIBUTED_CONSENSUS_PHASE13B_TEST_BIN) $(RPI_CLUSTER_PHASE14A_TEST_BIN) $(SYSTEM_DIAGNOSTICS_PHASE14B_TEST_BIN) $(HARDWARE_ABSTRACTION_PHASE15A_TEST_BIN) $(QNX_DRIVER_FRAMEWORK_PHASE15B_TEST_BIN) $(BSP_PHASE16A_TEST_BIN) $(QNX_NETWORK_TEST_BIN) $(DISTRIBUTED_RUNTIME_PHASE16C_TEST_BIN) $(DEPLOYMENT_LAYER_PHASE16D_TEST_BIN) $(CITY_SIMULATION_PHASE19_TEST_BIN) $(HARDWARE_INTEGRATION_PHASE20_TEST_BIN) $(QNX_DEPLOYMENT_PHASE20_5_TEST_BIN) $(QNX_HARDENING_PHASE20_6_TEST_BIN) $(QNX_HARDENING_PHASE20_7_TEST_BIN) $(PHASE1_HARDWARE_TEST_BIN) $(PHASE2_EMBEDDED_TEST_BIN) $(PHASE3_CORRIDOR_TEST_BIN) $(PHASE4_QNX_TEST_BIN) $(COORDINATOR_SERVER_BIN) $(COORDINATOR_TCP_TEST_BIN) $(TRAFFIC_SIGNAL_NODE_BIN) $(TRAFFIC_NODE_TEST_BIN) $(COORD_INTEL_TEST_BIN) $(COORD_ORCHESTRATION_TEST_BIN) $(CONTROL_CENTER_TEST_BIN) $(ROAD_PERCEPTION_TEST_BIN) $(DIGITAL_TWIN_TEST_BIN) $(PRODUCTION_RELIABILITY_TEST_BIN) $(SYSTEM_VVE_TEST_BIN) $(CLUSTER_DEPLOYMENT_TEST_BIN) $(PHASE12_DEMO_TEST_BIN) $(PHASE13_COMMAND_CENTER_TEST_BIN) $(EXAMPLE_SERVER_BIN) $(EXAMPLE_CLIENT_BIN)
	@echo "=========================================================="
	@echo " SMART AEGIS Build Complete [$(BUILD_TYPE)]"
	@echo " Binaries located in: $(BIN_DIR)"
	@echo "=========================================================="

directories:
	@mkdir -p $(BIN_DIR) $(LIB_DIR)
	@mkdir -p $(OBJ_DIR)/common $(OBJ_DIR)/config $(OBJ_DIR)/logger $(OBJ_DIR)/safety
	@mkdir -p $(OBJ_DIR)/network $(OBJ_DIR)/communication $(OBJ_DIR)/priority_scheduler $(OBJ_DIR)/route_planner
	@mkdir -p $(OBJ_DIR)/coordinator $(OBJ_DIR)/ambulance $(OBJ_DIR)/traffic_controller $(OBJ_DIR)/intersection $(OBJ_DIR)/system $(OBJ_DIR)/medical $(OBJ_DIR)/scheduler $(OBJ_DIR)/control $(OBJ_DIR)/traffic_signal $(OBJ_DIR)/green_corridor $(OBJ_DIR)/green_wave $(OBJ_DIR)/orchestrator $(OBJ_DIR)/replanning $(OBJ_DIR)/multi_corridor $(OBJ_DIR)/distributed_signal $(OBJ_DIR)/predictive_traffic $(OBJ_DIR)/fleet_management $(OBJ_DIR)/hospital_selection $(OBJ_DIR)/hospital_management $(OBJ_DIR)/mission_execution $(OBJ_DIR)/incident_command $(OBJ_DIR)/traffic_network_orchestrator $(OBJ_DIR)/distributed_consensus $(OBJ_DIR)/rpi_cluster $(OBJ_DIR)/system_diagnostics $(OBJ_DIR)/hardware_abstraction $(OBJ_DIR)/qnx_driver_framework $(OBJ_DIR)/bsp $(OBJ_DIR)/qnx_network $(OBJ_DIR)/runtime $(OBJ_DIR)/deployment $(OBJ_DIR)/simulator $(OBJ_DIR)/hardware $(OBJ_DIR)/corridor $(OBJ_DIR)/cluster $(OBJ_DIR)/recovery $(OBJ_DIR)/qnx
	@mkdir -p $(OBJ_DIR)/common/utils $(OBJ_DIR)/common/protocol $(OBJ_DIR)/common/config $(OBJ_DIR)/common/network
	@mkdir -p $(OBJ_DIR)/coordinator/server $(OBJ_DIR)/coordinator/logger $(OBJ_DIR)/coordinator/scheduler $(OBJ_DIR)/coordinator/routing $(OBJ_DIR)/coordinator/protocol
	@mkdir -p $(OBJ_DIR)/coordinator/orchestration/state $(OBJ_DIR)/coordinator/orchestration/event_bus $(OBJ_DIR)/coordinator/orchestration/session $(OBJ_DIR)/coordinator/orchestration/fsm $(OBJ_DIR)/coordinator/orchestration/dispatcher $(OBJ_DIR)/coordinator/orchestration/sync $(OBJ_DIR)/coordinator/orchestration/monitor $(OBJ_DIR)/coordinator/orchestration/recovery $(OBJ_DIR)/coordinator/orchestration/config
	@mkdir -p $(OBJ_DIR)/coordinator/control_center $(OBJ_DIR)/coordinator/hospital
	@mkdir -p $(OBJ_DIR)/coordinator/perception $(OBJ_DIR)/coordinator/perception/analytics $(OBJ_DIR)/coordinator/perception/digital_twin
	@mkdir -p $(OBJ_DIR)/digital_twin $(OBJ_DIR)/digital_twin/visualizers
	@mkdir -p $(OBJ_DIR)/production $(OBJ_DIR)/production/health $(OBJ_DIR)/production/fault_tolerance $(OBJ_DIR)/production/communication $(OBJ_DIR)/production/security $(OBJ_DIR)/production/config $(OBJ_DIR)/production/reliability $(OBJ_DIR)/production/fault_injection $(OBJ_DIR)/production/benchmark
	@mkdir -p $(OBJ_DIR)/traffic_node/config $(OBJ_DIR)/traffic_node/logger $(OBJ_DIR)/traffic_node/gpio $(OBJ_DIR)/traffic_node/relay_controller $(OBJ_DIR)/traffic_node/lamp_controller $(OBJ_DIR)/traffic_node/signal_controller $(OBJ_DIR)/traffic_node/watchdog $(OBJ_DIR)/traffic_node/client $(OBJ_DIR)/traffic_node/protocol
	@mkdir -p $(OBJ_DIR)/hardware_integration $(OBJ_DIR)/hardware_integration/gpio $(OBJ_DIR)/hardware_integration/relay $(OBJ_DIR)/hardware_integration/sensor $(OBJ_DIR)/hardware_integration/config $(OBJ_DIR)/hardware_integration/commissioning $(OBJ_DIR)/hardware_integration/cluster $(OBJ_DIR)/hardware_integration/validation $(OBJ_DIR)/hardware_integration/logging $(OBJ_DIR)/hardware_integration/demo
	@mkdir -p $(OBJ_DIR)/command_center $(OBJ_DIR)/command_center/fleet $(OBJ_DIR)/command_center/tracking $(OBJ_DIR)/command_center/mission $(OBJ_DIR)/command_center/hospital $(OBJ_DIR)/command_center/traffic $(OBJ_DIR)/command_center/cluster $(OBJ_DIR)/command_center/health $(OBJ_DIR)/command_center/analytics $(OBJ_DIR)/command_center/replay $(OBJ_DIR)/command_center/dashboard $(OBJ_DIR)/command_center/bridge

	@mkdir -p $(OBJ_DIR)/hospital_dashboard $(OBJ_DIR)/traffic_dashboard
	@mkdir -p $(OBJ_DIR)/apps $(OBJ_DIR)/tests/unit $(OBJ_DIR)/tests/integration

$(OBJ_DIR)/%.o: src/%.cpp | directories
	@echo "[CXX] $<"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/common/%.o: common/%.cpp | directories
	@mkdir -p $(dir $@)
	@echo "[CXX] $<"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/coordinator/%.o: coordinator/%.cpp | directories
	@mkdir -p $(dir $@)
	@echo "[CXX] $<"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/traffic_node/%.o: traffic_node/%.cpp | directories
	@mkdir -p $(dir $@)
	@echo "[CXX] $<"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/digital_twin/%.o: digital_twin/%.cpp | directories
	@mkdir -p $(dir $@)
	@echo "[CXX] $<"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/production/%.o: production/%.cpp | directories
	@mkdir -p $(dir $@)
	@echo "[CXX] $<"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/hardware_integration/%.o: hardware_integration/%.cpp | directories
	@mkdir -p $(dir $@)
	@echo "[CXX] $<"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/command_center/%.o: command_center/%.cpp | directories
	@mkdir -p $(dir $@)
	@echo "[CXX] $<"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/qnx_network/%.o: network/%.cpp | directories
	@echo "[CXX] $<"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(STATIC_LIB): $(ALL_OBJS) $(QNX_NET_OBJS) $(COORD_P2_OBJS) $(TRAFFIC_NODE_P3_OBJS) $(COORD_INTEL_OBJS) $(COORD_ORCHESTRATION_OBJS) $(COORD_PHASE6_OBJS) $(COORD_PHASE7_OBJS) $(PHASE8_DT_OBJS) $(PHASE9_PROD_OBJS) $(HARDWARE_INTEGRATION_OBJS) $(HARDWARE_DEMO_OBJS) $(COMMAND_CENTER_OBJS) | directories
	@echo "[AR] $@"
	@ar rcs $@ $(ALL_OBJS) $(QNX_NET_OBJS) $(COORD_P2_OBJS) $(TRAFFIC_NODE_P3_OBJS) $(COORD_INTEL_OBJS) $(COORD_ORCHESTRATION_OBJS) $(COORD_PHASE6_OBJS) $(COORD_PHASE7_OBJS) $(PHASE8_DT_OBJS) $(PHASE9_PROD_OBJS) $(HARDWARE_INTEGRATION_OBJS) $(HARDWARE_DEMO_OBJS) $(COMMAND_CENTER_OBJS)

$(BIN_DIR)/aegis_coordinator: src/apps/coordinator_main.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(BIN_DIR)/aegis_ambulance: src/apps/ambulance_main.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(BIN_DIR)/aegis_traffic_controller: src/apps/traffic_main.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(BIN_DIR)/aegis_intersection_node: src/apps/intersection_main.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(SMOKE_TEST_BIN): tests/unit/test_build_smoke.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(FOUNDATION_TEST_BIN): tests/unit/test_common_foundation.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(COMM_TEST_BIN): tests/unit/test_communication_infrastructure.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(COMM_MOD_TEST_BIN): tests/unit/test_communication_module.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(COORD_TEST_BIN): tests/unit/test_coordinator_core.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(COORD_FOUNDATION_TEST_BIN): tests/unit/test_coordinator_foundation.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(GREEN_CORRIDOR_TEST_BIN): tests/unit/test_green_corridor.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(AMB_TEST_BIN): tests/unit/test_ambulance_node.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(TRAFFIC_TEST_BIN): tests/unit/test_traffic_signal_node.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(INTERSECTION_NODE_TEST_BIN): tests/unit/test_intersection_node.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(SYSTEM_INTEGRATION_TEST_BIN): tests/integration/test_system_integration.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(RPI_DEPLOYMENT_TEST_BIN): tests/integration/test_rpi_deployment.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(MEDICAL_TEST_BIN): tests/unit/test_medical_priority.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(AMB_REG_TEST_BIN): tests/unit/test_ambulance_registry.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(CONFLICT_TEST_BIN): tests/unit/test_conflict_detection.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(CONTROL_TEST_BIN): tests/unit/test_manual_override.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(LOGGER_TEST_BIN): tests/unit/test_event_logger.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(PHASE8_TEST_BIN): tests/unit/test_phase8_intelligent_scheduler.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(COORD_MONITORING_TEST_BIN): tests/unit/test_coordinator_monitoring.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(TRAFFIC_SIGNAL_PHASE6A_TEST_BIN): tests/unit/test_traffic_signal_node_phase6a.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(COORD_TRAFFIC_SIGNAL_PHASE6B_TEST_BIN): tests/unit/test_coordinator_traffic_signal_phase6b.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(EMERGENCY_CORRIDOR_PHASE7A_TEST_BIN): tests/unit/test_emergency_corridor_planner_phase7a.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(GREEN_WAVE_PHASE7B_TEST_BIN): tests/unit/test_green_wave_coordinator_phase7b.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(PRIORITY_SCHEDULER_PHASE8A_TEST_BIN): tests/unit/test_priority_scheduler_phase8a.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(EMERGENCY_ORCHESTRATOR_PHASE8B_TEST_BIN): tests/unit/test_emergency_orchestrator_phase8b.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(DYNAMIC_ROUTE_REPLANNER_PHASE9A_TEST_BIN): tests/unit/test_dynamic_route_replanner_phase9a.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(MULTI_CORRIDOR_COORDINATOR_PHASE9B_TEST_BIN): tests/unit/test_multi_corridor_coordinator_phase9b.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(DISTRIBUTED_SIGNAL_PHASE10A_TEST_BIN): tests/unit/test_distributed_signal_phase10a.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(PREDICTIVE_TRAFFIC_PHASE10B_TEST_BIN): tests/unit/test_predictive_traffic_phase10b.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(FLEET_MANAGEMENT_PHASE11A_TEST_BIN): tests/unit/test_fleet_management_phase11a.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(HOSPITAL_SELECTION_PHASE11B_TEST_BIN): tests/unit/test_hospital_selection_phase11b.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(HOSPITAL_RESOURCE_PHASE11B_TEST_BIN): tests/unit/test_hospital_resource_phase11b.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(MISSION_EXECUTION_PHASE12A_TEST_BIN): tests/unit/test_mission_execution_phase12a.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(INCIDENT_COMMAND_PHASE12B_TEST_BIN): tests/unit/test_incident_command_phase12b.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(TRAFFIC_NETWORK_ORCHESTRATOR_PHASE13A_TEST_BIN): tests/unit/test_traffic_network_orchestrator_phase13a.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(DISTRIBUTED_CONSENSUS_PHASE13B_TEST_BIN): tests/unit/test_distributed_consensus_phase13b.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(RPI_CLUSTER_PHASE14A_TEST_BIN): tests/unit/test_rpi_cluster_phase14a.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(SYSTEM_DIAGNOSTICS_PHASE14B_TEST_BIN): tests/unit/test_system_diagnostics_phase14b.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(HARDWARE_ABSTRACTION_PHASE15A_TEST_BIN): tests/unit/test_hardware_abstraction_phase15a.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(QNX_DRIVER_FRAMEWORK_PHASE15B_TEST_BIN): tests/unit/test_qnx_driver_framework_phase15b.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(BSP_PHASE16A_TEST_BIN): tests/unit/test_raspberry_pi4_bsp_phase16a.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(QNX_NETWORK_TEST_BIN): tests/unit/test_qnx_network_layer.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(DISTRIBUTED_RUNTIME_PHASE16C_TEST_BIN): tests/unit/test_distributed_runtime_integration_phase16c.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(DEPLOYMENT_LAYER_PHASE16D_TEST_BIN): tests/unit/test_deployment_layer_phase16d.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(EXAMPLE_SERVER_BIN): network/example_server.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(EXAMPLE_CLIENT_BIN): network/example_client.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

# Phase 19 City Simulation Engine
QT_DIR ?= /opt/homebrew/opt/qtbase
QT_FLAGS ?= -iframework $(QT_DIR)/lib -F$(QT_DIR)/lib -isystem $(QT_DIR)/include -isystem $(QT_DIR)/include/QtCore -isystem $(QT_DIR)/include/QtWidgets -isystem $(QT_DIR)/include/QtGui
QT_LIBS ?= -F$(QT_DIR)/lib -framework QtCore -framework QtWidgets -framework QtGui
QT_MOC ?= /opt/homebrew/Cellar/qtbase/6.12.0_2/share/qt/libexec/moc
QT_MOC_INCLUDES = -Iinclude -Isrc -I$(QT_DIR)/include -I$(QT_DIR)/include/QtCore -I$(QT_DIR)/include/QtWidgets -I$(QT_DIR)/include/QtGui

build/test_moc/moc_DashboardDataModel.cpp: src/dashboard/DashboardDataModel.h | directories
	@mkdir -p build/test_moc
	@$(QT_MOC) $(QT_MOC_INCLUDES) $< -o $@

build/test_moc/moc_DashboardDataModel.o: build/test_moc/moc_DashboardDataModel.cpp
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(QT_FLAGS) -c $< -o $@

build/test_moc/DashboardDataModel.o: src/dashboard/DashboardDataModel.cpp | directories
	@mkdir -p build/test_moc
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(QT_FLAGS) -c $< -o $@

build/test_traffic_moc/moc_TrafficDashboardDataModel.cpp: src/dashboard/traffic/TrafficDashboardDataModel.h | directories
	@mkdir -p build/test_traffic_moc
	@$(QT_MOC) $(QT_MOC_INCLUDES) $< -o $@

build/test_traffic_moc/moc_TrafficDashboardDataModel.o: build/test_traffic_moc/moc_TrafficDashboardDataModel.cpp
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(QT_FLAGS) -c $< -o $@

build/test_traffic_moc/TrafficDashboardDataModel.o: src/dashboard/traffic/TrafficDashboardDataModel.cpp | directories
	@mkdir -p build/test_traffic_moc
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(QT_FLAGS) -c $< -o $@

$(OBJ_DIR)/simulator/DashboardAdapter.o: src/simulator/DashboardAdapter.cpp | directories
	@echo "[CXX] $<"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(QT_FLAGS) -c $< -o $@

$(OBJ_DIR)/simulator/SimulationEngine.o: src/simulator/SimulationEngine.cpp | directories
	@echo "[CXX] $<"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(QT_FLAGS) -c $< -o $@

$(OBJ_DIR)/tests/unit/test_city_simulation_phase19.o: tests/unit/test_city_simulation_phase19.cpp | directories
	@echo "[CXX] $<"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(QT_FLAGS) -c $< -o $@

$(CITY_SIMULATION_PHASE19_TEST_BIN): $(OBJ_DIR)/tests/unit/test_city_simulation_phase19.o $(OBJ_DIR)/simulator/DashboardAdapter.o $(OBJ_DIR)/simulator/SimulationEngine.o build/test_moc/DashboardDataModel.o build/test_moc/moc_DashboardDataModel.o build/test_traffic_moc/TrafficDashboardDataModel.o build/test_traffic_moc/moc_TrafficDashboardDataModel.o $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(OBJ_DIR)/tests/unit/test_city_simulation_phase19.o $(OBJ_DIR)/simulator/DashboardAdapter.o $(OBJ_DIR)/simulator/SimulationEngine.o build/test_moc/DashboardDataModel.o build/test_moc/moc_DashboardDataModel.o build/test_traffic_moc/TrafficDashboardDataModel.o build/test_traffic_moc/moc_TrafficDashboardDataModel.o -L$(LIB_DIR) -laegis $(QT_LIBS) -o $@

$(HARDWARE_INTEGRATION_PHASE20_TEST_BIN): tests/unit/test_hardware_integration_phase20.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(QNX_DEPLOYMENT_PHASE20_5_TEST_BIN): tests/unit/test_qnx_deployment_phase20_5.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(QNX_HARDENING_PHASE20_6_TEST_BIN): tests/unit/test_qnx_hardening_phase20_6.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(QNX_HARDENING_PHASE20_7_TEST_BIN): tests/unit/test_qnx_hardening_phase20_7.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(PHASE1_HARDWARE_TEST_BIN): tests/unit/test_phase1_real_qnx_hardware.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(PHASE2_EMBEDDED_TEST_BIN): tests/unit/test_phase2_embedded_production.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(PHASE3_CORRIDOR_TEST_BIN): tests/unit/test_phase3_corridor_operations.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(PHASE4_QNX_TEST_BIN): tests/unit/test_phase4_native_qnx.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(COORDINATOR_SERVER_BIN): coordinator/server/main.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(COORDINATOR_TCP_TEST_BIN): tests/unit/test_coordinator_tcp_server.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(TRAFFIC_SIGNAL_NODE_BIN): traffic_node/main.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(TRAFFIC_NODE_TEST_BIN): tests/unit/test_traffic_node.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(COORD_INTEL_TEST_BIN): tests/unit/test_coordinator_intelligence.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(COORD_ORCHESTRATION_TEST_BIN): tests/unit/test_coordinator_orchestration.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(CONTROL_CENTER_TEST_BIN): tests/unit/test_control_center.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(ROAD_PERCEPTION_TEST_BIN): tests/unit/test_road_perception.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(DIGITAL_TWIN_TEST_BIN): tests/unit/test_digital_twin_operations_center.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(PRODUCTION_RELIABILITY_TEST_BIN): tests/unit/test_production_reliability.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(SYSTEM_VVE_TEST_BIN): tests/unit/test_system_vve.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(CLUSTER_DEPLOYMENT_TEST_BIN): tests/unit/test_cluster_deployment_hardware.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(PHASE12_DEMO_TEST_BIN): tests/unit/test_phase12_hardware_demo.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

$(PHASE13_COMMAND_CENTER_TEST_BIN): tests/unit/test_phase13_command_center.cpp $(STATIC_LIB)
	@echo "[LINK] $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $< -L$(LIB_DIR) -laegis -o $@

test: directories $(SMOKE_TEST_BIN) $(FOUNDATION_TEST_BIN) $(COMM_TEST_BIN) $(COMM_MOD_TEST_BIN) $(COORD_TEST_BIN) $(COORD_FOUNDATION_TEST_BIN) $(GREEN_CORRIDOR_TEST_BIN) $(AMB_TEST_BIN) $(TRAFFIC_TEST_BIN) $(INTERSECTION_NODE_TEST_BIN) $(SYSTEM_INTEGRATION_TEST_BIN) $(RPI_DEPLOYMENT_TEST_BIN) $(MEDICAL_TEST_BIN) $(AMB_REG_TEST_BIN) $(CONFLICT_TEST_BIN) $(CONTROL_TEST_BIN) $(LOGGER_TEST_BIN) $(PHASE8_TEST_BIN) $(COORD_MONITORING_TEST_BIN) $(TRAFFIC_SIGNAL_PHASE6A_TEST_BIN) $(COORD_TRAFFIC_SIGNAL_PHASE6B_TEST_BIN) $(EMERGENCY_CORRIDOR_PHASE7A_TEST_BIN) $(GREEN_WAVE_PHASE7B_TEST_BIN) $(PRIORITY_SCHEDULER_PHASE8A_TEST_BIN) $(EMERGENCY_ORCHESTRATOR_PHASE8B_TEST_BIN) $(DYNAMIC_ROUTE_REPLANNER_PHASE9A_TEST_BIN) $(MULTI_CORRIDOR_COORDINATOR_PHASE9B_TEST_BIN) $(DISTRIBUTED_SIGNAL_PHASE10A_TEST_BIN) $(PREDICTIVE_TRAFFIC_PHASE10B_TEST_BIN) $(FLEET_MANAGEMENT_PHASE11A_TEST_BIN) $(HOSPITAL_SELECTION_PHASE11B_TEST_BIN) $(HOSPITAL_RESOURCE_PHASE11B_TEST_BIN) $(MISSION_EXECUTION_PHASE12A_TEST_BIN) $(INCIDENT_COMMAND_PHASE12B_TEST_BIN) $(TRAFFIC_NETWORK_ORCHESTRATOR_PHASE13A_TEST_BIN) $(DISTRIBUTED_CONSENSUS_PHASE13B_TEST_BIN) $(RPI_CLUSTER_PHASE14A_TEST_BIN) $(SYSTEM_DIAGNOSTICS_PHASE14B_TEST_BIN) $(HARDWARE_ABSTRACTION_PHASE15A_TEST_BIN) $(QNX_DRIVER_FRAMEWORK_PHASE15B_TEST_BIN) $(BSP_PHASE16A_TEST_BIN) $(QNX_NETWORK_TEST_BIN) $(DISTRIBUTED_RUNTIME_PHASE16C_TEST_BIN) $(DEPLOYMENT_LAYER_PHASE16D_TEST_BIN) $(CITY_SIMULATION_PHASE19_TEST_BIN) $(HARDWARE_INTEGRATION_PHASE20_TEST_BIN) $(QNX_DEPLOYMENT_PHASE20_5_TEST_BIN) $(QNX_HARDENING_PHASE20_6_TEST_BIN) $(QNX_HARDENING_PHASE20_7_TEST_BIN) $(PHASE1_HARDWARE_TEST_BIN) $(PHASE2_EMBEDDED_TEST_BIN) $(PHASE3_CORRIDOR_TEST_BIN) $(PHASE4_QNX_TEST_BIN) $(COORDINATOR_TCP_TEST_BIN) $(TRAFFIC_NODE_TEST_BIN) $(COORD_INTEL_TEST_BIN) $(COORD_ORCHESTRATION_TEST_BIN) $(CONTROL_CENTER_TEST_BIN) $(ROAD_PERCEPTION_TEST_BIN) $(DIGITAL_TWIN_TEST_BIN) $(PRODUCTION_RELIABILITY_TEST_BIN) $(SYSTEM_VVE_TEST_BIN) $(CLUSTER_DEPLOYMENT_TEST_BIN) $(PHASE12_DEMO_TEST_BIN) $(PHASE13_COMMAND_CENTER_TEST_BIN)
	@echo "=========================================================="
	@echo " Running SMART AEGIS Test Suites"
	@echo "=========================================================="
	@$(SMOKE_TEST_BIN)
	@$(FOUNDATION_TEST_BIN)
	@$(COMM_TEST_BIN)
	@$(COMM_MOD_TEST_BIN)
	@$(COORD_TEST_BIN)
	@$(COORD_FOUNDATION_TEST_BIN)
	@$(GREEN_CORRIDOR_TEST_BIN)
	@$(AMB_TEST_BIN)
	@$(TRAFFIC_TEST_BIN)
	@$(INTERSECTION_NODE_TEST_BIN)
	@$(SYSTEM_INTEGRATION_TEST_BIN)
	@cd $(BUILD_DIR) && ./bin/test_rpi_deployment
	@$(MEDICAL_TEST_BIN)
	@$(AMB_REG_TEST_BIN)
	@$(CONFLICT_TEST_BIN)
	@$(CONTROL_TEST_BIN)
	@$(LOGGER_TEST_BIN)
	@$(PHASE8_TEST_BIN)
	@$(COORD_MONITORING_TEST_BIN)
	@$(TRAFFIC_SIGNAL_PHASE6A_TEST_BIN)
	@$(COORD_TRAFFIC_SIGNAL_PHASE6B_TEST_BIN)
	@$(EMERGENCY_CORRIDOR_PHASE7A_TEST_BIN)
	@$(GREEN_WAVE_PHASE7B_TEST_BIN)
	@$(PRIORITY_SCHEDULER_PHASE8A_TEST_BIN)
	@$(EMERGENCY_ORCHESTRATOR_PHASE8B_TEST_BIN)
	@$(DYNAMIC_ROUTE_REPLANNER_PHASE9A_TEST_BIN)
	@$(MULTI_CORRIDOR_COORDINATOR_PHASE9B_TEST_BIN)
	@$(DISTRIBUTED_SIGNAL_PHASE10A_TEST_BIN)
	@$(PREDICTIVE_TRAFFIC_PHASE10B_TEST_BIN)
	@$(FLEET_MANAGEMENT_PHASE11A_TEST_BIN)
	@$(HOSPITAL_SELECTION_PHASE11B_TEST_BIN)
	@$(HOSPITAL_RESOURCE_PHASE11B_TEST_BIN)
	@$(MISSION_EXECUTION_PHASE12A_TEST_BIN)
	@$(INCIDENT_COMMAND_PHASE12B_TEST_BIN)
	@$(TRAFFIC_NETWORK_ORCHESTRATOR_PHASE13A_TEST_BIN)
	@$(DISTRIBUTED_CONSENSUS_PHASE13B_TEST_BIN)
	@$(RPI_CLUSTER_PHASE14A_TEST_BIN)
	@$(SYSTEM_DIAGNOSTICS_PHASE14B_TEST_BIN)
	@$(HARDWARE_ABSTRACTION_PHASE15A_TEST_BIN)
	@$(QNX_DRIVER_FRAMEWORK_PHASE15B_TEST_BIN)
	@$(BSP_PHASE16A_TEST_BIN)
	@$(QNX_NETWORK_TEST_BIN)
	@$(DISTRIBUTED_RUNTIME_PHASE16C_TEST_BIN)
	@$(DEPLOYMENT_LAYER_PHASE16D_TEST_BIN)
	@$(CITY_SIMULATION_PHASE19_TEST_BIN)
	@$(HARDWARE_INTEGRATION_PHASE20_TEST_BIN)
	@$(QNX_DEPLOYMENT_PHASE20_5_TEST_BIN)
	@$(QNX_HARDENING_PHASE20_6_TEST_BIN)
	@$(QNX_HARDENING_PHASE20_7_TEST_BIN)
	@$(PHASE1_HARDWARE_TEST_BIN)
	@$(PHASE2_EMBEDDED_TEST_BIN)
	@$(PHASE3_CORRIDOR_TEST_BIN)
	@$(PHASE4_QNX_TEST_BIN)
	@$(COORDINATOR_TCP_TEST_BIN)
	@$(TRAFFIC_NODE_TEST_BIN)
	@$(COORD_INTEL_TEST_BIN)
	@$(COORD_ORCHESTRATION_TEST_BIN)
	@$(CONTROL_CENTER_TEST_BIN)
	@$(ROAD_PERCEPTION_TEST_BIN)
	@$(DIGITAL_TWIN_TEST_BIN)
	@$(PRODUCTION_RELIABILITY_TEST_BIN)
	@$(SYSTEM_VVE_TEST_BIN)
	@$(CLUSTER_DEPLOYMENT_TEST_BIN)
	@$(PHASE12_DEMO_TEST_BIN)
	@$(PHASE13_COMMAND_CENTER_TEST_BIN)

clean:
	@rm -rf $(BUILD_DIR)
	@echo "Cleaned build artifacts."
