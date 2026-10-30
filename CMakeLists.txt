cmake_minimum_required(VERSION 3.20)
project(SMART_AEGIS 
    VERSION 1.0.0 
    DESCRIPTION "SMART AEGIS - Distributed Ambulance Emergency Response System"
    LANGUAGES C CXX
)

# ==============================================================================
# 1. C++ Standard & Global Compilation Flags
# ==============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build Type Default (Supports Debug, Release, Profile)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose Debug, Release, or Profile build" FORCE)
endif()

message(STATUS "SMART AEGIS - Build Configuration: ${CMAKE_BUILD_TYPE}")
message(STATUS "Target System: ${CMAKE_SYSTEM_NAME}")

# ==============================================================================
# 2. Strict Compiler Diagnostic Options
# ==============================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g3 -O0 -DDEBUG -DAEGIS_DEBUG)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Profile")
        add_compile_options(-O2 -g3 -DPROFILE -DAEGIS_PROFILE)
    else()
        add_compile_options(-O3 -DNDEBUG)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "QCC")
    # QNX Neutrino Compiler Options (SDP 8.0 / aarch64le default)
    if(NOT DEFINED QCC_TARGET_SPEC)
        set(QCC_TARGET_SPEC "gcc_ntoaarch64le")
    endif()
    add_compile_options(-V${QCC_TARGET_SPEC}_cpp -Y_gpp -lang-c++ -w8)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0 -DAEGIS_DEBUG)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Profile")
        add_compile_options(-O2 -g -p -DPROFILE -DAEGIS_PROFILE)
    else()
        add_compile_options(-O3 -DNDEBUG)
    endif()
endif()

# ==============================================================================
# 3. Platform & Target Options
# ==============================================================================
option(BUILD_TESTS "Build automated test suite" ON)
option(BUILD_SIMULATORS "Build simulation tools" ON)
option(ENABLE_QNX_TARGET "Target QNX Neutrino RTOS environment" OFF)

if(ENABLE_QNX_TARGET OR CMAKE_SYSTEM_NAME STREQUAL "QNX")
    add_compile_definitions(AEGIS_TARGET_QNX=1 __QNX__=1 __QNXNTO__=1 _QNX_SOURCE=1)
    message(STATUS "Configured for QNX Neutrino RTOS target environment")
else()
    add_compile_definitions(AEGIS_TARGET_POSIX=1)
    message(STATUS "Configured for POSIX / Linux / macOS host environment")
endif()

# ==============================================================================
# 4. Global Include Directories
# ==============================================================================
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ==============================================================================
# 5. Core Source Tree Subdirectories
# ==============================================================================
add_subdirectory(network)
add_subdirectory(src)

# ==============================================================================
# 6. Simulator Tools
# ==============================================================================
if(BUILD_SIMULATORS)
    add_subdirectory(tools/simulator)
endif()

# ==============================================================================
# 7. Automated Test Suites
# ==============================================================================
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
