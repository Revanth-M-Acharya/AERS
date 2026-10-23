add_library(aegis_network STATIC
    retry_policy.cpp
    serializer.cpp
    message_router.cpp
    authentication_manager.cpp
    connection_manager.cpp
    socket_client.cpp
    socket_server.cpp
    heartbeat_manager.cpp
    mqtt_client.cpp
    network_manager.cpp
)
target_include_directories(aegis_network PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(aegis_network PUBLIC aegis_common aegis_logger aegis_config)
