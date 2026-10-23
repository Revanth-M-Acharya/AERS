cmake_minimum_required(VERSION 3.20)
project(SMART_AEGIS VERSION 1.0.0 LANGUAGES C CXX)

# ==============================================================================
# Global Compilation Standards
# ==============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for language servers and static analysis
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Strict warning flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

# Build Options
option(BUILD_TESTS "Build automated test suites" ON)
option(BUILD_SIMULATORS "Build simulation and testbed tools" ON)
option(ENABLE_QNX_TARGET "Target QNX Neutrino RTOS" OFF)

# Include Directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Core Sources Subdirectory
add_subdirectory(src)

# Tools and Simulators Subdirectory
if(BUILD_SIMULATORS)
    add_subdirectory(tools/simulator)
endif()

# Test Suites Subdirectory
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
