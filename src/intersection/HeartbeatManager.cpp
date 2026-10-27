/**
 * @file HeartbeatManager.cpp
 * @brief Implementation of HeartbeatManager.
 */

#include "HeartbeatManager.h"

namespace aegis {
namespace intersection {

HeartbeatManager::HeartbeatManager(std::string intersectionId, uint32_t intervalMs)
    : m_intersectionId(std::move(intersectionId))
    , m_intervalMs(intervalMs)
    , m_running(false)
    , m_heartbeatCount(0) {
}

HeartbeatManager::~HeartbeatManager() {
    Stop();
}

HeartbeatManager::HeartbeatManager(HeartbeatManager&& other) noexcept {
    Stop();
    other.Stop();

    std::lock_guard<std::mutex> lock(other.m_mutex);
    m_intersectionId = std::move(other.m_intersectionId);
    m_intervalMs.store(other.m_intervalMs.load());
    m_heartbeatCount.store(other.m_heartbeatCount.load());

    std::lock_guard<std::mutex> cbLock(other.m_callbackMutex);
    m_callback = std::move(other.m_callback);
}

HeartbeatManager& HeartbeatManager::operator=(HeartbeatManager&& other) noexcept {
    if (this != &other) {
        Stop();
        other.Stop();

        std::scoped_lock lock(m_mutex, other.m_mutex);
        m_intersectionId = std::move(other.m_intersectionId);
        m_intervalMs.store(other.m_intervalMs.load());
        m_heartbeatCount.store(other.m_heartbeatCount.load());

        std::scoped_lock cbLock(m_callbackMutex, other.m_callbackMutex);
        m_callback = std::move(other.m_callback);
    }
    return *this;
}

bool HeartbeatManager::Start() {
    if (m_running.exchange(true)) {
        return false; // Already executing
    }

    m_workerThread = std::thread(&HeartbeatManager::WorkerLoop, this);
    return true;
}

bool HeartbeatManager::Start(HeartbeatCallback callback) {
    SetCallback(std::move(callback));
    return Start();
}

void HeartbeatManager::Stop() {
    if (!m_running.exchange(false)) {
        return;
    }

    m_cv.notify_all();

    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

bool HeartbeatManager::IsRunning() const noexcept {
    return m_running.load();
}

communication::Message HeartbeatManager::GenerateHeartbeat() const {
    std::string idCopy;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        idCopy = m_intersectionId;
    }

    return communication::Message(communication::MessageType::HEARTBEAT,
                                  std::move(idCopy),
                                  "HEARTBEAT",
                                  communication::Message::CurrentTimestampMs());
}

communication::Message HeartbeatManager::GenerateHeartbeat(const std::string& payload) const {
    std::string idCopy;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        idCopy = m_intersectionId;
    }

    return communication::Message(communication::MessageType::HEARTBEAT,
                                  std::move(idCopy),
                                  payload,
                                  communication::Message::CurrentTimestampMs());
}

void HeartbeatManager::SetIntersectionId(std::string intersectionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_intersectionId = std::move(intersectionId);
}

std::string HeartbeatManager::GetIntersectionId() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_intersectionId;
}

void HeartbeatManager::SetIntervalMs(uint32_t intervalMs) {
    m_intervalMs.store(intervalMs);
    m_cv.notify_all(); // Wakeup to adapt to new interval
}

uint32_t HeartbeatManager::GetIntervalMs() const noexcept {
    return m_intervalMs.load();
}

void HeartbeatManager::SetCallback(HeartbeatCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_callback = std::move(callback);
}

uint64_t HeartbeatManager::GetHeartbeatCount() const noexcept {
    return m_heartbeatCount.load();
}

void HeartbeatManager::WorkerLoop() {
    while (m_running.load()) {
        std::unique_lock<std::mutex> lock(m_mutex);
        uint32_t interval = m_intervalMs.load();

        m_cv.wait_for(lock, std::chrono::milliseconds(interval), [this]() {
            return !m_running.load();
        });

        if (!m_running.load()) {
            break;
        }

        lock.unlock(); // Release mutex before callback invocation

        communication::Message msg = GenerateHeartbeat();
        m_heartbeatCount.fetch_add(1);

        HeartbeatCallback cb = nullptr;
        {
            std::lock_guard<std::mutex> cbLock(m_callbackMutex);
            cb = m_callback;
        }

        if (cb) {
            cb(msg);
        }
    }
}

} // namespace intersection
} // namespace aegis
