add_library(aegis_hospital_dashboard STATIC
    hospital_dashboard_backend.cpp
)
target_include_directories(aegis_hospital_dashboard PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(aegis_hospital_dashboard PUBLIC
    aegis_common
    aegis_network
    aegis_logger
)
