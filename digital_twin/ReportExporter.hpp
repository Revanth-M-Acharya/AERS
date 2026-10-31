/**
 * @file ReportExporter.hpp
 * @brief Exports structured mission summary reports in Markdown, JSON, and ASCII formats.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "digital_twin/CityModel.hpp"
#include "digital_twin/MissionTimeline.hpp"
#include "digital_twin/TelemetryCollector.hpp"
#include "digital_twin/PerformanceMonitor.hpp"
#include "digital_twin/StatisticsEngine.hpp"
#include "digital_twin/BenchmarkRecorder.hpp"
#include <string>

namespace aegis::digital_twin {

class ReportExporter {
public:
    static std::string ExportMarkdownReport(
        const CityModel& city,
        const MissionTimeline& timeline,
        const TelemetryCollector& telemetry,
        const PerformanceMonitor& perf,
        const StatisticsEngine& stats,
        const BenchmarkRecorder& benchmarks,
        const std::string& reportTitle = "SMART AEGIS Mission & System Evaluation Report");

    static std::string ExportJsonReport(
        const CityModel& city,
        const MissionTimeline& timeline,
        const TelemetryCollector& telemetry,
        const PerformanceMonitor& perf,
        const StatisticsEngine& stats,
        const BenchmarkRecorder& benchmarks);

    static std::string ExportAsciiSummary(
        const CityModel& city,
        const TelemetryCollector& telemetry,
        const PerformanceMonitor& perf,
        const StatisticsEngine& stats,
        const BenchmarkRecorder& benchmarks);
};

} // namespace aegis::digital_twin

