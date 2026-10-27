add_library(aegis_logger STATIC
    logger.cpp
    EventLogger.cpp
    SimulationRecorder.cpp
)
target_include_directories(aegis_logger PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(aegis_logger PUBLIC aegis_common)
