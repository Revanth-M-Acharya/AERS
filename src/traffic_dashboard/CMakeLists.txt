add_library(aegis_traffic_dashboard STATIC
    traffic_dashboard_backend.cpp
)
target_include_directories(aegis_traffic_dashboard PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(aegis_traffic_dashboard PUBLIC
    aegis_common
    aegis_network
    aegis_logger
)
