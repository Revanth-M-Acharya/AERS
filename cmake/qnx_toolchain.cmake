# ==============================================================================
# QNX Neutrino RTOS CMake Toolchain Configuration
# Usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/qnx_toolchain.cmake -DQNX_TARGET_ARCH=aarch64le ..
# ==============================================================================

set(CMAKE_SYSTEM_NAME QNX)
set(CMAKE_SYSTEM_VERSION 7.1.0)

# Default architecture: ARMv8 64-bit for Raspberry Pi 4B/5 QNX BSP
if(NOT QNX_TARGET_ARCH)
    set(QNX_TARGET_ARCH aarch64le)
endif()

if(QNX_TARGET_ARCH STREQUAL "aarch64le")
    set(QNX_CPU "aarch64")
    set(CMAKE_SYSTEM_PROCESSOR aarch64)
elseif(QNX_TARGET_ARCH STREQUAL "armle-v7")
    set(QNX_CPU "armv7")
    set(CMAKE_SYSTEM_PROCESSOR arm)
elseif(QNX_TARGET_ARCH STREQUAL "x86_64")
    set(QNX_CPU "x86_64")
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
endif()

# Locate QNX Host & Target Environment
if(NOT DEFINED ENV{QNX_HOST} OR NOT DEFINED ENV{QNX_TARGET})
    message(WARNING "QNX_HOST or QNX_TARGET environment variables not detected.")
endif()

set(CMAKE_C_COMPILER qcc)
set(CMAKE_CXX_COMPILER q++ -Vgcc_ntoaarch64le)

set(CMAKE_FIND_ROOT_PATH $ENV{QNX_TARGET})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

add_definitions(-D__QNX__ -D__QNXNTO__)
