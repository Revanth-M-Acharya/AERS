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




