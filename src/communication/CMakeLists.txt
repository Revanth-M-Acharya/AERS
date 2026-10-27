add_library(aegis_communication STATIC
    Message.cpp
    SocketServer.cpp
    SocketClient.cpp
)

target_include_directories(aegis_communication PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)
