add_library(aegis_coordinator STATIC
    emergency_call_handler.cpp
    incident_manager.cpp
    ambulance_registry.cpp
    fleet_manager.cpp
    dispatch_engine.cpp
    dynamic_green_corridor.cpp
    coordinator.cpp
    EventQueue.cpp
    IntersectionRegistry.cpp
    CoordinatorManager.cpp
    IntersectionState.cpp
    GreenCorridor.cpp
    TrafficCommandManager.cpp
    NodeRegistrationManager.cpp
)
target_include_directories(aegis_coordinator PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(aegis_coordinator PUBLIC
    aegis_common
    aegis_communication
    aegis_route_planner
    aegis_priority_scheduler
    aegis_network
    aegis_logger
    aegis_config
)
