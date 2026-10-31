/**
 * @file DigitalTwinEngine.hpp
 * @brief Master facade for Phase 8 Integrated Digital Twin, Operations Center & System Simulation.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/orchestration/event_bus/IEventBus.hpp"
#include "digital_twin/CityModel.hpp"
#include "digital_twin/SimulationClock.hpp"
#include "digital_twin/TelemetryCollector.hpp"
#include "digital_twin/DashboardEventBridge.hpp"
#include "digital_twin/MissionTimeline.hpp"
#include "digital_twin/MissionReplayEngine.hpp"
#include "digital_twin/ScenarioPlaybackController.hpp"
#include "digital_twin/SimulationScenarioManager.hpp"
#include "digital_twin/PerformanceMonitor.hpp"
#include "digital_twin/StatisticsEngine.hpp"
#include "digital_twin/BenchmarkRecorder.hpp"
#include "digital_twin/DashboardController.hpp"
#include "digital_twin/ReportExporter.hpp"

#include <memory>
#include <mutex>
#include <string>

namespace aegis::digital_twin {

class DigitalTwinEngine {
public:
    explicit DigitalTwinEngine(std::shared_ptr<coordinator::orchestration::IEventBus> eventBus = nullptr);
    ~DigitalTwinEngine();

    void Initialize();
    void Start();
    void Pause();
    void Resume();
    void Stop();
    void Step(double stepSeconds);
    void Update();
    void Reset();

    // Component Accessors
    std::shared_ptr<CityModel> GetCityModel() const { return cityModel_; }
    std::shared_ptr<SimulationClock> GetClock() const { return clock_; }
    std::shared_ptr<TelemetryCollector> GetTelemetryCollector() const { return telemetryCollector_; }
    std::shared_ptr<DashboardEventBridge> GetEventBridge() const { return eventBridge_; }
    std::shared_ptr<MissionTimeline> GetTimeline() const { return timeline_; }
    std::shared_ptr<MissionReplayEngine> GetReplayEngine() const { return replayEngine_; }
    std::shared_ptr<SimulationScenarioManager> GetScenarioManager() const { return scenarioManager_; }
    std::shared_ptr<PerformanceMonitor> GetPerformanceMonitor() const { return performanceMonitor_; }
    std::shared_ptr<StatisticsEngine> GetStatisticsEngine() const { return statisticsEngine_; }
    std::shared_ptr<BenchmarkRecorder> GetBenchmarkRecorder() const { return benchmarkRecorder_; }
    std::shared_ptr<DashboardController> GetDashboardController() const { return dashboardController_; }

    // Reports and Unified Exports
    std::string ExportMarkdownReport(const std::string& title = "SMART AEGIS System Report") const;
    std::string ExportJsonReport() const;
    std::string ExportAsciiDashboard() const;

private:
    std::shared_ptr<coordinator::orchestration::IEventBus> eventBus_;

    std::shared_ptr<CityModel> cityModel_;
    std::shared_ptr<SimulationClock> clock_;
    std::shared_ptr<TelemetryCollector> telemetryCollector_;
    std::shared_ptr<DashboardEventBridge> eventBridge_;
    std::shared_ptr<MissionTimeline> timeline_;
    std::shared_ptr<MissionReplayEngine> replayEngine_;
    std::shared_ptr<SimulationScenarioManager> scenarioManager_;
    std::shared_ptr<PerformanceMonitor> performanceMonitor_;
    std::shared_ptr<StatisticsEngine> statisticsEngine_;
    std::shared_ptr<BenchmarkRecorder> benchmarkRecorder_;
    std::shared_ptr<DashboardController> dashboardController_;

    mutable std::mutex mutex_;
    bool isRunning_{false};
    uint64_t lastReplayFrameMonoMs_{0};
};

} // namespace aegis::digital_twin

