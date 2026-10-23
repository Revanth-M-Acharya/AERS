add_library(aegis_coordinator STATIC
    emergency_call_handler.cpp
    incident_manager.cpp
    ambulance_registry.cpp
    fleet_manager.cpp
    dispatch_engine.cpp
    dynamic_green_corridor.cpp
    coordinator.cpp
)
target_include_directories(aegis_coordinator PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(aegis_coordinator PUBLIC
    aegis_common
    aegis_route_planner
    aegis_priority_scheduler
    aegis_network
    aegis_logger
    aegis_config
)
