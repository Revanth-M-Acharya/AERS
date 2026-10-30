add_library(aegis_predictive_traffic STATIC
    PredictiveTrafficEngine.cpp
)

target_include_directories(aegis_predictive_traffic PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_predictive_traffic PUBLIC
    aegis_replanning
    aegis_green_wave
    aegis_green_corridor
    aegis_common
    aegis_logger
    aegis_config
)
