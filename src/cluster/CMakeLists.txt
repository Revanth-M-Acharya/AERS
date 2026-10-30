add_library(aegis_cluster STATIC
    ClusterHealthMonitor.cpp
)

target_include_directories(aegis_cluster PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_cluster PUBLIC
    aegis_common
    aegis_logger
    aegis_hardware
)

