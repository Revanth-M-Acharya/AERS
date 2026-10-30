add_library(aegis_deployment STATIC
    NodeConfiguration.cpp
    ConfigurationLoader.cpp
    DeploymentValidator.cpp
    RuntimeConfiguration.cpp
    NetworkDiscovery.cpp
    BootManager.cpp
    StartupManager.cpp
    DeploymentGpioDriver.cpp
    DeploymentLogger.cpp
    HeartbeatSupervisor.cpp
    GracefulShutdownManager.cpp
    DeploymentManager.cpp
    ClusterLauncher.cpp
)

target_include_directories(aegis_deployment PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/network
)

target_link_libraries(aegis_deployment PUBLIC
    aegis_common
    aegis_logger
    aegis_config
    aegis_safety
    aegis_runtime
    aegis_bsp
    aegis_hardware_abstraction
    aegis_qnx_driver_framework
    aegis_qnx_network
    Threads::Threads
)
