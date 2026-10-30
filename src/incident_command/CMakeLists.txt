add_library(aegis_incident_command STATIC
    IncidentCommandEngine.cpp
)

target_include_directories(aegis_incident_command PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_incident_command PUBLIC
    aegis_common
    aegis_logger
    aegis_config
)

