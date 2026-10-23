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
