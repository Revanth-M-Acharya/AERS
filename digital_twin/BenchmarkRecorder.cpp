/**
 * @file BenchmarkRecorder.cpp
 * @brief Implementation of BenchmarkRecorder.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "digital_twin/BenchmarkRecorder.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>

namespace aegis::digital_twin {

void BenchmarkRecorder::RecordLatency(const std::string& operationName, double latencyUs, double targetSlaUs) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& op = history_[operationName];
    op.name = operationName;
    op.targetSlaUs = targetSlaUs;
    if (op.samplesUs.size() >= 1000) {
        op.samplesUs.erase(op.samplesUs.begin());
    }
    op.samplesUs.push_back(latencyUs);
}

BenchmarkMetric BenchmarkRecorder::GetMetric(const std::string& operationName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = history_.find(operationName);
    if (it == history_.end() || it->second.samplesUs.empty()) {
        BenchmarkMetric def;
        def.operationName = operationName;
        return def;
    }

    const auto& op = it->second;
    auto sorted = op.samplesUs;
    std::sort(sorted.begin(), sorted.end());

    BenchmarkMetric m;
    m.operationName = operationName;
    m.sampleCount = sorted.size();
    m.minLatencyUs = sorted.front();
    m.maxLatencyUs = sorted.back();

    double sum = std::accumulate(sorted.begin(), sorted.end(), 0.0);
    m.meanLatencyUs = sum / static_cast<double>(sorted.size());

    size_t idx95 = static_cast<size_t>(static_cast<double>(sorted.size()) * 0.95);
    size_t idx99 = static_cast<size_t>(static_cast<double>(sorted.size()) * 0.99);
    if (idx95 >= sorted.size()) idx95 = sorted.size() - 1;
    if (idx99 >= sorted.size()) idx99 = sorted.size() - 1;

    m.p95LatencyUs = sorted[idx95];
    m.p99LatencyUs = sorted[idx99];
    m.targetSlaUs = op.targetSlaUs;
    m.passesSla = (m.p99LatencyUs <= op.targetSlaUs);

    return m;
}

std::vector<BenchmarkMetric> BenchmarkRecorder::GetAllMetrics() const {
    std::vector<std::string> names;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [name, _] : history_) {
            names.push_back(name);
        }
    }
    std::vector<BenchmarkMetric> list;
    for (const auto& name : names) {
        list.push_back(GetMetric(name));
    }
    return list;
}

void BenchmarkRecorder::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    history_.clear();
}

std::string BenchmarkRecorder::ToJson() const {
    auto metrics = GetAllMetrics();
    std::ostringstream ss;
    ss << "{\"benchmarks\":[";
    for (size_t i = 0; i < metrics.size(); ++i) {
        const auto& m = metrics[i];
        ss << "{\"op\":\"" << m.operationName << "\""
           << ",\"count\":" << m.sampleCount
           << ",\"minUs\":" << std::fixed << std::setprecision(1) << m.minLatencyUs
           << ",\"maxUs\":" << m.maxLatencyUs
           << ",\"meanUs\":" << m.meanLatencyUs
           << ",\"p95Us\":" << m.p95LatencyUs
           << ",\"p99Us\":" << m.p99LatencyUs
           << ",\"slaUs\":" << m.targetSlaUs
           << ",\"pass\":" << (m.passesSla ? "true" : "false") << "}"
           << (i + 1 < metrics.size() ? "," : "");
    }
    ss << "]}";
    return ss.str();
}

std::string BenchmarkRecorder::ToAsciiTable() const {
    auto metrics = GetAllMetrics();
    std::ostringstream ss;
    ss << "===================================================================================================\n"
       << " SMART AEGIS REAL-TIME BENCHMARKS & LATENCY PERCENTILES (QNX SDP 8.0 RTOS)                         \n"
       << "===================================================================================================\n"
       << " Operation                    | Samples | Min(us) | Mean(us) | P95(us) | P99(us) | SLA(us) | Status \n"
       << "------------------------------+---------+---------+----------+---------+---------+---------+--------\n";

    if (metrics.empty()) {
        ss << " (No benchmarks recorded)\n";
    } else {
        for (const auto& m : metrics) {
            ss << " " << std::left << std::setw(28) << m.operationName << " | "
               << std::right << std::setw(7) << m.sampleCount << " | "
               << std::fixed << std::setprecision(1)
               << std::setw(7) << m.minLatencyUs << " | "
               << std::setw(8) << m.meanLatencyUs << " | "
               << std::setw(7) << m.p95LatencyUs << " | "
               << std::setw(7) << m.p99LatencyUs << " | "
               << std::setw(7) << m.targetSlaUs << " | "
               << (m.passesSla ? "PASS   " : "FAIL   ") << "\n";
        }
    }
    ss << "===================================================================================================\n";
    return ss.str();
}

} // namespace aegis::digital_twin

