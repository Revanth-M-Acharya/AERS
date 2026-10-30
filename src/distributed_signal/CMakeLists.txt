add_library(aegis_distributed_signal STATIC
    DistributedSignalSynchronizer.cpp
)

target_include_directories(aegis_distributed_signal PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_distributed_signal PUBLIC
    aegis_green_wave
    aegis_green_corridor
    aegis_coordinator
    aegis_common
    aegis_logger
    aegis_config
)
