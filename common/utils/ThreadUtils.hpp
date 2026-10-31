/**
 * @file ThreadUtils.hpp
 * @brief Cross-platform POSIX and QNX Neutrino RTOS thread management utilities.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <pthread.h>

namespace aegis::common {

class ThreadUtils {
public:
    /**
     * @brief Set the name of the calling thread for QNX Momentics / debugger inspection.
     * @param name Name string (truncated if exceeding platform limit, e.g. 15 chars on Linux).
     */
    static void SetCurrentThreadName(const std::string& name);

    /**
     * @brief Configure real-time scheduling policy and priority on POSIX / QNX RTOS.
     * @param thread The target pthread_t handle.
     * @param policy Scheduling policy (e.g. SCHED_FIFO or SCHED_RR).
     * @param priority Real-time priority level (e.g. 20-30 on QNX).
     * @return true if successful, false otherwise.
     */
    static bool SetRealtimePriority(pthread_t thread, int policy, int priority);
};

} // namespace aegis::common

