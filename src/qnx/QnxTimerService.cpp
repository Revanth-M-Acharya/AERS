/**
 * @file QnxTimerService.cpp
 * @brief Implementation of Phase 4 High-Resolution QNX Timers.
 */

#include "aegis/qnx/QnxTimerService.hpp"
#include "aegis/qnx/QnxPulseManager.hpp"
#include "aegis/common/time_utils.hpp"
#include "aegis/logger/logger.hpp"

#include <cmath>

namespace aegis {
namespace qnx {

QnxTimerService::QnxTimerService() = default;

QnxTimerService::~QnxTimerService() {
#if !defined(__QNX__) && !defined(__QNXNTO__)
    std::lock_guard<std::mutex> lock(m_simMutex);
    for (auto& pair : m_simTimers) {
        auto t = pair.second;
        if (t) {
            t->destroyed.store(true);
            t->armed.store(false);
            t->cv.notify_all();
            if (t->workerThread.joinable()) {
                t->workerThread.join();
            }
        }
    }
    m_simTimers.clear();
#else
    // Disarm and delete any remaining native timers
    std::lock_guard<std::mutex> lock(m_qnxStatsMutex);
    for (auto& pair : m_qnxConfigs) {
        timer_delete(pair.first);
    }
    m_qnxConfigs.clear();
    m_qnxStats.clear();
#endif
}

int QnxTimerService::CreateTimer(int coid, int8_t pulseCode, int32_t pulseValue, int priority, AegisTimerId* outTimerId) {
    if (!outTimerId) return -1;

#if defined(__QNX__) || defined(__QNXNTO__)
    struct sigevent event;
    SIGEV_PULSE_INIT(&event, coid, priority, pulseCode, pulseValue);

    timer_t t;
    if (timer_create(CLOCK_MONOTONIC, &event, &t) != 0) {
        return -1;
    }

    *outTimerId = t;

    TimerConfig cfg;
    cfg.coid = coid;
    cfg.pulseCode = pulseCode;
    cfg.pulseValue = pulseValue;
    cfg.priority = priority;

    std::lock_guard<std::mutex> lock(m_qnxStatsMutex);
    m_qnxConfigs[t] = cfg;
    m_qnxStats[t] = TimerJitterStats{};
    return 0;
#else
    std::lock_guard<std::mutex> lock(m_simMutex);
    AegisTimerId id = m_nextSimTimerId++;

    auto simTimer = std::make_shared<SimulatedTimer>();
    simTimer->id = id;
    simTimer->config.coid = coid;
    simTimer->config.pulseCode = pulseCode;
    simTimer->config.pulseValue = pulseValue;
    simTimer->config.priority = priority;

    m_simTimers[id] = simTimer;
    *outTimerId = id;
    return 0;
#endif
}

int QnxTimerService::ArmPeriodicTimer(AegisTimerId timerId, uint64_t periodNs, uint64_t initialDelayNs) {
    if (periodNs == 0) return -1;

#if defined(__QNX__) || defined(__QNXNTO__)
    uint64_t initDelay = (initialDelayNs > 0) ? initialDelayNs : periodNs;

    struct itimerspec itime;
    itime.it_value.tv_sec = static_cast<time_t>(initDelay / 1000000000ULL);
    itime.it_value.tv_nsec = static_cast<long>(initDelay % 1000000000ULL);
    itime.it_interval.tv_sec = static_cast<time_t>(periodNs / 1000000000ULL);
    itime.it_interval.tv_nsec = static_cast<long>(periodNs % 1000000000ULL);

    if (timer_settime(timerId, 0, &itime, nullptr) != 0) {
        return -1;
    }

    std::lock_guard<std::mutex> lock(m_qnxStatsMutex);
    auto it = m_qnxConfigs.find(timerId);
    if (it != m_qnxConfigs.end()) {
        it->second.periodNs = periodNs;
        it->second.initialDelayNs = initDelay;
        it->second.periodic = true;
    }
    return 0;
#else
    std::shared_ptr<SimulatedTimer> timer;
    {
        std::lock_guard<std::mutex> lock(m_simMutex);
        auto it = m_simTimers.find(timerId);
        if (it == m_simTimers.end()) return -1;
        timer = it->second;
    }

    // Stop existing thread if running
    timer->armed.store(false);
    timer->cv.notify_all();
    if (timer->workerThread.joinable()) {
        timer->workerThread.join();
    }

    timer->config.periodNs = periodNs;
    timer->config.initialDelayNs = (initialDelayNs > 0) ? initialDelayNs : periodNs;
    timer->config.periodic = true;
    timer->armed.store(true);

    timer->workerThread = std::thread([this, timer]() {
        uint64_t initialDelayMs = timer->config.initialDelayNs / 1000000ULL;
        uint64_t periodMs = timer->config.periodNs / 1000000ULL;
        if (periodMs == 0) periodMs = 1;

        std::unique_lock<std::mutex> lock(timer->mtx);
        if (initialDelayMs > 0) {
            timer->cv.wait_for(lock, std::chrono::milliseconds(initialDelayMs), [&]() {
                return !timer->armed.load() || timer->destroyed.load();
            });
        }

        while (timer->armed.load() && !timer->destroyed.load()) {
            uint64_t expectedUs = aegis::common::GetMonotonicMicroseconds();

            // Send pulse via QnxPulseManager
            QnxPulseManager::SendPulse(timer->config.coid, timer->config.priority,
                                      timer->config.pulseCode, timer->config.pulseValue);

            uint64_t actualUs = aegis::common::GetMonotonicMicroseconds();
            RecordFiring(timer->id, expectedUs, actualUs);

            timer->cv.wait_for(lock, std::chrono::milliseconds(periodMs), [&]() {
                return !timer->armed.load() || timer->destroyed.load();
            });
        }
    });

    return 0;
#endif
}

int QnxTimerService::ArmOneShotTimer(AegisTimerId timerId, uint64_t delayNs) {
    if (delayNs == 0) return -1;

#if defined(__QNX__) || defined(__QNXNTO__)
    struct itimerspec itime;
    itime.it_value.tv_sec = static_cast<time_t>(delayNs / 1000000000ULL);
    itime.it_value.tv_nsec = static_cast<long>(delayNs % 1000000000ULL);
    itime.it_interval.tv_sec = 0;
    itime.it_interval.tv_nsec = 0;

    if (timer_settime(timerId, 0, &itime, nullptr) != 0) {
        return -1;
    }

    std::lock_guard<std::mutex> lock(m_qnxStatsMutex);
    auto it = m_qnxConfigs.find(timerId);
    if (it != m_qnxConfigs.end()) {
        it->second.periodNs = 0;
        it->second.initialDelayNs = delayNs;
        it->second.periodic = false;
    }
    return 0;
#else
    std::shared_ptr<SimulatedTimer> timer;
    {
        std::lock_guard<std::mutex> lock(m_simMutex);
        auto it = m_simTimers.find(timerId);
        if (it == m_simTimers.end()) return -1;
        timer = it->second;
    }

    timer->armed.store(false);
    timer->cv.notify_all();
    if (timer->workerThread.joinable()) {
        timer->workerThread.join();
    }

    timer->config.periodNs = 0;
    timer->config.initialDelayNs = delayNs;
    timer->config.periodic = false;
    timer->armed.store(true);

    timer->workerThread = std::thread([this, timer]() {
        uint64_t delayMs = timer->config.initialDelayNs / 1000000ULL;
        if (delayMs == 0) delayMs = 1;

        std::unique_lock<std::mutex> lock(timer->mtx);
        timer->cv.wait_for(lock, std::chrono::milliseconds(delayMs), [&]() {
            return !timer->armed.load() || timer->destroyed.load();
        });

        if (timer->armed.load() && !timer->destroyed.load()) {
            uint64_t expectedUs = aegis::common::GetMonotonicMicroseconds();
            QnxPulseManager::SendPulse(timer->config.coid, timer->config.priority,
                                      timer->config.pulseCode, timer->config.pulseValue);
            uint64_t actualUs = aegis::common::GetMonotonicMicroseconds();
            RecordFiring(timer->id, expectedUs, actualUs);
            timer->armed.store(false);
        }
    });

    return 0;
#endif
}

int QnxTimerService::DisarmTimer(AegisTimerId timerId) {
#if defined(__QNX__) || defined(__QNXNTO__)
    struct itimerspec itime{};
    return timer_settime(timerId, 0, &itime, nullptr);
#else
    std::lock_guard<std::mutex> lock(m_simMutex);
    auto it = m_simTimers.find(timerId);
    if (it == m_simTimers.end()) return -1;

    it->second->armed.store(false);
    it->second->cv.notify_all();
    return 0;
#endif
}

int QnxTimerService::DestroyTimer(AegisTimerId timerId) {
#if defined(__QNX__) || defined(__QNXNTO__)
    std::lock_guard<std::mutex> lock(m_qnxStatsMutex);
    m_qnxConfigs.erase(timerId);
    m_qnxStats.erase(timerId);
    return timer_delete(timerId);
#else
    std::shared_ptr<SimulatedTimer> timer;
    {
        std::lock_guard<std::mutex> lock(m_simMutex);
        auto it = m_simTimers.find(timerId);
        if (it == m_simTimers.end()) return -1;
        timer = it->second;
        m_simTimers.erase(it);
    }

    if (timer) {
        timer->destroyed.store(true);
        timer->armed.store(false);
        timer->cv.notify_all();
        if (timer->workerThread.joinable()) {
            timer->workerThread.join();
        }
    }
    return 0;
#endif
}

void QnxTimerService::RecordFiring(AegisTimerId timerId, uint64_t expectedUs, uint64_t actualUs) {
    double jitter = (actualUs > expectedUs) ? static_cast<double>(actualUs - expectedUs) : 0.0;

#if defined(__QNX__) || defined(__QNXNTO__)
    std::lock_guard<std::mutex> lock(m_qnxStatsMutex);
    auto it = m_qnxStats.find(timerId);
    if (it != m_qnxStats.end()) {
        auto& stats = it->second;
        stats.totalFirings++;
        int overruns = timer_getoverrun(timerId);
        if (overruns > 0) {
            stats.overrunCount += static_cast<uint64_t>(overruns);
        }
        if (stats.totalFirings == 1) {
            stats.minJitterUs = jitter;
            stats.maxJitterUs = jitter;
            stats.avgJitterUs = jitter;
        } else {
            if (jitter < stats.minJitterUs) stats.minJitterUs = jitter;
            if (jitter > stats.maxJitterUs) stats.maxJitterUs = jitter;
            stats.avgJitterUs = (stats.avgJitterUs * 0.9) + (jitter * 0.1);
        }
    }
#else
    std::lock_guard<std::mutex> lock(m_simMutex);
    auto it = m_simTimers.find(timerId);
    if (it != m_simTimers.end()) {
        auto& stats = it->second->stats;
        stats.totalFirings++;
        if (stats.totalFirings == 1) {
            stats.minJitterUs = jitter;
            stats.maxJitterUs = jitter;
            stats.avgJitterUs = jitter;
        } else {
            if (jitter < stats.minJitterUs) stats.minJitterUs = jitter;
            if (jitter > stats.maxJitterUs) stats.maxJitterUs = jitter;
            stats.avgJitterUs = (stats.avgJitterUs * 0.9) + (jitter * 0.1);
        }
    }
#endif
}

TimerJitterStats QnxTimerService::GetJitterStats(AegisTimerId timerId) const {
#if defined(__QNX__) || defined(__QNXNTO__)
    std::lock_guard<std::mutex> lock(m_qnxStatsMutex);
    auto it = m_qnxStats.find(timerId);
    if (it != m_qnxStats.end()) {
        return it->second;
    }
    return TimerJitterStats{};
#else
    std::lock_guard<std::mutex> lock(m_simMutex);
    auto it = m_simTimers.find(timerId);
    if (it != m_simTimers.end()) {
        return it->second->stats;
    }
    return TimerJitterStats{};
#endif
}

void QnxTimerService::ResetJitterStats(AegisTimerId timerId) {
#if defined(__QNX__) || defined(__QNXNTO__)
    std::lock_guard<std::mutex> lock(m_qnxStatsMutex);
    auto it = m_qnxStats.find(timerId);
    if (it != m_qnxStats.end()) {
        it->second = TimerJitterStats{};
    }
#else
    std::lock_guard<std::mutex> lock(m_simMutex);
    auto it = m_simTimers.find(timerId);
    if (it != m_simTimers.end()) {
        it->second->stats = TimerJitterStats{};
    }
#endif
}

size_t QnxTimerService::GetActiveTimerCount() const {
#if defined(__QNX__) || defined(__QNXNTO__)
    std::lock_guard<std::mutex> lock(m_qnxStatsMutex);
    return m_qnxConfigs.size();
#else
    std::lock_guard<std::mutex> lock(m_simMutex);
    return m_simTimers.size();
#endif
}

} // namespace qnx
} // namespace aegis
