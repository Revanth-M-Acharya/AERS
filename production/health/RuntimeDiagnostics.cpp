/**
 * @file RuntimeDiagnostics.cpp
 * @brief Implementation of runtime metrics and host diagnostics.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "production/health/RuntimeDiagnostics.hpp"
#include "common/utils/TimeUtils.hpp"
#include <sys/resource.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace aegis::production {

std::string DiagnosticsSnapshot::ToJson() const {
    std::ostringstream ss;
    ss << "{\n"
       << "  \"timestamp_ms\": " << timestampMs << ",\n"
       << "  \"cpu_percent\": " << std::fixed << std::setprecision(2) << cpuUsagePercent << ",\n"
       << "  \"memory_kb\": " << residentMemoryKb << ",\n"
       << "  \"user_time_ms\": " << userTimeMs << ",\n"
       << "  \"system_time_ms\": " << systemTimeMs << ",\n"
       << "  \"active_threads\": " << activeThreads << ",\n"
       << "  \"open_fds\": " << openFileDescriptors << ",\n"
       << "  \"active_sockets\": " << activeSockets << ",\n"
       << "  \"voluntary_switches\": " << voluntaryContextSwitches << ",\n"
       << "  \"involuntary_switches\": " << involuntaryContextSwitches << "\n"
       << "}";
    return ss.str();
}

std::string DiagnosticsSnapshot::ToAscii() const {
    std::ostringstream ss;
    ss << "+--------------------------------------------------------------+\n"
       << "| SMART AEGIS RUNTIME DIAGNOSTICS                              |\n"
       << "+--------------------------------------------------------------+\n"
       << "| CPU Usage:         " << std::setw(6) << std::fixed << std::setprecision(1) << cpuUsagePercent << " %       "
       << "Memory (RSS):    " << std::setw(8) << residentMemoryKb << " KB |\n"
       << "| Active Threads:    " << std::setw(6) << activeThreads << "         "
       << "Open Sockets:    " << std::setw(8) << activeSockets << "    |\n"
       << "| Context Switches:  " << std::setw(6) << (voluntaryContextSwitches + involuntaryContextSwitches) << "         "
       << "Open FDs:        " << std::setw(8) << openFileDescriptors << "    |\n"
       << "+--------------------------------------------------------------+\n";
    return ss.str();
}

RuntimeDiagnostics::RuntimeDiagnostics() {
    lastSampleMonoMs_ = common::TimeUtils::GetMonotonicTimeMs();
    struct rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        lastTotalCpuTimeUs_ = static_cast<uint64_t>(usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) * 1000000ULL +
                              static_cast<uint64_t>(usage.ru_utime.tv_usec + usage.ru_stime.tv_usec);
    }
}

DiagnosticsSnapshot RuntimeDiagnostics::Sample() {
    uint64_t nowMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    uint64_t nowEpochMs = common::TimeUtils::GetCurrentTimestampMs();

    struct rusage usage{};
    uint64_t userUs = 0;
    uint64_t sysUs = 0;
    uint64_t maxRss = 0;
    uint64_t volSwitches = 0;
    uint64_t involSwitches = 0;

    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        userUs = static_cast<uint64_t>(usage.ru_utime.tv_sec) * 1000000ULL + static_cast<uint64_t>(usage.ru_utime.tv_usec);
        sysUs = static_cast<uint64_t>(usage.ru_stime.tv_sec) * 1000000ULL + static_cast<uint64_t>(usage.ru_stime.tv_usec);
        maxRss = static_cast<uint64_t>(usage.ru_maxrss);
        volSwitches = static_cast<uint64_t>(usage.ru_nvcsw);
        involSwitches = static_cast<uint64_t>(usage.ru_nivcsw);
    }

    // On macOS ru_maxrss is in bytes, on Linux/QNX in kilobytes.
#if defined(__APPLE__)
    uint64_t rssKb = maxRss / 1024;
#else
    uint64_t rssKb = maxRss;
#endif
    if (rssKb == 0) rssKb = 16384; // Fallback baseline if unpopulated

    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t dtMs = (nowMonoMs > lastSampleMonoMs_) ? (nowMonoMs - lastSampleMonoMs_) : 1;
    uint64_t totalCpuUs = userUs + sysUs;
    uint64_t dCpuUs = (totalCpuUs >= lastTotalCpuTimeUs_) ? (totalCpuUs - lastTotalCpuTimeUs_) : 0;

    double cpuPercent = 0.0;
    if (dtMs > 0) {
        // dCpuUs / (dtMs * 1000) * 100
        cpuPercent = (static_cast<double>(dCpuUs) / static_cast<double>(dtMs * 1000ULL)) * 100.0;
        if (cpuPercent > 100.0) cpuPercent = 100.0;
        if (cpuPercent < 0.0) cpuPercent = 0.0;
    }

    lastSampleMonoMs_ = nowMonoMs;
    lastTotalCpuTimeUs_ = totalCpuUs;

    DiagnosticsSnapshot snap;
    snap.timestampMs = nowEpochMs;
    snap.cpuUsagePercent = cpuPercent;
    snap.residentMemoryKb = rssKb;
    snap.userTimeMs = userUs / 1000ULL;
    snap.systemTimeMs = sysUs / 1000ULL;
    snap.activeThreads = threadCount_;
    snap.openFileDescriptors = socketCount_ + 4; // Baseline FDs (stdin, stdout, stderr, log)
    snap.activeSockets = socketCount_;
    snap.voluntaryContextSwitches = volSwitches;
    snap.involuntaryContextSwitches = involSwitches;

    latest_ = snap;
    history_.push_back(snap);
    if (history_.size() > 100) {
        history_.erase(history_.begin());
    }

    return snap;
}

DiagnosticsSnapshot RuntimeDiagnostics::GetLatestSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return latest_;
}

std::vector<DiagnosticsSnapshot> RuntimeDiagnostics::GetHistory(size_t limit) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (limit >= history_.size()) {
        return history_;
    }
    return std::vector<DiagnosticsSnapshot>(history_.end() - static_cast<std::ptrdiff_t>(limit), history_.end());
}

void RuntimeDiagnostics::SetSocketCount(uint32_t count) {
    std::lock_guard<std::mutex> lock(mutex_);
    socketCount_ = count;
}

void RuntimeDiagnostics::SetThreadCount(uint32_t count) {
    std::lock_guard<std::mutex> lock(mutex_);
    threadCount_ = (count == 0) ? 1 : count;
}

void RuntimeDiagnostics::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    history_.clear();
    lastSampleMonoMs_ = common::TimeUtils::GetMonotonicTimeMs();
}

} // namespace aegis::production

