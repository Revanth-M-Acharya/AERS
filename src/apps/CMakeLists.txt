# Executable Targets

# Coordinator Daemon
add_executable(aegis_coordinator_app coordinator_main.cpp)
target_link_libraries(aegis_coordinator_app PRIVATE
    aegis_coordinator
    aegis_route_planner
    aegis_priority_scheduler
    aegis_network
    aegis_logger
    aegis_config
    aegis_common
)

# Ambulance In-Vehicle OBU Daemon
add_executable(aegis_ambulance_app ambulance_main.cpp)
target_link_libraries(aegis_ambulance_app PRIVATE
    aegis_ambulance
    aegis_network
    aegis_safety
    aegis_logger
    aegis_config
    aegis_common
)

# Traffic Intersection RSU Daemon
add_executable(aegis_traffic_app traffic_main.cpp)
target_link_libraries(aegis_traffic_app PRIVATE
    aegis_traffic_controller
    aegis_network
    aegis_safety
    aegis_logger
    aegis_config
    aegis_common
)
