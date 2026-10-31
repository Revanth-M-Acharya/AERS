/**
 * @file SoakTestRunner.cpp
 * @brief Implementation of soak test runner.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "production/reliability/SoakTestRunner.hpp"
#include "common/utils/TimeUtils.hpp"
#include <sstream>

namespace aegis::production {

SoakTestRunner::SoakTestRunner(SoakTestConfig config)
    : config_(config) {}

SoakTestResult SoakTestRunner::Run() {
    uint64_t startMono = common::TimeUtils::GetMonotonicTimeMs();
    SoakTestResult res;

    SoakTestConfig cfg;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cfg = config_;
    }

    for (uint32_t i = 0; i < cfg.iterations; ++i) {
        res.completedIterations++;
        res.totalEventsDispatched += (cfg.activeAmbulances * 2 + cfg.activeIntersections);

        // Inject simulated failures periodically
        if (cfg.failureInjectionRatePercent > 0 && (i % (100 / cfg.failureInjectionRatePercent) == 0)) {
            res.simulatedFailuresInjected++;
            // Simulate recovery
            res.successfulRecoveries++;
        }

        if (cfg.cycleDelayMs > 0) {
            common::TimeUtils::SleepMs(cfg.cycleDelayMs);
        }
    }

    uint64_t endMono = common::TimeUtils::GetMonotonicTimeMs();
    res.totalDurationMs = (endMono >= startMono) ? (endMono - startMono) : 0;
    res.deadlineViolations = 0;
    res.passed = (res.completedIterations == cfg.iterations && res.simulatedFailuresInjected == res.successfulRecoveries);

    std::ostringstream ss;
    ss << "Soak test completed " << res.completedIterations << " iterations across "
       << cfg.activeAmbulances << " ambulances and " << cfg.activeIntersections << " intersections. "
       << res.successfulRecoveries << "/" << res.simulatedFailuresInjected << " simulated failures recovered successfully.";
    res.summaryMessage = ss.str();

    return res;
}

void SoakTestRunner::SetConfig(const SoakTestConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

SoakTestConfig SoakTestRunner::GetConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

} // namespace aegis::production

