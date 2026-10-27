# System Integration Layer Library
add_library(aegis_system STATIC
    SystemManager.cpp
)

target_include_directories(aegis_system PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(aegis_system PUBLIC
    aegis_coordinator
    aegis_intersection
    aegis_communication
    aegis_common
    aegis_logger
    aegis_config
)
