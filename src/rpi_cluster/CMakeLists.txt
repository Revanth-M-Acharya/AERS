add_library(aegis_rpi_cluster STATIC
    RaspberryPiClusterManager.cpp
)

target_include_directories(aegis_rpi_cluster PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_rpi_cluster PUBLIC
    aegis_common
    aegis_logger
    aegis_config
)

