cmake_minimum_required(VERSION 3.16)
project(aegis_qnx_network CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wnon-virtual-dtor")

# Find threading library (QNX/POSIX native threads)
find_package(Threads REQUIRED)

# Networking library
add_library(aegis_qnx_network STATIC
    message.cpp
    tcp_server.cpp
    tcp_client.cpp
)

target_include_directories(aegis_qnx_network PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aegis_qnx_network PUBLIC
    Threads::Threads
)

# QNX Neutrino RTOS socket library if targeting QNX
if(QNXNTO)
    target_link_libraries(aegis_qnx_network PUBLIC socket)
endif()

# Example binaries
add_executable(example_server example_server.cpp)
target_link_libraries(example_server PRIVATE aegis_qnx_network)

add_executable(example_client example_client.cpp)
target_link_libraries(example_client PRIVATE aegis_qnx_network)

