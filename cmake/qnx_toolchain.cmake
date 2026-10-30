# ==============================================================================
# QNX Neutrino RTOS CMake Toolchain Configuration (QNX SDP 8.0 / 7.1)
# Target Architecture: aarch64le (ARM64 Little Endian for Raspberry Pi 4 Model B)
#
# Usage:
#   cmake -B build-qnx \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/qnx_toolchain.cmake \
#         -DQNX_TARGET_ARCH=aarch64le \
#         -DCMAKE_BUILD_TYPE=Debug|Release|Profile
# ==============================================================================

set(CMAKE_SYSTEM_NAME QNX)

# Detect QNX SDP version (Defaults to 8.0.0 for SMART AEGIS Phase 20.5)
if(NOT DEFINED CMAKE_SYSTEM_VERSION)
    if(DEFINED ENV{QNX_SDP_VERSION})
        set(CMAKE_SYSTEM_VERSION $ENV{QNX_SDP_VERSION})
    else()
        set(CMAKE_SYSTEM_VERSION 8.0.0)
    endif()
endif()

# Default architecture: ARMv8-A 64-bit for Raspberry Pi 4B (BCM2711 SoC)
if(NOT QNX_TARGET_ARCH)
    set(QNX_TARGET_ARCH aarch64le)
endif()

if(QNX_TARGET_ARCH STREQUAL "aarch64le")
    set(QNX_CPU "aarch64")
    set(CMAKE_SYSTEM_PROCESSOR aarch64)
    set(QCC_TARGET_SPEC "gcc_ntoaarch64le")
elseif(QNX_TARGET_ARCH STREQUAL "armle-v7")
    set(QNX_CPU "armv7")
    set(CMAKE_SYSTEM_PROCESSOR arm)
    set(QCC_TARGET_SPEC "gcc_ntoarmv7le")
elseif(QNX_TARGET_ARCH STREQUAL "x86_64")
    set(QNX_CPU "x86_64")
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
    set(QCC_TARGET_SPEC "gcc_ntox86_64")
else()
    message(FATAL_ERROR "Unsupported QNX target architecture: ${QNX_TARGET_ARCH}")
endif()

# Locate QNX Host & Target Environment
if(NOT DEFINED ENV{QNX_HOST} OR NOT DEFINED ENV{QNX_TARGET})
    message(WARNING "QNX_HOST or QNX_TARGET environment variables not set. Ensure QNX SDP environment is sourced: source <path-to-sdp>/qnxsdp-env.sh")
    if(NOT CMAKE_C_COMPILER)
        set(CMAKE_C_COMPILER qcc)
    endif()
    if(NOT CMAKE_CXX_COMPILER)
        set(CMAKE_CXX_COMPILER q++)
    endif()
else()
    set(CMAKE_C_COMPILER "$ENV{QNX_HOST}/usr/bin/qcc")
    set(CMAKE_CXX_COMPILER "$ENV{QNX_HOST}/usr/bin/q++")
    set(CMAKE_AR "$ENV{QNX_HOST}/usr/bin/nto${CMAKE_SYSTEM_PROCESSOR}-ar" CACHE FILEPATH "QNX ar")
    set(CMAKE_RANLIB "$ENV{QNX_HOST}/usr/bin/nto${CMAKE_SYSTEM_PROCESSOR}-ranlib" CACHE FILEPATH "QNX ranlib")
endif()

# Cross-compilation root path setup
if(DEFINED ENV{QNX_TARGET})
    set(CMAKE_SYSROOT $ENV{QNX_TARGET})
    set(CMAKE_FIND_ROOT_PATH $ENV{QNX_TARGET})
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
endif()

# Global QNX preprocessor definitions
add_definitions(
    -D__QNX__
    -D__QNXNTO__
    -DAEGIS_TARGET_QNX=1
    -D_QNX_SOURCE
)

# QCC/Q++ Compiler and Linker Flags
set(CMAKE_C_FLAGS_INIT "-V${QCC_TARGET_SPEC} -w8")
set(CMAKE_CXX_FLAGS_INIT "-V${QCC_TARGET_SPEC}_cpp -Y_gpp -lang-c++ -w8 -std=c++17")

# Build Configurations (Debug, Release, Profile)
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g -O0 -DAEGIS_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_PROFILE_INIT "-O2 -g -p -DPROFILE -DAEGIS_PROFILE")

# QNX Shared Library and Linker options
set(CMAKE_EXE_LINKER_FLAGS_INIT "-V${QCC_TARGET_SPEC}_cpp -lang-c++")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-V${QCC_TARGET_SPEC}_cpp -lang-c++")

message(STATUS "Configured QNX SDP ${CMAKE_SYSTEM_VERSION} Toolchain for ${QNX_TARGET_ARCH} (${QCC_TARGET_SPEC})")
