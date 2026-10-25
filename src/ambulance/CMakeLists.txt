add_library(aegis_ambulance STATIC
    ambulance_node.cpp
    gps_tracker.cpp
    vitals_monitor.cpp
    v2v_module.cpp
    ambulance_mission_simulator.cpp
)
target_include_directories(aegis_ambulance PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(aegis_ambulance PUBLIC
    aegis_common
    aegis_network
    aegis_logger
    aegis_safety
    aegis_config
)
