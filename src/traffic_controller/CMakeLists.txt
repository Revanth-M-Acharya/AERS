add_library(aegis_traffic_controller STATIC
    traffic_signal_node.cpp
    traffic_controller.cpp
    signal_state_machine.cpp
    preemption_manager.cpp
    recovery_manager.cpp
    gpio_interface.cpp
)
target_include_directories(aegis_traffic_controller PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(aegis_traffic_controller PUBLIC
    aegis_common
    aegis_network
    aegis_safety
    aegis_logger
    aegis_config
)
