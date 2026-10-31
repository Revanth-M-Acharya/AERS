/**
 * @file TcpSocket.hpp
 * @brief High-performance RAII IPv4 TCP Socket wrapper for QNX Neutrino and POSIX.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <cstdint>
#include <vector>

namespace aegis::network {

class TcpSocket {
public:
    TcpSocket();
    explicit TcpSocket(int fd);
    ~TcpSocket();

    // Prevent copying to ensure unique ownership of file descriptor
    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;

    // Enable move semantics
    TcpSocket(TcpSocket&& other) noexcept;
    TcpSocket& operator=(TcpSocket&& other) noexcept;

    // Socket Lifecycle Management
    bool Create();
    bool SetReuseAddress(bool enable);
    bool SetNonBlocking(bool enable);
    bool SetNoDelay(bool enable);
    bool Bind(const std::string& ip, uint16_t port);
    bool Listen(int backlog = 16);
    int Accept(std::string& clientIp, uint16_t& clientPort);
    bool Connect(const std::string& ip, uint16_t port, uint32_t timeoutMs = 3000);
    void Close();

    // Data Transfer
    bool SendAll(const uint8_t* data, size_t length);
    bool SendAll(const std::vector<uint8_t>& data);
    ssize_t Receive(uint8_t* buffer, size_t maxLen);

    // Queries
    bool IsValid() const { return fd_ >= 0; }
    int GetFd() const { return fd_; }

private:
    int fd_{-1};
};

} // namespace aegis::network

