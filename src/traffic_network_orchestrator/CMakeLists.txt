add_library(aegis_traffic_network_orchestrator STATIC
    TrafficNetworkOrchestrator.cpp
)

target_include_directories(aegis_traffic_network_orchestrator PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_traffic_network_orchestrator PUBLIC
    aegis_common
    aegis_logger
    aegis_config
)

