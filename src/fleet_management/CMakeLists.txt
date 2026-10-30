add_library(aegis_fleet_management STATIC
    FleetManagementEngine.cpp
)

target_include_directories(aegis_fleet_management PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_fleet_management PUBLIC
    aegis_common
    aegis_logger
    aegis_config
)
