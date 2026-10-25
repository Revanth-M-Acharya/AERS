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

add_executable(test_ambulance_node unit/test_ambulance_node.cpp)
target_link_libraries(test_ambulance_node PRIVATE
    aegis_ambulance
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
