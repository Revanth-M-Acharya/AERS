add_library(aegis_safety STATIC
    safety_monitor.cpp
    watchdog.cpp
)
target_include_directories(aegis_safety PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(aegis_safety PUBLIC aegis_common aegis_logger)
