#ifndef AEGIS_NETWORK_INTERNAL_SOCKET_LOOPBACK_HPP
#define AEGIS_NETWORK_INTERNAL_SOCKET_LOOPBACK_HPP

#include <string>
#include <queue>
#include <mutex>
#include <unordered_map>
#include <cstdint>

namespace aegis {
namespace network {
namespace internal {

struct DatagramPacket {
    std::string senderIp;
    uint16_t senderPort{0};
    std::string payload;
};

class DatagramLoopbackBus {
public:
    static DatagramLoopbackBus& Instance() {
        static DatagramLoopbackBus instance;
        return instance;
    }

    void Send(uint16_t destinationPort, const std::string& senderIp, uint16_t senderPort, const std::string& payload) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queues[destinationPort].push({senderIp, senderPort, payload});
    }

    bool Receive(uint16_t port, DatagramPacket& outPacket) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_queues.find(port);
        if (it != m_queues.end() && !it->second.empty()) {
            outPacket = it->second.front();
            it->second.pop();
            return true;
        }
        return false;
    }

    void Clear(uint16_t port) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queues.erase(port);
    }

private:
    DatagramLoopbackBus() = default;
    ~DatagramLoopbackBus() = default;
    DatagramLoopbackBus(const DatagramLoopbackBus&) = delete;
    DatagramLoopbackBus& operator=(const DatagramLoopbackBus&) = delete;

    std::mutex m_mutex;
    std::unordered_map<uint16_t, std::queue<DatagramPacket>> m_queues;
};

} // namespace internal
} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_INTERNAL_SOCKET_LOOPBACK_HPP
