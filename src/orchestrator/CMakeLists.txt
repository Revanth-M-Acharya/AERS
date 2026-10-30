add_library(aegis_orchestrator STATIC
    EmergencyOrchestrator.cpp
)

target_include_directories(aegis_orchestrator PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_orchestrator PUBLIC
    aegis_scheduler
    aegis_green_corridor
    aegis_green_wave
    aegis_coordinator
    aegis_common
    aegis_logger
    aegis_config
)
