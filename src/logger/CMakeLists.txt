add_library(aegis_logger STATIC
    logger.cpp
)
target_include_directories(aegis_logger PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(aegis_logger PUBLIC aegis_common)
