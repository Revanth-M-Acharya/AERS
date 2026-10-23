add_library(aegis_network STATIC
    network_manager.cpp
    mqtt_client.cpp
)
target_include_directories(aegis_network PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(aegis_network PUBLIC aegis_common aegis_logger)
