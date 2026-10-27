add_library(aegis_config STATIC
    config_manager.cpp
    NodeConfig.cpp
)
target_include_directories(aegis_config PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(aegis_config PUBLIC aegis_common)
