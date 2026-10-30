add_library(aegis_qnx_driver_framework STATIC
    QNXDeviceDriverFramework.cpp
)

target_include_directories(aegis_qnx_driver_framework PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_qnx_driver_framework PUBLIC
    aegis_common
    aegis_logger
    aegis_config
    aegis_hardware_abstraction
)

