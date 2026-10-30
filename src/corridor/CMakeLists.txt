add_library(aegis_corridor STATIC
    CorridorDeadlineMonitor.cpp
    EmergencyTimeoutManager.cpp
    DynamicCorridorReleaseManager.cpp
    RouteProgressTracker.cpp
    SignalTimelineGenerator.cpp
)

target_include_directories(aegis_corridor PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_corridor PUBLIC
    aegis_common
    aegis_logger
    aegis_green_corridor
    aegis_green_wave
)

