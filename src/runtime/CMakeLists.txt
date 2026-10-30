add_library(aegis_runtime STATIC
    RuntimeEventFlow.cpp
    CoordinatorRuntime.cpp
    TrafficNodeRuntime.cpp
    DistributedRuntimeOrchestrator.cpp
)

target_include_directories(aegis_runtime PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/network
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_runtime PUBLIC
    Threads::Threads
    aegis_qnx_network
    aegis_common
    aegis_logger
    aegis_config
    aegis_safety
    aegis_network
    aegis_coordinator
    aegis_medical
    aegis_incident_command
    aegis_fleet_management
    aegis_route_planner
    aegis_replanning
    aegis_green_corridor
    aegis_green_wave
    aegis_scheduler
    aegis_multi_corridor
    aegis_mission_execution
    aegis_hospital_management
    aegis_hospital_selection
    aegis_traffic_network_orchestrator
    aegis_distributed_consensus
    aegis_rpi_cluster
    aegis_system_diagnostics
    aegis_hardware_abstraction
    aegis_qnx_driver_framework
    aegis_bsp
    aegis_traffic_signal
    aegis_intersection
)
