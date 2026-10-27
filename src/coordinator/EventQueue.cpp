/**
 * @file EventQueue.cpp
 * @brief Implementation of EventQueue thread-safe queue.
 */

#include "EventQueue.h"

namespace aegis {
namespace coordinator {

const char* CoordinatorEventTypeToString(CoordinatorEventType type) noexcept {
    switch (type) {
        case CoordinatorEventType::MESSAGE_RECEIVED:    return "MESSAGE_RECEIVED";
        case CoordinatorEventType::CLIENT_CONNECTED:    return "CLIENT_CONNECTED";
        case CoordinatorEventType::CLIENT_DISCONNECTED: return "CLIENT_DISCONNECTED";
        case CoordinatorEventType::HEARTBEAT_TIMEOUT:   return "HEARTBEAT_TIMEOUT";
        case CoordinatorEventType::SYSTEM_SHUTDOWN:     return "SYSTEM_SHUTDOWN";
        default:                                        return "UNKNOWN";
    }
}

EventQueue::EventQueue(size_t maxCapacity)
    : m_maxCapacity(maxCapacity)
    , m_shutdown(false) {
}

EventQueue::~EventQueue() {
    Shutdown();
}

EventQueue::EventQueue(EventQueue&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.m_mutex);
    m_maxCapacity = other.m_maxCapacity;
    m_queue = std::move(other.m_queue);
    m_shutdown.store(other.m_shutdown.load());
}

EventQueue& EventQueue::operator=(EventQueue&& other) noexcept {
    if (this != &other) {
        std::scoped_lock lock(m_mutex, other.m_mutex);
        m_maxCapacity = other.m_maxCapacity;
        m_queue = std::move(other.m_queue);
        m_shutdown.store(other.m_shutdown.load());
    }
    return *this;
}

void EventQueue::PushEvent(const CoordinatorEvent& event) {
    if (m_shutdown.load()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_maxCapacity > 0 && m_queue.size() >= m_maxCapacity) {
            m_queue.pop_front();
        }
        m_queue.push_back(event);
    }
    m_cv.notify_one();
}

void EventQueue::PushEvent(CoordinatorEvent&& event) {
    if (m_shutdown.load()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_maxCapacity > 0 && m_queue.size() >= m_maxCapacity) {
            m_queue.pop_front();
        }
        m_queue.push_back(std::move(event));
    }
    m_cv.notify_one();
}

bool EventQueue::PopEvent(CoordinatorEvent& outEvent, uint32_t timeoutMs) {
    std::unique_lock<std::mutex> lock(m_mutex);

    if (timeoutMs > 0) {
        m_cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]() {
            return !m_queue.empty() || m_shutdown.load();
        });
    }

    if (!m_queue.empty()) {
        outEvent = std::move(m_queue.front());
        m_queue.pop_front();
        return true;
    }

    return false;
}

bool EventQueue::PopEventBlocking(CoordinatorEvent& outEvent) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [this]() {
        return !m_queue.empty() || m_shutdown.load();
    });

    if (!m_queue.empty()) {
        outEvent = std::move(m_queue.front());
        m_queue.pop_front();
        return true;
    }

    return false;
}

bool EventQueue::Empty() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.empty();
}

size_t EventQueue::Size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

void EventQueue::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.clear();
}

void EventQueue::Shutdown() {
    m_shutdown.store(true);
    m_cv.notify_all();
}

bool EventQueue::IsShutdown() const noexcept {
    return m_shutdown.load();
}

} // namespace coordinator
} // namespace aegis
