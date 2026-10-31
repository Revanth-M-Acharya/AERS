# Test Suites Configuration
add_executable(test_build_smoke unit/test_build_smoke.cpp)
target_link_libraries(test_build_smoke PRIVATE
    aegis_common
    aegis_config
    aegis_logger
    aegis_safety
    aegis_network
    aegis_priority_scheduler
    aegis_route_planner
    aegis_coordinator
    aegis_ambulance
    aegis_traffic_controller
    aegis_hospital_dashboard
    aegis_traffic_dashboard
)
add_test(NAME test_build_smoke COMMAND test_build_smoke)

add_executable(test_common_foundation unit/test_common_foundation.cpp)
target_link_libraries(test_common_foundation PRIVATE
    aegis_common
)
add_test(NAME test_common_foundation COMMAND test_common_foundation)

add_executable(test_communication_infrastructure unit/test_communication_infrastructure.cpp)
target_link_libraries(test_communication_infrastructure PRIVATE
    aegis_network
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_communication_infrastructure COMMAND test_communication_infrastructure)

add_executable(test_communication_module unit/test_communication_module.cpp)
target_link_libraries(test_communication_module PRIVATE
    aegis_communication
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_communication_module COMMAND test_communication_module)

add_executable(test_coordinator_core unit/test_coordinator_core.cpp)
target_link_libraries(test_coordinator_core PRIVATE
    aegis_coordinator
    aegis_route_planner
    aegis_priority_scheduler
    aegis_network
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_coordinator_core COMMAND test_coordinator_core)

add_executable(test_coordinator_foundation unit/test_coordinator_foundation.cpp)
target_link_libraries(test_coordinator_foundation PRIVATE
    aegis_coordinator
    aegis_communication
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_coordinator_foundation COMMAND test_coordinator_foundation)

add_executable(test_green_corridor unit/test_green_corridor.cpp)
target_link_libraries(test_green_corridor PRIVATE
    aegis_coordinator
    aegis_communication
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_green_corridor COMMAND test_green_corridor)

add_executable(test_ambulance_node unit/test_ambulance_node.cpp)
target_link_libraries(test_ambulance_node PRIVATE
    aegis_ambulance
    aegis_coordinator
    aegis_communication
    aegis_network
    aegis_common
    aegis_logger
    aegis_config
    aegis_safety
)
add_test(NAME test_ambulance_node COMMAND test_ambulance_node)

add_executable(test_traffic_signal_node unit/test_traffic_signal_node.cpp)
target_link_libraries(test_traffic_signal_node PRIVATE
    aegis_traffic_controller
    aegis_network
    aegis_common
    aegis_logger
    aegis_config
    aegis_safety
)
add_test(NAME test_traffic_signal_node COMMAND test_traffic_signal_node)

add_executable(test_intersection_node unit/test_intersection_node.cpp)
target_link_libraries(test_intersection_node PRIVATE
    aegis_intersection
    aegis_coordinator
    aegis_communication
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_intersection_node COMMAND test_intersection_node)

add_executable(test_system_integration integration/test_system_integration.cpp)
target_link_libraries(test_system_integration PRIVATE
    aegis_system
    aegis_intersection
    aegis_coordinator
    aegis_communication
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_system_integration COMMAND test_system_integration)

add_executable(test_rpi_deployment integration/test_rpi_deployment.cpp)
target_link_libraries(test_rpi_deployment PRIVATE
    aegis_intersection
    aegis_coordinator
    aegis_communication
    aegis_config
    aegis_common
    aegis_logger
)
add_test(
    NAME test_rpi_deployment
    COMMAND test_rpi_deployment
)

set_tests_properties(test_rpi_deployment PROPERTIES
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_executable(test_medical_priority unit/test_medical_priority.cpp)
target_link_libraries(test_medical_priority PRIVATE
    aegis_medical
    aegis_common
)
add_test(NAME test_medical_priority COMMAND test_medical_priority)

add_executable(test_ambulance_registry unit/test_ambulance_registry.cpp)
target_link_libraries(test_ambulance_registry PRIVATE
    aegis_ambulance
    aegis_medical
    aegis_common
)
add_test(NAME test_ambulance_registry COMMAND test_ambulance_registry)

add_executable(test_conflict_detection unit/test_conflict_detection.cpp)
target_link_libraries(test_conflict_detection PRIVATE
    aegis_scheduler
    aegis_ambulance
    aegis_medical
    aegis_common
)
add_test(NAME test_conflict_detection COMMAND test_conflict_detection)

add_executable(test_manual_override unit/test_manual_override.cpp)
target_link_libraries(test_manual_override PRIVATE
    aegis_control
    aegis_scheduler
    aegis_ambulance
    aegis_medical
    aegis_common
)
add_test(NAME test_manual_override COMMAND test_manual_override)

add_executable(test_event_logger unit/test_event_logger.cpp)
target_link_libraries(test_event_logger PRIVATE
    aegis_logger
    aegis_common
)
add_test(NAME test_event_logger COMMAND test_event_logger)

add_executable(test_phase8_intelligent_scheduler unit/test_phase8_intelligent_scheduler.cpp)
target_link_libraries(test_phase8_intelligent_scheduler PRIVATE
    aegis_scheduler
    aegis_ambulance
    aegis_medical
    aegis_coordinator
    aegis_communication
    aegis_logger
    aegis_common
)
add_test(NAME test_phase8_intelligent_scheduler COMMAND test_phase8_intelligent_scheduler)

add_executable(test_coordinator_monitoring unit/test_coordinator_monitoring.cpp)
target_link_libraries(test_coordinator_monitoring PRIVATE
    aegis_coordinator
    aegis_communication
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_coordinator_monitoring COMMAND test_coordinator_monitoring)

add_executable(test_traffic_signal_node_phase6a unit/test_traffic_signal_node_phase6a.cpp)
target_link_libraries(test_traffic_signal_node_phase6a PRIVATE
    aegis_traffic_signal
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_traffic_signal_node_phase6a COMMAND test_traffic_signal_node_phase6a)

add_executable(test_coordinator_traffic_signal_phase6b unit/test_coordinator_traffic_signal_phase6b.cpp)
target_link_libraries(test_coordinator_traffic_signal_phase6b PRIVATE
    aegis_coordinator
    aegis_intersection
    aegis_traffic_signal
    aegis_communication
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_coordinator_traffic_signal_phase6b COMMAND test_coordinator_traffic_signal_phase6b)

add_executable(test_emergency_corridor_planner_phase7a unit/test_emergency_corridor_planner_phase7a.cpp)
target_link_libraries(test_emergency_corridor_planner_phase7a PRIVATE
    aegis_green_corridor
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_emergency_corridor_planner_phase7a COMMAND test_emergency_corridor_planner_phase7a)

add_executable(test_green_wave_coordinator_phase7b unit/test_green_wave_coordinator_phase7b.cpp)
target_link_libraries(test_green_wave_coordinator_phase7b PRIVATE
    aegis_green_wave
    aegis_green_corridor
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_green_wave_coordinator_phase7b COMMAND test_green_wave_coordinator_phase7b)

add_executable(test_priority_scheduler_phase8a unit/test_priority_scheduler_phase8a.cpp)
target_link_libraries(test_priority_scheduler_phase8a PRIVATE
    aegis_scheduler
    aegis_ambulance
    aegis_medical
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_priority_scheduler_phase8a COMMAND test_priority_scheduler_phase8a)

add_executable(test_emergency_orchestrator_phase8b unit/test_emergency_orchestrator_phase8b.cpp)
target_link_libraries(test_emergency_orchestrator_phase8b PRIVATE
    aegis_orchestrator
    aegis_scheduler
    aegis_green_wave
    aegis_green_corridor
    aegis_coordinator
    aegis_intersection
    aegis_traffic_signal
    aegis_communication
    aegis_ambulance
    aegis_medical
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_emergency_orchestrator_phase8b COMMAND test_emergency_orchestrator_phase8b)

add_executable(test_dynamic_route_replanner_phase9a unit/test_dynamic_route_replanner_phase9a.cpp)
target_link_libraries(test_dynamic_route_replanner_phase9a PRIVATE
    aegis_replanning
    aegis_orchestrator
    aegis_scheduler
    aegis_green_wave
    aegis_green_corridor
    aegis_coordinator
    aegis_intersection
    aegis_traffic_signal
    aegis_communication
    aegis_ambulance
    aegis_medical
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_dynamic_route_replanner_phase9a COMMAND test_dynamic_route_replanner_phase9a)

add_executable(test_multi_corridor_coordinator_phase9b unit/test_multi_corridor_coordinator_phase9b.cpp)
target_link_libraries(test_multi_corridor_coordinator_phase9b PRIVATE
    aegis_multi_corridor
    aegis_replanning
    aegis_orchestrator
    aegis_scheduler
    aegis_green_wave
    aegis_green_corridor
    aegis_coordinator
    aegis_intersection
    aegis_traffic_signal
    aegis_communication
    aegis_ambulance
    aegis_medical
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_multi_corridor_coordinator_phase9b COMMAND test_multi_corridor_coordinator_phase9b)

add_executable(test_distributed_signal_phase10a unit/test_distributed_signal_phase10a.cpp)
target_link_libraries(test_distributed_signal_phase10a PRIVATE
    aegis_distributed_signal
    aegis_multi_corridor
    aegis_replanning
    aegis_orchestrator
    aegis_scheduler
    aegis_green_wave
    aegis_green_corridor
    aegis_coordinator
    aegis_intersection
    aegis_traffic_signal
    aegis_communication
    aegis_ambulance
    aegis_medical
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_distributed_signal_phase10a COMMAND test_distributed_signal_phase10a)

add_executable(test_predictive_traffic_phase10b unit/test_predictive_traffic_phase10b.cpp)
target_link_libraries(test_predictive_traffic_phase10b PRIVATE
    aegis_predictive_traffic
    aegis_distributed_signal
    aegis_multi_corridor
    aegis_replanning
    aegis_orchestrator
    aegis_scheduler
    aegis_green_wave
    aegis_green_corridor
    aegis_coordinator
    aegis_intersection
    aegis_traffic_signal
    aegis_communication
    aegis_ambulance
    aegis_medical
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_predictive_traffic_phase10b COMMAND test_predictive_traffic_phase10b)

add_executable(test_fleet_management_phase11a unit/test_fleet_management_phase11a.cpp)
target_link_libraries(test_fleet_management_phase11a PRIVATE
    aegis_fleet_management
    aegis_predictive_traffic
    aegis_distributed_signal
    aegis_multi_corridor
    aegis_replanning
    aegis_orchestrator
    aegis_scheduler
    aegis_green_wave
    aegis_green_corridor
    aegis_coordinator
    aegis_intersection
    aegis_traffic_signal
    aegis_communication
    aegis_ambulance
    aegis_medical
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_fleet_management_phase11a COMMAND test_fleet_management_phase11a)

add_executable(test_hospital_selection_phase11b unit/test_hospital_selection_phase11b.cpp)
target_link_libraries(test_hospital_selection_phase11b PRIVATE
    aegis_hospital_selection
    aegis_fleet_management
    aegis_predictive_traffic
    aegis_distributed_signal
    aegis_multi_corridor
    aegis_replanning
    aegis_orchestrator
    aegis_scheduler
    aegis_green_wave
    aegis_green_corridor
    aegis_coordinator
    aegis_intersection
    aegis_traffic_signal
    aegis_communication
    aegis_ambulance
    aegis_medical
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_hospital_selection_phase11b COMMAND test_hospital_selection_phase11b)

add_executable(test_hospital_resource_phase11b unit/test_hospital_resource_phase11b.cpp)
target_link_libraries(test_hospital_resource_phase11b PRIVATE
    aegis_hospital_management
    aegis_hospital_selection
    aegis_fleet_management
    aegis_predictive_traffic
    aegis_distributed_signal
    aegis_multi_corridor
    aegis_replanning
    aegis_orchestrator
    aegis_scheduler
    aegis_green_wave
    aegis_green_corridor
    aegis_coordinator
    aegis_intersection
    aegis_traffic_signal
    aegis_communication
    aegis_ambulance
    aegis_medical
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_hospital_resource_phase11b COMMAND test_hospital_resource_phase11b)

add_executable(test_mission_execution_phase12a unit/test_mission_execution_phase12a.cpp)
target_link_libraries(test_mission_execution_phase12a PRIVATE
    aegis_mission_execution
    aegis_hospital_management
    aegis_hospital_selection
    aegis_fleet_management
    aegis_predictive_traffic
    aegis_distributed_signal
    aegis_multi_corridor
    aegis_replanning
    aegis_orchestrator
    aegis_scheduler
    aegis_green_wave
    aegis_green_corridor
    aegis_coordinator
    aegis_intersection
    aegis_traffic_signal
    aegis_communication
    aegis_ambulance
    aegis_medical
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_mission_execution_phase12a COMMAND test_mission_execution_phase12a)

add_executable(test_incident_command_phase12b unit/test_incident_command_phase12b.cpp)
target_link_libraries(test_incident_command_phase12b PRIVATE
    aegis_incident_command
    aegis_mission_execution
    aegis_hospital_management
    aegis_hospital_selection
    aegis_fleet_management
    aegis_predictive_traffic
    aegis_distributed_signal
    aegis_multi_corridor
    aegis_replanning
    aegis_orchestrator
    aegis_scheduler
    aegis_green_wave
    aegis_green_corridor
    aegis_coordinator
    aegis_intersection
    aegis_traffic_signal
    aegis_communication
    aegis_ambulance
    aegis_medical
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_incident_command_phase12b COMMAND test_incident_command_phase12b)

add_executable(test_traffic_network_orchestrator_phase13a unit/test_traffic_network_orchestrator_phase13a.cpp)
target_link_libraries(test_traffic_network_orchestrator_phase13a PRIVATE
    aegis_traffic_network_orchestrator
    aegis_incident_command
    aegis_mission_execution
    aegis_hospital_management
    aegis_hospital_selection
    aegis_fleet_management
    aegis_predictive_traffic
    aegis_distributed_signal
    aegis_multi_corridor
    aegis_replanning
    aegis_orchestrator
    aegis_scheduler
    aegis_green_wave
    aegis_green_corridor
    aegis_coordinator
    aegis_intersection
    aegis_traffic_signal
    aegis_communication
    aegis_ambulance
    aegis_medical
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_traffic_network_orchestrator_phase13a COMMAND test_traffic_network_orchestrator_phase13a)

add_executable(test_distributed_consensus_phase13b unit/test_distributed_consensus_phase13b.cpp)
target_link_libraries(test_distributed_consensus_phase13b PRIVATE
    aegis_distributed_consensus
    aegis_traffic_network_orchestrator
    aegis_incident_command
    aegis_mission_execution
    aegis_hospital_management
    aegis_hospital_selection
    aegis_fleet_management
    aegis_predictive_traffic
    aegis_distributed_signal
    aegis_multi_corridor
    aegis_replanning
    aegis_orchestrator
    aegis_scheduler
    aegis_green_wave
    aegis_green_corridor
    aegis_coordinator
    aegis_intersection
    aegis_traffic_signal
    aegis_communication
    aegis_ambulance
    aegis_medical
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_distributed_consensus_phase13b COMMAND test_distributed_consensus_phase13b)

add_executable(test_rpi_cluster_phase14a unit/test_rpi_cluster_phase14a.cpp)
target_link_libraries(test_rpi_cluster_phase14a PRIVATE
    aegis_rpi_cluster
    aegis_distributed_consensus
    aegis_traffic_network_orchestrator
    aegis_incident_command
    aegis_mission_execution
    aegis_hospital_management
    aegis_hospital_selection
    aegis_fleet_management
    aegis_predictive_traffic
    aegis_distributed_signal
    aegis_multi_corridor
    aegis_replanning
    aegis_orchestrator
    aegis_scheduler
    aegis_green_wave
    aegis_green_corridor
    aegis_coordinator
    aegis_intersection
    aegis_traffic_signal
    aegis_communication
    aegis_ambulance
    aegis_medical
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_rpi_cluster_phase14a COMMAND test_rpi_cluster_phase14a)

add_executable(test_system_diagnostics_phase14b unit/test_system_diagnostics_phase14b.cpp)
target_link_libraries(test_system_diagnostics_phase14b PRIVATE
    aegis_system_diagnostics
    aegis_rpi_cluster
    aegis_distributed_consensus
    aegis_traffic_network_orchestrator
    aegis_incident_command
    aegis_mission_execution
    aegis_hospital_management
    aegis_hospital_selection
    aegis_fleet_management
    aegis_predictive_traffic
    aegis_distributed_signal
    aegis_multi_corridor
    aegis_replanning
    aegis_orchestrator
    aegis_scheduler
    aegis_green_wave
    aegis_green_corridor
    aegis_coordinator
    aegis_intersection
    aegis_traffic_signal
    aegis_communication
    aegis_ambulance
    aegis_medical
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_system_diagnostics_phase14b COMMAND test_system_diagnostics_phase14b)

add_executable(test_hardware_abstraction_phase15a unit/test_hardware_abstraction_phase15a.cpp)
target_link_libraries(test_hardware_abstraction_phase15a PRIVATE
    aegis_hardware_abstraction
    aegis_system_diagnostics
    aegis_rpi_cluster
    aegis_distributed_consensus
    aegis_traffic_network_orchestrator
    aegis_incident_command
    aegis_mission_execution
    aegis_hospital_management
    aegis_hospital_selection
    aegis_fleet_management
    aegis_predictive_traffic
    aegis_distributed_signal
    aegis_multi_corridor
    aegis_replanning
    aegis_orchestrator
    aegis_scheduler
    aegis_green_wave
    aegis_green_corridor
    aegis_coordinator
    aegis_intersection
    aegis_traffic_signal
    aegis_communication
    aegis_ambulance
    aegis_medical
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_hardware_abstraction_phase15a COMMAND test_hardware_abstraction_phase15a)

add_executable(test_qnx_driver_framework_phase15b unit/test_qnx_driver_framework_phase15b.cpp)
target_link_libraries(test_qnx_driver_framework_phase15b PRIVATE
    aegis_qnx_driver_framework
    aegis_hardware_abstraction
    aegis_system_diagnostics
    aegis_rpi_cluster
    aegis_distributed_consensus
    aegis_traffic_network_orchestrator
    aegis_incident_command
    aegis_mission_execution
    aegis_hospital_management
    aegis_hospital_selection
    aegis_fleet_management
    aegis_predictive_traffic
    aegis_distributed_signal
    aegis_multi_corridor
    aegis_replanning
    aegis_orchestrator
    aegis_scheduler
    aegis_green_wave
    aegis_green_corridor
    aegis_coordinator
    aegis_intersection
    aegis_traffic_signal
    aegis_communication
    aegis_ambulance
    aegis_medical
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_qnx_driver_framework_phase15b COMMAND test_qnx_driver_framework_phase15b)

add_executable(test_raspberry_pi4_bsp_phase16a unit/test_raspberry_pi4_bsp_phase16a.cpp)
target_link_libraries(test_raspberry_pi4_bsp_phase16a PRIVATE
    aegis_bsp
    aegis_qnx_driver_framework
    aegis_hardware_abstraction
    aegis_system_diagnostics
    aegis_rpi_cluster
    aegis_distributed_consensus
    aegis_traffic_network_orchestrator
    aegis_incident_command
    aegis_mission_execution
    aegis_hospital_management
    aegis_hospital_selection
    aegis_fleet_management
    aegis_predictive_traffic
    aegis_distributed_signal
    aegis_multi_corridor
    aegis_replanning
    aegis_orchestrator
    aegis_scheduler
    aegis_green_wave
    aegis_green_corridor
    aegis_coordinator
    aegis_intersection
    aegis_traffic_signal
    aegis_communication
    aegis_ambulance
    aegis_medical
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_raspberry_pi4_bsp_phase16a COMMAND test_raspberry_pi4_bsp_phase16a)

add_executable(test_qnx_network_layer
    unit/test_qnx_network_layer.cpp
    ../network/message.cpp
    ../network/tcp_server.cpp
    ../network/tcp_client.cpp
)
target_include_directories(test_qnx_network_layer PRIVATE ${CMAKE_SOURCE_DIR}/network)
target_link_libraries(test_qnx_network_layer PRIVATE
    Threads::Threads
    aegis_common
    aegis_logger
    aegis_config
)
add_test(NAME test_qnx_network_layer COMMAND test_qnx_network_layer)

add_executable(test_distributed_runtime_phase16c unit/test_distributed_runtime_integration_phase16c.cpp)
target_include_directories(test_distributed_runtime_phase16c PRIVATE ${CMAKE_SOURCE_DIR}/network ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_distributed_runtime_phase16c PRIVATE
    aegis_runtime
    Threads::Threads
)
add_test(NAME test_distributed_runtime_phase16c COMMAND test_distributed_runtime_phase16c)

add_executable(test_deployment_layer_phase16d unit/test_deployment_layer_phase16d.cpp)
target_include_directories(test_deployment_layer_phase16d PRIVATE ${CMAKE_SOURCE_DIR}/network ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_deployment_layer_phase16d PRIVATE
    aegis_deployment
    aegis_runtime
    Threads::Threads
)
add_test(NAME test_deployment_layer_phase16d COMMAND test_deployment_layer_phase16d)

# Phase 19 City Simulation Engine Tests
if(Qt6_FOUND)
    add_executable(test_city_simulation_phase19 unit/test_city_simulation_phase19.cpp)
    target_include_directories(test_city_simulation_phase19 PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(test_city_simulation_phase19 PRIVATE
        aegis_simulator
        Threads::Threads
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
    )
    add_test(NAME test_city_simulation_phase19 COMMAND test_city_simulation_phase19)
endif()

# Phase 20 Physical Hardware Integration Tests
add_executable(test_hardware_integration_phase20 unit/test_hardware_integration_phase20.cpp)
target_include_directories(test_hardware_integration_phase20 PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/network
)
target_link_libraries(test_hardware_integration_phase20 PRIVATE
    aegis_hardware
    aegis_bsp
    aegis_qnx_driver_framework
    aegis_hardware_abstraction
    aegis_deployment
    aegis_runtime
    aegis_common
    aegis_logger
    aegis_config
    Threads::Threads
)
add_test(NAME test_hardware_integration_phase20 COMMAND test_hardware_integration_phase20)

# Phase 20.5 QNX Raspberry Pi 4 Deployment & Platform Compatibility Tests
add_executable(test_qnx_deployment_phase20_5 unit/test_qnx_deployment_phase20_5.cpp)
target_include_directories(test_qnx_deployment_phase20_5 PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/network
)
target_link_libraries(test_qnx_deployment_phase20_5 PRIVATE
    aegis_hardware
    aegis_simulator
    aegis_bsp
    aegis_qnx_driver_framework
    aegis_hardware_abstraction
    aegis_deployment
    aegis_runtime
    aegis_common
    aegis_logger
    aegis_config
    Threads::Threads
)
add_test(NAME test_qnx_deployment_phase20_5 COMMAND test_qnx_deployment_phase20_5)

# Phase 20.6 QNX Deployment Hardening & Raspberry Pi 4 Validation Tests
add_executable(test_qnx_hardening_phase20_6 unit/test_qnx_hardening_phase20_6.cpp)
target_include_directories(test_qnx_hardening_phase20_6 PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/network
)
target_link_libraries(test_qnx_hardening_phase20_6 PRIVATE
    aegis_hardware
    aegis_simulator
    aegis_bsp
    aegis_qnx_driver_framework
    aegis_hardware_abstraction
    aegis_deployment
    aegis_runtime
    aegis_common
    aegis_logger
    aegis_config
    Threads::Threads
)
add_test(NAME test_qnx_hardening_phase20_6 COMMAND test_qnx_hardening_phase20_6)

# Phase 20.7 QNX Production Hardening Tests
add_executable(test_qnx_hardening_phase20_7 unit/test_qnx_hardening_phase20_7.cpp)
target_include_directories(test_qnx_hardening_phase20_7 PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/network
)
target_link_libraries(test_qnx_hardening_phase20_7 PRIVATE
    aegis_hardware
    aegis_coordinator
    aegis_simulator
    aegis_bsp
    aegis_qnx_driver_framework
    aegis_hardware_abstraction
    aegis_deployment
    aegis_runtime
    aegis_common
    aegis_logger
    aegis_config
    Threads::Threads
)
add_test(NAME test_qnx_hardening_phase20_7 COMMAND test_qnx_hardening_phase20_7)

add_executable(test_phase1_real_qnx_hardware unit/test_phase1_real_qnx_hardware.cpp)
target_link_libraries(test_phase1_real_qnx_hardware PRIVATE
    aegis_hardware
    aegis_bsp
    aegis_common
    aegis_logger
    aegis_config
    Threads::Threads
)
add_test(NAME test_phase1_real_qnx_hardware COMMAND test_phase1_real_qnx_hardware)

add_executable(test_phase2_embedded_production unit/test_phase2_embedded_production.cpp)
target_link_libraries(test_phase2_embedded_production PRIVATE
    aegis_hardware
    aegis_coordinator
    aegis_bsp
    aegis_common
    aegis_logger
    aegis_config
    Threads::Threads
)
add_test(NAME test_phase2_embedded_production COMMAND test_phase2_embedded_production)

add_executable(test_phase3_corridor_operations unit/test_phase3_corridor_operations.cpp)
target_link_libraries(test_phase3_corridor_operations PRIVATE
    aegis_corridor
    aegis_cluster
    aegis_recovery
    aegis_hardware
    aegis_coordinator
    aegis_bsp
    aegis_common
    aegis_logger
    aegis_config
    Threads::Threads
)
add_test(NAME test_phase3_corridor_operations COMMAND test_phase3_corridor_operations)

add_executable(test_phase4_native_qnx unit/test_phase4_native_qnx.cpp)
target_link_libraries(test_phase4_native_qnx PRIVATE
    aegis_qnx
    aegis_corridor
    aegis_cluster
    aegis_recovery
    aegis_hardware
    aegis_coordinator
    aegis_bsp
    aegis_common
    aegis_logger
    aegis_config
    Threads::Threads
)
add_test(NAME test_phase4_native_qnx COMMAND test_phase4_native_qnx)

add_executable(test_coordinator_tcp_server unit/test_coordinator_tcp_server.cpp)
target_link_libraries(test_coordinator_tcp_server PRIVATE
    coordinator_server_lib
    Threads::Threads
)
add_test(NAME test_coordinator_tcp_server COMMAND test_coordinator_tcp_server)

add_executable(test_traffic_node unit/test_traffic_node.cpp)
target_link_libraries(test_traffic_node PRIVATE
    traffic_node_client_lib
    coordinator_server_lib
    Threads::Threads
)
add_test(NAME test_traffic_node COMMAND test_traffic_node)

add_executable(test_coordinator_intelligence unit/test_coordinator_intelligence.cpp)
target_link_libraries(test_coordinator_intelligence PRIVATE
    coordinator_intel_lib
    coordinator_server_lib
    Threads::Threads
)
add_test(NAME test_coordinator_intelligence COMMAND test_coordinator_intelligence)

add_executable(test_coordinator_orchestration unit/test_coordinator_orchestration.cpp)
target_link_libraries(test_coordinator_orchestration PRIVATE
    coordinator_orchestration_lib
    coordinator_intel_lib
    coordinator_server_lib
    Threads::Threads
)
add_test(NAME test_coordinator_orchestration COMMAND test_coordinator_orchestration)

add_executable(test_control_center unit/test_control_center.cpp)
target_link_libraries(test_control_center PRIVATE
    coordinator_phase6_lib
    coordinator_orchestration_lib
    coordinator_intel_lib
    coordinator_server_lib
    Threads::Threads
)
add_test(NAME test_control_center COMMAND test_control_center)

add_executable(test_road_perception unit/test_road_perception.cpp)
target_link_libraries(test_road_perception PRIVATE
    coordinator_phase7_lib
    coordinator_phase6_lib
    coordinator_orchestration_lib
    coordinator_intel_lib
    coordinator_server_lib
    Threads::Threads
)
add_test(NAME test_road_perception COMMAND test_road_perception)

add_executable(test_digital_twin_operations_center unit/test_digital_twin_operations_center.cpp)
target_link_libraries(test_digital_twin_operations_center PRIVATE
    coordinator_phase8_lib
    coordinator_phase7_lib
    coordinator_phase6_lib
    coordinator_orchestration_lib
    coordinator_intel_lib
    coordinator_server_lib
    Threads::Threads
)
add_test(NAME test_digital_twin_operations_center COMMAND test_digital_twin_operations_center)

add_executable(test_production_reliability unit/test_production_reliability.cpp)
target_link_libraries(test_production_reliability PRIVATE
    coordinator_phase9_lib
    coordinator_phase8_lib
    coordinator_phase7_lib
    coordinator_phase6_lib
    coordinator_orchestration_lib
    coordinator_intel_lib
    coordinator_server_lib
    Threads::Threads
)
add_test(NAME test_production_reliability COMMAND test_production_reliability)

add_executable(test_system_vve unit/test_system_vve.cpp)
target_link_libraries(test_system_vve PRIVATE
    coordinator_phase9_lib
    coordinator_phase8_lib
    coordinator_phase7_lib
    coordinator_phase6_lib
    coordinator_orchestration_lib
    coordinator_intel_lib
    coordinator_server_lib
    Threads::Threads
)
add_test(NAME test_system_vve COMMAND test_system_vve)

add_executable(test_cluster_deployment_hardware unit/test_cluster_deployment_hardware.cpp)
target_link_libraries(test_cluster_deployment_hardware PRIVATE
    coordinator_phase9_lib
    coordinator_phase8_lib
    coordinator_phase7_lib
    coordinator_phase6_lib
    coordinator_orchestration_lib
    coordinator_intel_lib
    coordinator_server_lib
    Threads::Threads
)
add_test(NAME test_cluster_deployment_hardware COMMAND test_cluster_deployment_hardware)

















