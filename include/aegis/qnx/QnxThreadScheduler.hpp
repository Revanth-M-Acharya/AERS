/**
 * @file QnxThreadScheduler.hpp
 * @brief SMART AEGIS (AERS) Phase 4 - Real-Time Thread Scheduling & Priority-Inheritance Mutexes.
 *
 * Implements native QNX SCHED_FIFO thread priority hierarchy and mutexes:
 * - Priority 30: Coordinator Core Thread
 * - Priority 25: Intersection Controller Threads
 * - Priority 21: Dynamic Corridor Release & Schedule Engine
 * - Priority 12: Health Monitor & Deadline Supervision
 * - Priority 5:  Telemetry, Logging, and devctl Handlers
 *
 * Provides PTHREAD_PRIO_INHERIT mutex wrappers to prevent unbounded priority inversion,
 * and a system-wide thread scheduler registry for runtime verification.
 *
 * Platform: Raspberry Pi 4 Model B | QNX SDP 8.0 (aarch64le) | Neutrino RTOS
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_QNX_THREAD_SCHEDULER_HPP
#define AEGIS_QNX_THREAD_SCHEDULER_HPP

#include <pthread.h>
#include <sched.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace aegis {
namespace qnx {

namespace Priority {
    constexpr int COORDINATOR_CORE   = 30; ///< Highest critical priority: municipal green corridor dispatch
    constexpr int INTERSECTION_CTRL  = 25; ///< Real-time GPIO relay actuation & phase sequencing
    constexpr int SCHEDULER_ENGINE   = 21; ///< Dynamic corridor release & timeline generation
    constexpr int HEALTH_DEADLINE    = 12; ///< Deadline monitor & fault detection supervision
    constexpr int TELEMETRY_LOGGING  = 5;  ///< Telemetry aggregation, logging & devctl server
}

/**
 * @brief Thread registration metadata for RTOS scheduling audit.
 */
struct ThreadScheduleInfo {
    pthread_t threadId{0};
    std::string threadName;
    int priority{0};
    int policy{SCHED_FIFO};
    uint64_t registeredTimestampUs{0};
};

/**
 * @brief RAII wrapper around pthread_mutex_t configured with PTHREAD_PRIO_INHERIT.
 * Compatible with std::lock_guard and std::unique_lock.
 */
class PrioInheritMutex {
public:
    PrioInheritMutex();
    ~PrioInheritMutex();

    // Prevent copying
    PrioInheritMutex(const PrioInheritMutex&) = delete;
    PrioInheritMutex& operator=(const PrioInheritMutex&) = delete;

    void lock();
    void unlock();
    bool try_lock();

    pthread_mutex_t* native_handle() noexcept { return &m_mutex; }

private:
    pthread_mutex_t m_mutex;
};

/**
 * @brief Manager and registry for real-time thread priority assignment and verification.
 */
class QnxThreadScheduler {
public:
    QnxThreadScheduler();
    ~QnxThreadScheduler() = default;

    /**
     * @brief Sets real-time scheduling priority and policy on the target thread.
     * Uses pthread_setschedparam() / pthread_setschedprio().
     * @param thread POSIX thread identifier.
     * @param priority Target priority level (1 to 63 on QNX).
     * @param policy Scheduling algorithm (default SCHED_FIFO).
     * @return 0 on success, or errno on failure.
     */
    static int SetThreadPriority(pthread_t thread, int priority, int policy = SCHED_FIFO);

    /**
     * @brief Sets real-time priority and policy on the calling thread.
     */
    static int SetCurrentThreadPriority(int priority, int policy = SCHED_FIFO);

    /**
     * @brief Gets current scheduling priority and policy of a thread.
     * @param thread POSIX thread identifier.
     * @param outPriority Variable to receive priority.
     * @param outPolicy Variable to receive policy.
     * @return 0 on success, or errno on failure.
     */
    static int GetThreadPriority(pthread_t thread, int& outPriority, int& outPolicy);

    /**
     * @brief Registers a thread with the system scheduler registry.
     */
    void RegisterThread(pthread_t thread, const std::string& name, int priority, int policy = SCHED_FIFO);

    /**
     * @brief Unregisters a thread from the registry.
     */
    void UnregisterThread(pthread_t thread);

    /**
     * @brief Looks up thread scheduling metadata by name.
     */
    bool GetThreadInfo(const std::string& name, ThreadScheduleInfo& outInfo) const;

    /**
     * @brief Retrieves a list of all registered threads.
     */
    std::vector<ThreadScheduleInfo> GetAllThreads() const;

    /**
     * @brief Verifies that the priority hierarchy is maintained:
     * Coordinator(30) > Intersections(25) > Scheduler(21) > Health(12) > Telemetry(5).
     * @return true if all priority tiers are strictly monotonic and valid.
     */
    bool VerifyHierarchy() const;

private:
    mutable std::mutex m_registryMutex;
    std::unordered_map<pthread_t, ThreadScheduleInfo> m_threadsByHandle;
    std::unordered_map<std::string, ThreadScheduleInfo> m_threadsByName;
};

} // namespace qnx
} // namespace aegis

#endif // AEGIS_QNX_THREAD_SCHEDULER_HPP

