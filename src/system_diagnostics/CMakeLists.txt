add_library(aegis_system_diagnostics STATIC
    SystemDiagnosticsEngine.cpp
)

target_include_directories(aegis_system_diagnostics PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_system_diagnostics PUBLIC
    aegis_common
    aegis_logger
    aegis_config
)

