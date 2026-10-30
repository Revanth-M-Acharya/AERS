add_library(aegis_mission_execution STATIC
    MissionExecutionEngine.cpp
)

target_include_directories(aegis_mission_execution PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_mission_execution PUBLIC
    aegis_common
    aegis_logger
    aegis_config
)

