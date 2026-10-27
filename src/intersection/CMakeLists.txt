add_library(aegis_intersection STATIC
    SignalController.cpp
    HeartbeatManager.cpp
    IntersectionNode.cpp
)

target_include_directories(aegis_intersection PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_intersection PUBLIC
    aegis_communication
    aegis_common
    aegis_logger
    aegis_config
)
