add_library(aegis_distributed_consensus STATIC
    DistributedConsensusEngine.cpp
)

target_include_directories(aegis_distributed_consensus PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_distributed_consensus PUBLIC
    aegis_common
    aegis_logger
    aegis_config
)

