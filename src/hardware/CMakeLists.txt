add_library(aegis_hardware STATIC
    HardwareConfiguration.cpp
    HardwareEventLogger.cpp
    RelayController.cpp
    TrafficLightController.cpp
    IntersectionHardware.cpp
    WirelessNodeManager.cpp
    HardwareHealthMonitor.cpp
    HardwareDiagnostics.cpp
    HardwareController.cpp
    OpticalSensorFilter.cpp
    GpioReservationManager.cpp
    HardwareWatchdog.cpp
    WirelessQualityMonitor.cpp
    BootManager.cpp
    ShutdownManager.cpp
)

target_include_directories(aegis_hardware PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_hardware PUBLIC
    aegis_common
    aegis_logger
    aegis_config
    aegis_hardware_abstraction
    aegis_qnx_driver_framework
    aegis_bsp
    aegis_runtime
    aegis_deployment
)

