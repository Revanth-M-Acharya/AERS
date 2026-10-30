# Emergency Corridor Planner Module CMake Configuration (Phase 7A)
add_library(aegis_green_corridor STATIC
    EmergencyCorridorPlanner.cpp
)

target_include_directories(aegis_green_corridor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_link_libraries(aegis_green_corridor PUBLIC
    aegis_common
    aegis_logger
)
