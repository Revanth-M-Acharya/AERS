/**
 * @file SocketUtils.hpp
 * @brief Common socket helper utilities for QNX Neutrino and POSIX networking.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <cstdint>
#include <cstring>
#include <cerrno>

namespace aegis::network {

class SocketUtils {
public:
    static std::string GetLastSocketErrorString() {
        return std::strerror(errno);
    }

    static int GetLastSocketError() {
        return errno;
    }

    static bool IsWouldBlock(int err) {
        return (err == EAGAIN || err == EWOULDBLOCK);
    }
};

} // namespace aegis::network

