/**
 * @file RuntimeDiagnostics.hpp
 * @brief POSIX-compliant host and process diagnostics for QNX Neutrino RTOS 8.0.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <cstdint>
#include <mutex>
#include <vector>

namespace aegis::production {

struct DiagnosticsSnapshot {
    uint64_t timestampMs{0};
    double cpuUsagePercent{0.0};
    uint64_t residentMemoryKb{0};
    uint64_t userTimeMs{0};
    uint64_t systemTimeMs{0};
    uint32_t activeThreads{0};
    uint32_t openFileDescriptors{0};
    uint32_t activeSockets{0};
    uint64_t voluntaryContextSwitches{0};
    uint64_t involuntaryContextSwitches{0};

    std::string ToJson() const;
    std::string ToAscii() const;
};

class RuntimeDiagnostics {
public:
    RuntimeDiagnostics();
    ~RuntimeDiagnostics() = default;

    /**
     * @brief Sample current process runtime metrics using POSIX APIs.
     */
    DiagnosticsSnapshot Sample();

    DiagnosticsSnapshot GetLatestSnapshot() const;
    std::vector<DiagnosticsSnapshot> GetHistory(size_t limit = 50) const;

    void SetSocketCount(uint32_t count);
    void SetThreadCount(uint32_t count);

    void Reset();

private:
    mutable std::mutex mutex_;
    DiagnosticsSnapshot latest_;
    std::vector<DiagnosticsSnapshot> history_;

    uint64_t lastSampleMonoMs_{0};
    uint64_t lastTotalCpuTimeUs_{0};
    uint32_t socketCount_{0};
    uint32_t threadCount_{1};
};

} // namespace aegis::production

