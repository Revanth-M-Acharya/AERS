/**
 * @file ThreadUtils.cpp
 * @brief Implementation of cross-platform thread utilities.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "common/utils/ThreadUtils.hpp"
#include <cstring>

namespace aegis::common {

void ThreadUtils::SetCurrentThreadName(const std::string& name) {
#if defined(__QNX__) || defined(__QNXNTO__)
    // QNX Neutrino RTOS pthread_setname_np accepts (pthread_t, const char*)
    pthread_setname_np(pthread_self(), name.c_str());
#elif defined(__APPLE__)
    // macOS accepts single argument (const char*) for calling thread
    pthread_setname_np(name.c_str());
#elif defined(__linux__)
    // Linux restricts thread names to 16 bytes including null terminator
    std::string truncated = name.substr(0, 15);
    pthread_setname_np(pthread_self(), truncated.c_str());
#else
    // Fallback no-op for generic platforms
    (void)name;
#endif
}

bool ThreadUtils::SetRealtimePriority(pthread_t thread, int policy, int priority) {
    sched_param param{};
    param.sched_priority = priority;
    return (pthread_setschedparam(thread, policy, &param) == 0);
}

} // namespace aegis::common

