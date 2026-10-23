add_library(aegis_coordinator STATIC
    coordinator.cpp
    emergency_call_handler.cpp
    dynamic_green_corridor.cpp
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
