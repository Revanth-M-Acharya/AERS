add_library(aegis_hardware_abstraction STATIC
    HardwareAbstractionLayer.cpp
)

target_include_directories(aegis_hardware_abstraction PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_hardware_abstraction PUBLIC
    aegis_common
    aegis_logger
    aegis_config
)

