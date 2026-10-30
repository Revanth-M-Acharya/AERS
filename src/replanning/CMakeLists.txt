add_library(aegis_replanning STATIC
    DynamicRouteReplanner.cpp
)

target_include_directories(aegis_replanning PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_replanning PUBLIC
    aegis_green_corridor
    aegis_green_wave
    aegis_coordinator
    aegis_orchestrator
    aegis_scheduler
    aegis_common
    aegis_logger
    aegis_config
)
