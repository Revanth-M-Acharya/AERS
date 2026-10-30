add_library(aegis_bsp STATIC
    RaspberryPi4BSP.cpp
)

target_include_directories(aegis_bsp PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_bsp PUBLIC
    aegis_common
    aegis_logger
    aegis_config
    aegis_hardware_abstraction
    aegis_qnx_driver_framework
)

