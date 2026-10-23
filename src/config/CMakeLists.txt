add_library(aegis_config STATIC
    config_manager.cpp
)
target_include_directories(aegis_config PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(aegis_config PUBLIC aegis_common)
