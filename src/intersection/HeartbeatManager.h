/**
 * @file HeartbeatManager.h
 * @brief Autonomous, non-blocking heartbeat periodic transmitter.
 *
 * Emits keepalive heartbeat beacons to the Central Coordinator using
 * communication::Message at a configurable interval.
 */

#ifndef AEGIS_INTERSECTION_HEARTBEAT_MANAGER_H
#define AEGIS_INTERSECTION_HEARTBEAT_MANAGER_H

#include "communication/Message.h"
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <cstdint>

namespace aegis {
namespace intersection {

/**
 * @brief Thread-safe periodic heartbeat manager for intersection node telemetry.
 */
class HeartbeatManager {
public:
    using HeartbeatCallback = std::function<void(const communication::Message& message)>;

    /**
     * @brief Construct an unstarted HeartbeatManager.
     * @param intersectionId Identifying string for this roadside node.
     * @param intervalMs Heartbeat interval in milliseconds (default: 1000).
     */
    explicit HeartbeatManager(std::string intersectionId = "", uint32_t intervalMs = 1000);

    /**
     * @brief Destructor guarantees clean worker thread termination without leaks.
     */
    ~HeartbeatManager();

    // Non-copyable, movable
    HeartbeatManager(const HeartbeatManager&) = delete;
    HeartbeatManager& operator=(const HeartbeatManager&) = delete;
    HeartbeatManager(HeartbeatManager&&) noexcept;
    HeartbeatManager& operator=(HeartbeatManager&&) noexcept;

    // =========================================================================
    // Lifecycle Management
    // =========================================================================

    /**
     * @brief Start the periodic heartbeat background worker.
     * @return true if started successfully; false if already executing.
     */
    bool Start();

    /**
     * @brief Start with an explicit heartbeat delivery callback.
     * @param callback Function invoked each time a heartbeat packet is generated.
     * @return true if started; false otherwise.
     */
    bool Start(HeartbeatCallback callback);

    /**
     * @brief Stop the periodic heartbeat worker cleanly.
     */
    void Stop();

    /**
     * @brief Check whether heartbeat thread is currently running.
     */
    bool IsRunning() const noexcept;

    // =========================================================================
    // Heartbeat Factory
    // =========================================================================

    /**
     * @brief Construct an instantaneous HEARTBEAT message packet.
     * @return Formatted communication::Message ready for transmission.
     */
    communication::Message GenerateHeartbeat() const;

    /**
     * @brief Construct an instantaneous HEARTBEAT message with customized payload data.
     * @param payload Telemetry string or JSON payload.
     * @return Formatted communication::Message.
     */
    communication::Message GenerateHeartbeat(const std::string& payload) const;

    // =========================================================================
    // Configuration & Inspection
    // =========================================================================

    void SetIntersectionId(std::string intersectionId);
    std::string GetIntersectionId() const;

    void SetIntervalMs(uint32_t intervalMs);
    uint32_t GetIntervalMs() const noexcept;

    void SetCallback(HeartbeatCallback callback);

    uint64_t GetHeartbeatCount() const noexcept;

private:
    void WorkerLoop();

    std::string m_intersectionId{""};
    std::atomic<uint32_t> m_intervalMs{1000};
    std::atomic<bool> m_running{false};
    std::atomic<uint64_t> m_heartbeatCount{0};

    mutable std::mutex m_mutex{};
    std::condition_variable m_cv{};
    std::thread m_workerThread{};

    mutable std::mutex m_callbackMutex{};
    HeartbeatCallback m_callback{nullptr};
};

} // namespace intersection
} // namespace aegis

#endif // AEGIS_INTERSECTION_HEARTBEAT_MANAGER_H
