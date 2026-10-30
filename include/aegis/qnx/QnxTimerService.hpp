/**
 * @file QnxTimerService.hpp
 * @brief SMART AEGIS (AERS) Phase 4 - High-Resolution QNX Timers & Microsecond Jitter Statistics.
 *
 * Implements native QNX high-resolution monotonic timer services:
 * - POSIX timer_create() bound to QNX SIGEV_PULSE
 * - High-precision periodic and one-shot timers via timer_settime(CLOCK_MONOTONIC)
 * - Microsecond jitter tracking (min, max, average)
 * - Timer overrun monitoring via timer_getoverrun()
 * - Seamless fallback for macOS/Linux test environments
 *
 * Platform: Raspberry Pi 4 Model B | QNX SDP 8.0 (aarch64le) | Neutrino RTOS
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_QNX_TIMER_SERVICE_HPP
#define AEGIS_QNX_TIMER_SERVICE_HPP

#include <cstdint>
#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <condition_variable>

#if defined(__QNX__) || defined(__QNXNTO__)
    #include <time.h>
    #include <sys/neutrino.h>
    #include <sys/siginfo.h>
    using AegisTimerId = timer_t;
#else
    using AegisTimerId = uintptr_t;
#endif

namespace aegis {
namespace qnx {

/**
 * @brief Telemetry metrics for timer real-time precision.
 */
struct TimerJitterStats {
    uint64_t totalFirings{0};
    uint64_t overrunCount{0};
    double minJitterUs{0.0};
    double maxJitterUs{0.0};
    double avgJitterUs{0.0};
};

/**
 * @brief Configuration parameters for a QNX timer instance.
 */
struct TimerConfig {
    int coid{0};
    int8_t pulseCode{0};
    int32_t pulseValue{0};
    int priority{21};
    uint64_t periodNs{0};
    uint64_t initialDelayNs{0};
    bool periodic{false};
};

/**
 * @brief High-resolution timer service delivering QNX pulses on expiration.
 */
class QnxTimerService {
public:
    QnxTimerService();
    ~QnxTimerService();

    // Prevent copying
    QnxTimerService(const QnxTimerService&) = delete;
    QnxTimerService& operator=(const QnxTimerService&) = delete;

    /**
     * @brief Creates a high-resolution timer bound to pulse delivery.
     * @param coid Target connection ID.
     * @param pulseCode Application pulse code.
     * @param pulseValue 32-bit payload value.
     * @param priority Delivery priority (e.g. 21).
     * @param outTimerId Pointer to receive the timer identifier.
     * @return 0 on success, negative error code on failure.
     */
    int CreateTimer(int coid, int8_t pulseCode, int32_t pulseValue, int priority, AegisTimerId* outTimerId);

    /**
     * @brief Arms a periodic timer.
     * @param timerId Timer identifier.
     * @param periodNs Periodic interval in nanoseconds.
     * @param initialDelayNs Initial delay before first expiration in nanoseconds (0 = immediate periodNs).
     * @return 0 on success, -1 on failure.
     */
    int ArmPeriodicTimer(AegisTimerId timerId, uint64_t periodNs, uint64_t initialDelayNs = 0);

    /**
     * @brief Arms a one-shot timer.
     * @param timerId Timer identifier.
     * @param delayNs One-shot delay in nanoseconds.
     * @return 0 on success, -1 on failure.
     */
    int ArmOneShotTimer(AegisTimerId timerId, uint64_t delayNs);

    /**
     * @brief Disarms an active timer without destroying it.
     * @param timerId Timer identifier.
     * @return 0 on success, -1 on failure.
     */
    int DisarmTimer(AegisTimerId timerId);

    /**
     * @brief Destroys a timer and frees associated resources.
     * @param timerId Timer identifier.
     * @return 0 on success, -1 on failure.
     */
    int DestroyTimer(AegisTimerId timerId);

    /**
     * @brief Records a firing event to calculate real-time jitter statistics.
     * @param timerId Timer identifier.
     * @param expectedUs Expected expiration time in microseconds.
     * @param actualUs Actual expiration time in microseconds.
     */
    void RecordFiring(AegisTimerId timerId, uint64_t expectedUs, uint64_t actualUs);

    /**
     * @brief Retrieves jitter and overrun telemetry for a timer.
     */
    TimerJitterStats GetJitterStats(AegisTimerId timerId) const;

    /**
     * @brief Resets jitter statistics for a timer.
     */
    void ResetJitterStats(AegisTimerId timerId);

    /**
     * @brief Retrieves the total number of managed timers.
     */
    size_t GetActiveTimerCount() const;

private:
#if !defined(__QNX__) && !defined(__QNXNTO__)
    struct SimulatedTimer {
        AegisTimerId id{0};
        TimerConfig config;
        std::atomic<bool> armed{false};
        std::atomic<bool> destroyed{false};
        std::thread workerThread;
        std::mutex mtx;
        std::condition_variable cv;
        TimerJitterStats stats;
    };

    mutable std::mutex m_simMutex;
    std::unordered_map<AegisTimerId, std::shared_ptr<SimulatedTimer>> m_simTimers;
    AegisTimerId m_nextSimTimerId{1000};
#else
    mutable std::mutex m_qnxStatsMutex;
    std::unordered_map<AegisTimerId, TimerJitterStats> m_qnxStats;
    std::unordered_map<AegisTimerId, TimerConfig> m_qnxConfigs;
#endif
};

} // namespace qnx
} // namespace aegis

#endif // AEGIS_QNX_TIMER_SERVICE_HPP

