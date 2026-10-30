/**
 * @file QnxMessagePassing.cpp
 * @brief Implementation of Phase 4 Native QNX Message Passing IPC.
 */

#include "aegis/qnx/QnxMessagePassing.hpp"
#include "aegis/logger/logger.hpp"

#include <iostream>
#include <queue>
#include <chrono>

#if !defined(__QNX__) && !defined(__QNXNTO__)

namespace aegis {
namespace qnx {

struct SimulatedRequest {
    int rcvid;
    QnxMsgHeader header;
    std::vector<uint8_t> payload;
    bool replied{false};
    QnxReplyMsg reply;
    std::condition_variable replyCv;
};

struct SimulatedChannel {
    int chid{1};
    std::string name;
    std::mutex mutex;
    std::condition_variable cv;
    std::queue<std::shared_ptr<SimulatedRequest>> queue;
    int nextRcvid{1};
    std::unordered_map<int, std::shared_ptr<SimulatedRequest>> pendingRequests;
    bool destroyed{false};
};

static std::mutex g_channelRegistryMutex;
static std::unordered_map<int, std::shared_ptr<SimulatedChannel>> g_channelRegistry;
static std::unordered_map<std::string, std::shared_ptr<SimulatedChannel>> g_channelNameRegistry;
static int g_nextChid = 100;

} // namespace qnx
} // namespace aegis

#endif // !defined(__QNX__)

namespace aegis {
namespace qnx {

// =============================================================================
// QnxChannelServer Implementation
// =============================================================================

QnxChannelServer::QnxChannelServer(const std::string& serverName)
    : m_serverName(serverName) {}

QnxChannelServer::~QnxChannelServer() {
    DestroyChannel();
}

int QnxChannelServer::CreateChannel() {
#if defined(__QNX__) || defined(__QNXNTO__)
    m_chid = ChannelCreate(0);
    return m_chid;
#else
    std::lock_guard<std::mutex> lock(g_channelRegistryMutex);
    m_chid = g_nextChid++;
    m_simChannel = std::make_shared<SimulatedChannel>();
    m_simChannel->chid = m_chid;
    m_simChannel->name = m_serverName;
    m_simChannel->destroyed = false;

    g_channelRegistry[m_chid] = m_simChannel;
    if (!m_serverName.empty()) {
        g_channelNameRegistry[m_serverName] = m_simChannel;
    }
    return m_chid;
#endif
}

void QnxChannelServer::DestroyChannel() {
    if (m_chid < 0) return;

#if defined(__QNX__) || defined(__QNXNTO__)
    ChannelDestroy(m_chid);
    m_chid = -1;
#else
    std::lock_guard<std::mutex> lock(g_channelRegistryMutex);
    if (m_simChannel) {
        {
            std::lock_guard<std::mutex> chLock(m_simChannel->mutex);
            m_simChannel->destroyed = true;
            m_simChannel->cv.notify_all();
        }
        g_channelRegistry.erase(m_chid);
        if (!m_serverName.empty()) {
            g_channelNameRegistry.erase(m_serverName);
        }
        m_simChannel.reset();
    }
    m_chid = -1;
#endif
}

int QnxChannelServer::Receive(QnxMsgHeader& outHeader, void* outPayload, size_t maxPayloadBytes, int& outRcvid) {
    if (m_chid < 0) return -1;

#if defined(__QNX__) || defined(__QNXNTO__)
    QnxEnvelope env;
    int rcvid = MsgReceive(m_chid, &env, sizeof(env), nullptr);
    if (rcvid < 0) return -1;

    outRcvid = rcvid;
    outHeader = env.header;
    size_t copyBytes = (env.header.payloadSize < maxPayloadBytes) ? env.header.payloadSize : maxPayloadBytes;
    if (copyBytes > 0 && outPayload != nullptr) {
        std::memcpy(outPayload, env.payload, copyBytes);
    }
    m_msgCount++;
    return static_cast<int>(copyBytes);
#else
    if (!m_simChannel) return -1;

    std::unique_lock<std::mutex> lock(m_simChannel->mutex);
    while (m_simChannel->queue.empty() && !m_simChannel->destroyed) {
        m_simChannel->cv.wait(lock);
    }

    if (m_simChannel->destroyed || m_simChannel->queue.empty()) {
        return -1;
    }

    auto req = m_simChannel->queue.front();
    m_simChannel->queue.pop();

    outRcvid = req->rcvid;
    outHeader = req->header;
    size_t copyBytes = (req->payload.size() < maxPayloadBytes) ? req->payload.size() : maxPayloadBytes;
    if (copyBytes > 0 && outPayload != nullptr) {
        std::memcpy(outPayload, req->payload.data(), copyBytes);
    }

    m_simChannel->pendingRequests[req->rcvid] = req;
    m_msgCount++;
    return static_cast<int>(copyBytes);
#endif
}

int QnxChannelServer::Reply(int rcvid, const QnxReplyMsg& reply) {
    if (m_chid < 0 || rcvid <= 0) return -1;

#if defined(__QNX__) || defined(__QNXNTO__)
    return MsgReply(rcvid, reply.status, &reply, sizeof(reply));
#else
    if (!m_simChannel) return -1;

    std::shared_ptr<SimulatedRequest> req;
    {
        std::lock_guard<std::mutex> lock(m_simChannel->mutex);
        auto it = m_simChannel->pendingRequests.find(rcvid);
        if (it == m_simChannel->pendingRequests.end()) {
            return -1;
        }
        req = it->second;
        m_simChannel->pendingRequests.erase(it);
    }

    {
        std::lock_guard<std::mutex> lock(m_simChannel->mutex);
        req->reply = reply;
        req->replied = true;
        req->replyCv.notify_one();
    }
    return 0;
#endif
}

// =============================================================================
// QnxChannelClient Implementation
// =============================================================================

QnxChannelClient::QnxChannelClient(const std::string& clientName)
    : m_clientName(clientName) {}

QnxChannelClient::~QnxChannelClient() {
    Detach();
}

int QnxChannelClient::Attach(int targetChid, const std::string& serverName) {
#if defined(__QNX__) || defined(__QNXNTO__)
    m_targetChid = targetChid;
    m_coid = ConnectAttach(0, 0, targetChid, _NTO_SIDE_CHANNEL, 0);
    return m_coid;
#else
    std::lock_guard<std::mutex> lock(g_channelRegistryMutex);
    if (targetChid > 0) {
        auto it = g_channelRegistry.find(targetChid);
        if (it != g_channelRegistry.end()) {
            m_simChannel = it->second;
            m_targetChid = targetChid;
            m_coid = targetChid * 10;
            return m_coid;
        }
    }

    if (!serverName.empty()) {
        auto it = g_channelNameRegistry.find(serverName);
        if (it != g_channelNameRegistry.end()) {
            m_simChannel = it->second;
            m_targetChid = m_simChannel->chid;
            m_coid = m_targetChid * 10;
            return m_coid;
        }
    }
    return -1;
#endif
}

void QnxChannelClient::Detach() {
    if (m_coid < 0) return;

#if defined(__QNX__) || defined(__QNXNTO__)
    ConnectDetach(m_coid);
    m_coid = -1;
#else
    m_simChannel.reset();
    m_coid = -1;
#endif
}

int QnxChannelClient::Send(const QnxMsgHeader& header, const void* payload, size_t payloadSize, QnxReplyMsg& outReply) {
    if (m_coid < 0) return -1;

    uint64_t startUs = aegis::common::GetMonotonicMicroseconds();

#if defined(__QNX__) || defined(__QNXNTO__)
    QnxEnvelope env;
    env.header = header;
    env.header.payloadSize = static_cast<uint32_t>(payloadSize);
    if (payload != nullptr && payloadSize > 0) {
        size_t cpy = (payloadSize < sizeof(env.payload)) ? payloadSize : sizeof(env.payload);
        std::memcpy(env.payload, payload, cpy);
    }

    size_t sendBytes = sizeof(QnxMsgHeader) + env.header.payloadSize;
    int ret = MsgSend(m_coid, &env, sendBytes, &outReply, sizeof(outReply));

    uint64_t endUs = aegis::common::GetMonotonicMicroseconds();
    m_lastLatencyUs = static_cast<double>(endUs >= startUs ? endUs - startUs : 0);
    m_sendCount++;
    return ret;
#else
    if (!m_simChannel) return -1;

    auto req = std::make_shared<SimulatedRequest>();
    req->header = header;
    req->header.payloadSize = static_cast<uint32_t>(payloadSize);
    if (payload != nullptr && payloadSize > 0) {
        const uint8_t* bytePtr = static_cast<const uint8_t*>(payload);
        req->payload.assign(bytePtr, bytePtr + payloadSize);
    }
    req->replied = false;

    {
        std::lock_guard<std::mutex> lock(m_simChannel->mutex);
        if (m_simChannel->destroyed) return -1;
        req->rcvid = m_simChannel->nextRcvid++;
        m_simChannel->queue.push(req);
        m_simChannel->cv.notify_one();
    }

    // Block until MsgReply() is called by the server
    {
        std::unique_lock<std::mutex> lock(m_simChannel->mutex);
        while (!req->replied && !m_simChannel->destroyed) {
            req->replyCv.wait(lock);
        }

        if (!req->replied) {
            return -1; // Server destroyed or failed to reply
        }
        outReply = req->reply;
    }

    uint64_t endUs = aegis::common::GetMonotonicMicroseconds();
    m_lastLatencyUs = static_cast<double>(endUs >= startUs ? endUs - startUs : 0);
    m_sendCount++;
    return outReply.status;
#endif
}

} // namespace qnx
} // namespace aegis

