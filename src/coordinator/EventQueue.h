/**
 * @file EventQueue.h
 * @brief Thread-safe event queue for the SMART AEGIS Coordinator Core.
 *
 * Facilitates asynchronous event decoupling between the network reception
 * layer and coordinator business logic dispatch workers.
 */

#ifndef AEGIS_COORDINATOR_EVENT_QUEUE_H
#define AEGIS_COORDINATOR_EVENT_QUEUE_H

#include "communication/Message.h"
#include <string>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstdint>
#include <atomic>
namespace aegis {
namespace coordinator {

/**
 * @brief Categorization of events managed by the Coordinator EventQueue.
 */
enum class CoordinatorEventType : uint8_t {
    MESSAGE_RECEIVED       = 1, ///< Inbound packet received from a connected node
    CLIENT_CONNECTED       = 2, ///< New client TCP connection established
    CLIENT_DISCONNECTED    = 3, ///< Client disconnected or dropped
    HEARTBEAT_TIMEOUT      = 4, ///< Intersection heartbeat expired
    SYSTEM_SHUTDOWN        = 5  ///< Orderly shutdown trigger
};

/**
 * @brief Convert CoordinatorEventType enum to human-readable string.
 */
const char* CoordinatorEventTypeToString(CoordinatorEventType type) noexcept;

/**
 * @brief Represents an individual event item stored in the EventQueue.
 */
struct CoordinatorEvent {
    CoordinatorEventType type{CoordinatorEventType::MESSAGE_RECEIVED};
    int clientId{-1};                                                   ///< Client session ID
    std::string sourceId{""};                                           ///< Node identifier (e.g. "INT_101")
    communication::Message message{};                                   ///< Associated message payload
    uint64_t timestamp{0};                                              ///< Milliseconds epoch
    std::string details{""};                                            ///< Additional diagnostic metadata

    CoordinatorEvent() = default;

    CoordinatorEvent(CoordinatorEventType evType,
                     int cId,
                     std::string srcId,
                     communication::Message msg,
                     uint64_t ts = 0,
                     std::string det = "")
        : type(evType)
        , clientId(cId)
        , sourceId(std::move(srcId))
        , message(std::move(msg))
        , timestamp(ts)
        , details(std::move(det)) {}
};

/**
 * @brief Thread-safe, blocking and timeout-bounded FIFO event queue.
 */
class EventQueue {
public:
    /**
     * @brief Construct an empty EventQueue.
     * @param maxCapacity Maximum allowed events before push blocking or dropping (0 = unbounded).
     */
    explicit EventQueue(size_t maxCapacity = 10000);

    /**
     * @brief Destructor unblocks waiting consumers and releases queue resources.
     */
    ~EventQueue();

    // Non-copyable, movable
    EventQueue(const EventQueue&) = delete;
    EventQueue& operator=(const EventQueue&) = delete;
    EventQueue(EventQueue&&) noexcept;
    EventQueue& operator=(EventQueue&&) noexcept;

    // =========================================================================
    // Core Queue Operations
    // =========================================================================

    /**
     * @brief Push an event onto the tail of the queue and notify waiting consumers.
     * @param event The event to append.
     */
    void PushEvent(const CoordinatorEvent& event);

    /**
     * @brief Push an event by move onto the tail of the queue.
     * @param event The event to append.
     */
    void PushEvent(CoordinatorEvent&& event);

    /**
     * @brief Pop the oldest event from the queue with an optional timeout.
     * @param outEvent Populated with the dequeued event.
     * @param timeoutMs Timeout in milliseconds (0 = non-blocking poll).
     * @return true if an event was popped; false on timeout or queue shutdown.
     */
    bool PopEvent(CoordinatorEvent& outEvent, uint32_t timeoutMs = 1000);

    /**
     * @brief Blocking pop that waits indefinitely until an event is available or shutdown.
     * @param outEvent Populated with the dequeued event.
     * @return true if an event was popped; false if queue was shut down.
     */
    bool PopEventBlocking(CoordinatorEvent& outEvent);

    /**
     * @brief Check whether the event queue is currently empty.
     * @return true if queue contains no events; false otherwise.
     */
    bool Empty() const;

    /**
     * @brief Get the current number of events pending in the queue.
     */
    size_t Size() const;

    /**
     * @brief Clear all pending events from the queue.
     */
    void Clear();

    /**
     * @brief Signal shutdown to unblock all threads waiting on PopEvent.
     */
    void Shutdown();

    /**
     * @brief Check whether queue shutdown has been signaled.
     */
    bool IsShutdown() const noexcept;

private:
    size_t m_maxCapacity{10000};
    mutable std::mutex m_mutex{};
    std::condition_variable m_cv{};
    std::deque<CoordinatorEvent> m_queue{};
    std::atomic<bool> m_shutdown{false};
};

} // namespace coordinator
} // namespace aegis

#endif // AEGIS_COORDINATOR_EVENT_QUEUE_H
