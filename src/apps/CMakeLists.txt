# Executable Targets

# Central Coordinator Daemon (Raspberry Pi 1 / QNX RTOS)
add_executable(aegis_coordinator coordinator_main.cpp)
target_include_directories(aegis_coordinator PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(aegis_coordinator PRIVATE
    aegis_coordinator
    aegis_communication
    aegis_route_planner
    aegis_priority_scheduler
    aegis_network
    aegis_logger
    aegis_config
    aegis_common
)

# Intersection Node Daemon (Raspberry Pis 2, 3, 4)
add_executable(aegis_intersection_node intersection_main.cpp)
target_include_directories(aegis_intersection_node PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(aegis_intersection_node PRIVATE
    aegis_intersection
    aegis_communication
    aegis_config
    aegis_logger
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
