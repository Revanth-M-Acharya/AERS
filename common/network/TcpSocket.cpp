/**
 * @file TcpSocket.cpp
 * @brief Implementation of POSIX / QNX BSD socket wrapper.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "common/network/TcpSocket.hpp"
#include "common/network/SocketUtils.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cstring>

namespace aegis::network {

TcpSocket::TcpSocket() : fd_(-1) {}

TcpSocket::TcpSocket(int fd) : fd_(fd) {}

TcpSocket::~TcpSocket() {
    Close();
}

TcpSocket::TcpSocket(TcpSocket&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept {
    if (this != &other) {
        Close();
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

bool TcpSocket::Create() {
    Close();
    fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    return fd_ >= 0;
}

bool TcpSocket::SetReuseAddress(bool enable) {
    if (fd_ < 0) return false;
    int opt = enable ? 1 : 0;
    return (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0);
}

bool TcpSocket::SetNonBlocking(bool enable) {
    if (fd_ < 0) return false;
    int flags = ::fcntl(fd_, F_GETFL, 0);
    if (flags < 0) return false;
    if (enable) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    return (::fcntl(fd_, F_SETFL, flags) == 0);
}

bool TcpSocket::SetNoDelay(bool enable) {
    if (fd_ < 0) return false;
    int opt = enable ? 1 : 0;
    return (::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) == 0);
}

bool TcpSocket::Bind(const std::string& ip, uint16_t port) {
    if (fd_ < 0) return false;

    struct sockaddr_in addr{};
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (ip.empty() || ip == "0.0.0.0") {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
            return false;
        }
    }

    return (::bind(fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == 0);
}

bool TcpSocket::Listen(int backlog) {
    if (fd_ < 0) return false;
    return (::listen(fd_, backlog) == 0);
}

int TcpSocket::Accept(std::string& clientIp, uint16_t& clientPort) {
    if (fd_ < 0) return -1;

    struct sockaddr_in clientAddr{};
    socklen_t addrLen = sizeof(clientAddr);
    int clientFd = ::accept(fd_, reinterpret_cast<struct sockaddr*>(&clientAddr), &addrLen);
    if (clientFd < 0) {
        return -1;
    }

    char ipStr[INET_ADDRSTRLEN]{};
    ::inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));
    clientIp = ipStr;
    clientPort = ntohs(clientAddr.sin_port);

    return clientFd;
}

bool TcpSocket::Connect(const std::string& ip, uint16_t port, uint32_t timeoutMs) {
    if (fd_ < 0) {
        if (!Create()) return false;
    }

    struct sockaddr_in addr{};
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        return false;
    }

    // Set non-blocking to honor timeout
    SetNonBlocking(true);

    int res = ::connect(fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    if (res == 0) {
        return true;
    }

    if (errno != EINPROGRESS && errno != EWOULDBLOCK) {
        return false;
    }

    struct pollfd pfd{};
    pfd.fd = fd_;
    pfd.events = POLLOUT;
    int pollRes = ::poll(&pfd, 1, static_cast<int>(timeoutMs));
    if (pollRes <= 0) {
        return false;
    }

    int soError = 0;
    socklen_t len = sizeof(soError);
    if (::getsockopt(fd_, SOL_SOCKET, SO_ERROR, &soError, &len) < 0 || soError != 0) {
        return false;
    }

    return true;
}

void TcpSocket::Close() {
    if (fd_ >= 0) {
        ::shutdown(fd_, SHUT_RDWR);
        ::close(fd_);
        fd_ = -1;
    }
}

bool TcpSocket::SendAll(const uint8_t* data, size_t length) {
    if (fd_ < 0 || data == nullptr) return false;

    size_t totalSent = 0;
    while (totalSent < length) {
        ssize_t sent = ::send(fd_, data + totalSent, length - totalSent, 0);
        if (sent < 0) {
            if (SocketUtils::IsWouldBlock(errno)) {
                // Wait briefly for socket to become writable
                struct pollfd pfd{};
                pfd.fd = fd_;
                pfd.events = POLLOUT;
                int ret = ::poll(&pfd, 1, 100);
                if (ret > 0) continue;
            }
            return false;
        }
        totalSent += static_cast<size_t>(sent);
    }
    return true;
}

bool TcpSocket::SendAll(const std::vector<uint8_t>& data) {
    return SendAll(data.data(), data.size());
}

ssize_t TcpSocket::Receive(uint8_t* buffer, size_t maxLen) {
    if (fd_ < 0 || buffer == nullptr || maxLen == 0) return -1;
    return ::recv(fd_, buffer, maxLen, 0);
}

} // namespace aegis::network

