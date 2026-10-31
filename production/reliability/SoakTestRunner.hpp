/**
 * @file SoakTestRunner.hpp
 * @brief Long-duration stability, stress, and multi-node concurrent failure soak test engine.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>
#include <functional>

namespace aegis::production {

struct SoakTestConfig {
    uint32_t iterations{100};
    uint32_t activeAmbulances{4};
    uint32_t activeIntersections{12};
    uint32_t failureInjectionRatePercent{10};
    uint32_t cycleDelayMs{1};
};

struct SoakTestResult {
    uint32_t completedIterations{0};
    uint32_t totalEventsDispatched{0};
    uint32_t simulatedFailuresInjected{0};
    uint32_t successfulRecoveries{0};
    uint32_t deadlineViolations{0};
    uint64_t totalDurationMs{0};
    bool passed{true};
    std::string summaryMessage;
};

class SoakTestRunner {
public:
    explicit SoakTestRunner(SoakTestConfig config = SoakTestConfig{});
    ~SoakTestRunner() = default;

    /**
     * @brief Execute soak test iterations synchronously.
     */
    SoakTestResult Run();

    void SetConfig(const SoakTestConfig& config);
    SoakTestConfig GetConfig() const;

private:
    mutable std::mutex mutex_;
    SoakTestConfig config_;
};

} // namespace aegis::production

