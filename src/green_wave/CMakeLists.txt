# GreenWaveCoordinator Module CMake Configuration (Phase 7B)
add_library(aegis_green_wave STATIC
    GreenWaveCoordinator.cpp
)

target_include_directories(aegis_green_wave PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_link_libraries(aegis_green_wave PUBLIC
    aegis_green_corridor
    aegis_common
    aegis_logger
)
