/**
 * @file QnxPulseManager.hpp
 * @brief SMART AEGIS (AERS) Phase 4 - Native QNX Neutrino Pulse Management.
 *
 * Implements native QNX pulse delivery and handling:
 * - Non-blocking asynchronous event notification via MsgSendPulse()
 * - Reception via MsgReceivePulse() / MsgReceive()
 * - Pulse event definitions (Watchdog, Heartbeat, Corridor Timeout, Deadline, Shutdown, Fault Recovery, Interrupt)
 * - Thread-safe pulse dispatching and handler registration
 * - Latency and throughput statistics tracking
 *
 * Platform: Raspberry Pi 4 Model B | QNX SDP 8.0 (aarch64le) | Neutrino RTOS
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_QNX_PULSE_MANAGER_HPP
#define AEGIS_QNX_PULSE_MANAGER_HPP

#include <cstdint>
#include <string>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>

#if defined(__QNX__) || defined(__QNXNTO__)
    #include <sys/neutrino.h>
    #include <sys/siginfo.h>
#endif

namespace aegis {
namespace qnx {

namespace PulseCode {
    constexpr int8_t WATCHDOG        = 1;
    constexpr int8_t HEARTBEAT       = 2;
    constexpr int8_t CORRIDOR_TIMEOUT= 3;
    constexpr int8_t DEADLINE        = 4;
    constexpr int8_t SHUTDOWN        = 5;
    constexpr int8_t FAULT_RECOVERY  = 6;
    constexpr int8_t INTERRUPT       = 7;
}

/**
 * @brief Converts pulse code to human-readable string.
 */
inline const char* PulseCodeToString(int8_t code) noexcept {
    switch (code) {
        case PulseCode::WATCHDOG:        return "PULSE_WATCHDOG";
        case PulseCode::HEARTBEAT:       return "PULSE_HEARTBEAT";
        case PulseCode::CORRIDOR_TIMEOUT:return "PULSE_CORRIDOR_TIMEOUT";
        case PulseCode::DEADLINE:        return "PULSE_DEADLINE";
        case PulseCode::SHUTDOWN:        return "PULSE_SHUTDOWN";
        case PulseCode::FAULT_RECOVERY:  return "PULSE_FAULT_RECOVERY";
        case PulseCode::INTERRUPT:       return "PULSE_INTERRUPT";
        default:                         return "PULSE_UNKNOWN";
    }
}

#pragma pack(push, 1)

/**
 * @brief Layout matching QNX Neutrino struct _pulse for binary compatibility.
 */
struct QnxRawPulse {
    uint16_t type{0};
    uint16_t subtype{0};
    int8_t code{0};
    uint8_t zero[3]{0, 0, 0};
    int32_t value{0};
    int32_t scoid{0};
};

#pragma pack(pop)

/**
 * @brief High-level pulse event container with timestamp and telemetry.
 */
struct QnxPulseEvent {
    int8_t code{0};
    int32_t value{0};
    int32_t priority{0};
    uint64_t timestampUs{0};
    int32_t scoid{0};
};

/**
 * @brief Pulse delivery and reception statistics.
 */
struct PulseStats {
    uint64_t totalSent{0};
    uint64_t totalReceived{0};
    uint64_t dropped{0};
    std::unordered_map<int8_t, uint64_t> countPerCode;
    double avgLatencyUs{0.0};
};

/**
 * @brief Callback signature for registered pulse handlers.
 */
using PulseHandler = std::function<void(const QnxPulseEvent&)>;

/**
 * @brief Manager class for sending, receiving, and dispatching QNX pulses.
 */
class QnxPulseManager {
public:
    QnxPulseManager();
    ~QnxPulseManager();

    // Prevent copying
    QnxPulseManager(const QnxPulseManager&) = delete;
    QnxPulseManager& operator=(const QnxPulseManager&) = delete;

    /**
     * @brief Sends a non-blocking pulse to the target connection ID.
     * Uses MsgSendPulse() on QNX Neutrino.
     * @param coid Connection ID to target channel.
     * @param priority Delivery priority (e.g. 10 to 30).
     * @param code Application pulse code (PulseCode::*).
     * @param value 32-bit payload value.
     * @return 0 on success, negative error code on failure.
     */
    static int SendPulse(int coid, int priority, int8_t code, int32_t value);

    /**
     * @brief Receives a pulse from a channel with optional timeout.
     * @param chid Channel ID.
     * @param outPulse Populated pulse event structure.
     * @param timeoutMs Timeout in milliseconds (-1 = blocking indefinite).
     * @return 0 on success, -1 on timeout or error.
     */
    static int ReceivePulse(int chid, QnxPulseEvent& outPulse, int timeoutMs = -1);

    /**
     * @brief Registers an asynchronous handler for a specific pulse code.
     * @param code Pulse code to listen for.
     * @param handler Callback to invoke when pulse is received.
     */
    void RegisterHandler(int8_t code, PulseHandler handler);

    /**
     * @brief Unregisters a handler for a pulse code.
     */
    void UnregisterHandler(int8_t code);

    /**
     * @brief Starts an asynchronous background dispatcher thread on the given channel.
     * @param chid Channel ID to listen on.
     * @return true if started successfully.
     */
    bool StartDispatcher(int chid);

    /**
     * @brief Stops the background dispatcher thread.
     */
    void StopDispatcher();

    /**
     * @brief Checks if dispatcher thread is currently running.
     */
    bool IsDispatcherRunning() const noexcept { return m_running.load(); }

    /**
     * @brief Retrieves snapshot of pulse statistics.
     */
    PulseStats GetStats() const;

    /**
     * @brief Resets pulse statistics counters.
     */
    void ResetStats();

    /**
     * @brief Internal hook to register a simulated channel for host execution.
     */
    static void RegisterSimulatedChannel(int chid);
    static void UnregisterSimulatedChannel(int chid);

private:
    void DispatcherThreadFunc(int chid);

    std::atomic<bool> m_running{false};
    std::thread m_dispatcherThread;
    mutable std::mutex m_handlersMutex;
    std::unordered_map<int8_t, PulseHandler> m_handlers;

    mutable std::mutex m_statsMutex;
    PulseStats m_stats;
};

} // namespace qnx
} // namespace aegis

#endif // AEGIS_QNX_PULSE_MANAGER_HPP

