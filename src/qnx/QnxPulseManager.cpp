/**
 * @file QnxPulseManager.cpp
 * @brief Implementation of Phase 4 Native QNX Pulse Management.
 */

#include "aegis/qnx/QnxPulseManager.hpp"
#include "aegis/common/time_utils.hpp"
#include "aegis/logger/logger.hpp"

#include <queue>
#include <condition_variable>
#include <chrono>

#if !defined(__QNX__) && !defined(__QNXNTO__)

namespace {

struct SimulatedPulseQueue {
    std::mutex mtx;
    std::condition_variable cv;
    std::queue<aegis::qnx::QnxPulseEvent> queue;
    bool active{true};
};

static std::mutex g_pulseRegistryMtx;
static std::unordered_map<int, std::shared_ptr<SimulatedPulseQueue>> g_pulseQueues;
static aegis::qnx::PulseStats g_globalPulseStats;
static std::mutex g_globalStatsMtx;

} // anonymous namespace

#endif // !defined(__QNX__)

namespace aegis {
namespace qnx {

QnxPulseManager::QnxPulseManager() = default;

QnxPulseManager::~QnxPulseManager() {
    StopDispatcher();
}

void QnxPulseManager::RegisterSimulatedChannel(int chid) {
#if !defined(__QNX__) && !defined(__QNXNTO__)
    std::lock_guard<std::mutex> lock(g_pulseRegistryMtx);
    if (g_pulseQueues.find(chid) == g_pulseQueues.end()) {
        auto q = std::make_shared<SimulatedPulseQueue>();
        g_pulseQueues[chid] = q;
    }
#else
    (void)chid;
#endif
}

void QnxPulseManager::UnregisterSimulatedChannel(int chid) {
#if !defined(__QNX__) && !defined(__QNXNTO__)
    std::lock_guard<std::mutex> lock(g_pulseRegistryMtx);
    auto it = g_pulseQueues.find(chid);
    if (it != g_pulseQueues.end()) {
        {
            std::lock_guard<std::mutex> qLock(it->second->mtx);
            it->second->active = false;
            it->second->cv.notify_all();
        }
        g_pulseQueues.erase(it);
    }
#else
    (void)chid;
#endif
}

int QnxPulseManager::SendPulse(int coid, int priority, int8_t code, int32_t value) {
#if defined(__QNX__) || defined(__QNXNTO__)
    int rc = MsgSendPulse(coid, priority, code, value);
    return rc;
#else
    std::shared_ptr<SimulatedPulseQueue> targetQueue;
    {
        std::lock_guard<std::mutex> lock(g_pulseRegistryMtx);
        auto it = g_pulseQueues.find(coid);
        if (it != g_pulseQueues.end()) {
            targetQueue = it->second;
        } else {
            // Auto-create queue for this connection ID in simulation
            targetQueue = std::make_shared<SimulatedPulseQueue>();
            g_pulseQueues[coid] = targetQueue;
        }
    }

    if (!targetQueue) {
        return -1;
    }

    QnxPulseEvent evt;
    evt.code = code;
    evt.value = value;
    evt.priority = priority;
    evt.timestampUs = aegis::common::GetMonotonicMicroseconds();
    evt.scoid = coid;

    {
        std::lock_guard<std::mutex> qLock(targetQueue->mtx);
        if (!targetQueue->active) {
            return -1;
        }
        targetQueue->queue.push(evt);
        targetQueue->cv.notify_one();
    }

    {
        std::lock_guard<std::mutex> sLock(g_globalStatsMtx);
        g_globalPulseStats.totalSent++;
    }

    return 0;
#endif
}

int QnxPulseManager::ReceivePulse(int chid, QnxPulseEvent& outPulse, int timeoutMs) {
#if defined(__QNX__) || defined(__QNXNTO__)
    QnxRawPulse raw;
    int rc;
    if (timeoutMs < 0) {
        rc = MsgReceivePulse(chid, &raw, sizeof(raw), nullptr);
    } else {
        struct timespec timeout;
        timeout.tv_sec = timeoutMs / 1000;
        timeout.tv_nsec = (timeoutMs % 1000) * 1000000L;
        // TimerTimeout with _NTO_TIMEOUT_RECEIVE
        uint64_t to_ns = static_cast<uint64_t>(timeoutMs) * 1000000ULL;
        TimerTimeout(CLOCK_MONOTONIC, _NTO_TIMEOUT_RECEIVE, nullptr, &to_ns, nullptr);
        rc = MsgReceivePulse(chid, &raw, sizeof(raw), nullptr);
    }

    if (rc < 0) {
        return -1;
    }

    outPulse.code = raw.code;
    outPulse.value = raw.value;
    outPulse.priority = 0;
    outPulse.scoid = raw.scoid;
    outPulse.timestampUs = aegis::common::GetMonotonicMicroseconds();
    return 0;
#else
    std::shared_ptr<SimulatedPulseQueue> queue;
    {
        std::lock_guard<std::mutex> lock(g_pulseRegistryMtx);
        auto it = g_pulseQueues.find(chid);
        if (it != g_pulseQueues.end()) {
            queue = it->second;
        } else {
            queue = std::make_shared<SimulatedPulseQueue>();
            g_pulseQueues[chid] = queue;
        }
    }

    std::unique_lock<std::mutex> qLock(queue->mtx);
    if (queue->queue.empty()) {
        if (timeoutMs == 0) {
            return -1;
        } else if (timeoutMs > 0) {
            auto status = queue->cv.wait_for(qLock, std::chrono::milliseconds(timeoutMs), [&]() {
                return !queue->queue.empty() || !queue->active;
            });
            if (!status || !queue->active || queue->queue.empty()) {
                return -1;
            }
        } else {
            queue->cv.wait(qLock, [&]() {
                return !queue->queue.empty() || !queue->active;
            });
            if (!queue->active && queue->queue.empty()) {
                return -1;
            }
        }
    }

    if (queue->queue.empty()) {
        return -1;
    }

    outPulse = queue->queue.front();
    queue->queue.pop();

    uint64_t nowUs = aegis::common::GetMonotonicMicroseconds();
    double latency = (nowUs > outPulse.timestampUs) ? static_cast<double>(nowUs - outPulse.timestampUs) : 0.0;

    {
        std::lock_guard<std::mutex> sLock(g_globalStatsMtx);
        g_globalPulseStats.totalReceived++;
        g_globalPulseStats.countPerCode[outPulse.code]++;
        if (g_globalPulseStats.totalReceived == 1) {
            g_globalPulseStats.avgLatencyUs = latency;
        } else {
            g_globalPulseStats.avgLatencyUs = (g_globalPulseStats.avgLatencyUs * 0.9) + (latency * 0.1);
        }
    }

    return 0;
#endif
}

void QnxPulseManager::RegisterHandler(int8_t code, PulseHandler handler) {
    std::lock_guard<std::mutex> lock(m_handlersMutex);
    m_handlers[code] = std::move(handler);
}

void QnxPulseManager::UnregisterHandler(int8_t code) {
    std::lock_guard<std::mutex> lock(m_handlersMutex);
    m_handlers.erase(code);
}

bool QnxPulseManager::StartDispatcher(int chid) {
    if (m_running.load()) {
        return false;
    }

    RegisterSimulatedChannel(chid);
    m_running.store(true);
    m_dispatcherThread = std::thread(&QnxPulseManager::DispatcherThreadFunc, this, chid);
    return true;
}

void QnxPulseManager::StopDispatcher() {
    if (!m_running.load()) {
        return;
    }

    m_running.store(false);

    // Unregister channel to unblock any waiting thread
    // Or send a SHUTDOWN pulse to unblock
    SendPulse(100, 30, PulseCode::SHUTDOWN, 0);

#if !defined(__QNX__) && !defined(__QNXNTO__)
    std::lock_guard<std::mutex> lock(g_pulseRegistryMtx);
    for (auto& pair : g_pulseQueues) {
        std::lock_guard<std::mutex> qLock(pair.second->mtx);
        pair.second->active = false;
        pair.second->cv.notify_all();
    }
#endif

    if (m_dispatcherThread.joinable()) {
        m_dispatcherThread.join();
    }
}

void QnxPulseManager::DispatcherThreadFunc(int chid) {
    while (m_running.load()) {
        QnxPulseEvent evt;
        int rc = ReceivePulse(chid, evt, 100);
        if (rc == 0) {
            if (evt.code == PulseCode::SHUTDOWN) {
                break;
            }

            PulseHandler handler = nullptr;
            {
                std::lock_guard<std::mutex> lock(m_handlersMutex);
                auto it = m_handlers.find(evt.code);
                if (it != m_handlers.end()) {
                    handler = it->second;
                }
            }

            if (handler) {
                handler(evt);
            }

            {
                std::lock_guard<std::mutex> sLock(m_statsMutex);
                m_stats.totalReceived++;
                m_stats.countPerCode[evt.code]++;
            }
        }
    }
}

PulseStats QnxPulseManager::GetStats() const {
#if !defined(__QNX__) && !defined(__QNXNTO__)
    std::lock_guard<std::mutex> lock(g_globalStatsMtx);
    return g_globalPulseStats;
#else
    std::lock_guard<std::mutex> lock(m_statsMutex);
    return m_stats;
#endif
}

void QnxPulseManager::ResetStats() {
#if !defined(__QNX__) && !defined(__QNXNTO__)
    {
        std::lock_guard<std::mutex> gLock(g_globalStatsMtx);
        g_globalPulseStats = PulseStats{};
    }
#endif
    std::lock_guard<std::mutex> lock(m_statsMutex);
    m_stats = PulseStats{};
}

} // namespace qnx
} // namespace aegis
