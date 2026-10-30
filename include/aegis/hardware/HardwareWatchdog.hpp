/**
 * @file HardwareWatchdog.hpp
 * @brief SMART AEGIS (AERS) Phase 20.7 - Dedicated Hardware Watchdog Thread.
 *
 * Runs an active 1 Hz supervisory loop monitoring coordinator keepalives, local node
 * ticks, relay health, GPIO states, worker thread liveness, IR sensor activity,
 * and Wi-Fi link integrity. Automatically forces ALL_RED failsafe upon fault.
 */

#ifndef AEGIS_HARDWARE_HARDWARE_WATCHDOG_HPP
#define AEGIS_HARDWARE_HARDWARE_WATCHDOG_HPP

#include <string>
#include <unordered_map>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstdint>

namespace aegis {
namespace hardware {

enum class WatchdogFailsafeMode : uint8_t {
    ALL_RED = 0,
    FLASHING_YELLOW = 1
};

inline std::string WatchdogFailsafeModeToString(WatchdogFailsafeMode mode) {
    switch (mode) {
        case WatchdogFailsafeMode::ALL_RED:         return "ALL_RED";
        case WatchdogFailsafeMode::FLASHING_YELLOW: return "FLASHING_YELLOW";
        default:                                    return "UNKNOWN";
    }
}

/**
 * @brief Dedicated 1 Hz hardware supervisory watchdog monitoring physical hardware,
 * network connectivity, and RTOS thread health.
 */
class HardwareWatchdog {
public:
    using FailsafeAction = std::function<void(const std::string& reason)>;
    using CoordinatorNotifyAction = std::function<void(const std::string& alert)>;
    using HealthCheckFunc = std::function<bool()>;

    HardwareWatchdog();
    ~HardwareWatchdog();

    // Non-copyable, non-movable
    HardwareWatchdog(const HardwareWatchdog&) = delete;
    HardwareWatchdog& operator=(const HardwareWatchdog&) = delete;

    /**
     * @brief Start the dedicated 1 Hz watchdog background thread.
     * @return true if started successfully; false if already running.
     */
    bool Start();

    /**
     * @brief Stop the watchdog thread cleanly.
     */
    void Stop();

    /**
     * @brief Check whether the watchdog background thread is active.
     */
    bool IsRunning() const noexcept;

    /**
     * @brief Synchronously execute one supervisory inspection tick (1 Hz step).
     * Exposed for deterministic unit testing and synchronous execution.
     * @return true if all checks passed, false if fault tripped.
     */
    bool TickOnce();

    // Heartbeat Kicks
    void KickCoordinatorHeartbeat() noexcept;
    void KickNodeHeartbeat() noexcept;
    void KickThread(const std::string& threadName);

    // Health Check Registrations (Phase 2 Full Subsystem Supervision)
    void SetRelayHealthChecker(HealthCheckFunc checker);
    void SetGpioStateChecker(HealthCheckFunc checker);
    void SetIrSensorActivityChecker(HealthCheckFunc checker);
    void SetWifiConnectivityChecker(HealthCheckFunc checker);
    void SetTrafficControllerChecker(HealthCheckFunc checker);
    void SetBootManagerChecker(HealthCheckFunc checker);

    // Action Callbacks & Failsafe Mode
    void SetFailsafeCallback(FailsafeAction cb);
    void SetCoordinatorNotifyCallback(CoordinatorNotifyAction cb);
    void SetFailsafeMode(WatchdogFailsafeMode mode) noexcept { m_failsafeMode = mode; }
    WatchdogFailsafeMode GetFailsafeMode() const noexcept { return m_failsafeMode; }

    // Diagnostics & State
    bool HasFault() const noexcept;
    std::string GetFaultReason() const;
    uint64_t GetTickCount() const noexcept;
    uint32_t GetRecoveryAttemptCount() const noexcept;
    void ClearFault();
    bool AttemptAutoRecovery();
    std::string GenerateWatchdogReport() const;

    // Configurable Timeouts
    void SetCoordinatorTimeoutMs(uint32_t timeoutMs) noexcept { m_coordinatorTimeoutMs = timeoutMs; }
    void SetNodeTimeoutMs(uint32_t timeoutMs) noexcept { m_nodeTimeoutMs = timeoutMs; }
    void SetThreadTimeoutMs(uint32_t timeoutMs) noexcept { m_threadTimeoutMs = timeoutMs; }

private:
    void WatchdogLoop();
    bool PerformSupervisoryChecks(std::string& outFaultReason);

    mutable std::mutex m_mutex;
    std::atomic<bool> m_running{false};
    std::thread m_workerThread;

    std::atomic<uint64_t> m_tickCount{0};
    std::atomic<bool> m_hasFault{false};
    std::string m_faultReason;
    std::atomic<uint32_t> m_recoveryAttempts{0};

    // Keepalive Timestamps (in milliseconds)
    std::atomic<uint64_t> m_lastCoordinatorHeartbeatMs{0};
    std::atomic<uint64_t> m_lastNodeHeartbeatMs{0};
    std::unordered_map<std::string, uint64_t> m_threadHeartbeats;

    // Timeout Thresholds
    uint32_t m_coordinatorTimeoutMs{5000}; // 5.0 seconds
    uint32_t m_nodeTimeoutMs{2000};        // 2.0 seconds
    uint32_t m_threadTimeoutMs{3000};      // 3.0 seconds

    // Functional Checkers
    HealthCheckFunc m_relayHealthCheck{nullptr};
    HealthCheckFunc m_gpioStateCheck{nullptr};
    HealthCheckFunc m_irActivityCheck{nullptr};
    HealthCheckFunc m_wifiConnectivityCheck{nullptr};
    HealthCheckFunc m_trafficControllerCheck{nullptr};
    HealthCheckFunc m_bootManagerCheck{nullptr};

    // Failsafe Mode & Callbacks
    WatchdogFailsafeMode m_failsafeMode{WatchdogFailsafeMode::ALL_RED};
    FailsafeAction m_failsafeAction{nullptr};
    CoordinatorNotifyAction m_coordinatorNotifyAction{nullptr};
};

} // namespace hardware
} // namespace aegis

#endif // AEGIS_HARDWARE_HARDWARE_WATCHDOG_HPP

