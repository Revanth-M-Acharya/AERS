# TrafficSignalNode Module CMake Configuration (Phase 6A)
add_library(aegis_traffic_signal STATIC
    TrafficSignalNode.cpp
)

target_include_directories(aegis_traffic_signal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_link_libraries(aegis_traffic_signal PUBLIC
    aegis_common
    aegis_logger
)
