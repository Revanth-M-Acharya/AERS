add_library(aegis_multi_corridor STATIC
    MultiCorridorCoordinator.cpp
)

target_include_directories(aegis_multi_corridor PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_multi_corridor PUBLIC
    aegis_replanning
    aegis_green_corridor
    aegis_green_wave
    aegis_coordinator
    aegis_orchestrator
    aegis_scheduler
    aegis_common
    aegis_logger
    aegis_config
)
