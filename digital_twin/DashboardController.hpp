/**
 * @file DashboardController.hpp
 * @brief Master operations center console controller rendering multi-view dashboards and telemetry exports.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "digital_twin/CityModel.hpp"
#include "digital_twin/TelemetryCollector.hpp"
#include "digital_twin/PerformanceMonitor.hpp"
#include "digital_twin/StatisticsEngine.hpp"
#include "digital_twin/BenchmarkRecorder.hpp"
#include "digital_twin/MissionTimeline.hpp"
#include "digital_twin/MissionReplayEngine.hpp"
#include "digital_twin/visualizers/AmbulanceVisualizer.hpp"
#include "digital_twin/visualizers/TrafficSignalVisualizer.hpp"
#include "digital_twin/visualizers/CorridorVisualizer.hpp"
#include "digital_twin/visualizers/CameraVisualizer.hpp"
#include "digital_twin/visualizers/HospitalVisualizer.hpp"
#include "digital_twin/visualizers/CoordinatorVisualizer.hpp"

#include <string>
#include <memory>
#include <mutex>

namespace aegis::digital_twin {

enum class DashboardView {
    CITY_AND_CORRIDORS,
    AMBULANCE_AND_HOSPITALS,
    PERCEPTION_AND_CAMERAS,
    PERFORMANCE_AND_HARDWARE,
    TIMELINE_AND_REPLAY
};

std::string DashboardViewToString(DashboardView view);

class DashboardController {
public:
    DashboardController(
        std::shared_ptr<CityModel> cityModel,
        std::shared_ptr<TelemetryCollector> telemetryCollector,
        std::shared_ptr<PerformanceMonitor> performanceMonitor,
        std::shared_ptr<StatisticsEngine> statisticsEngine,
        std::shared_ptr<BenchmarkRecorder> benchmarkRecorder,
        std::shared_ptr<MissionTimeline> missionTimeline,
        std::shared_ptr<MissionReplayEngine> replayEngine);
    ~DashboardController() = default;

    void SetActiveView(DashboardView view);
    DashboardView GetActiveView() const;

    // Render currently active view as an ASCII console dashboard
    std::string RenderActiveView() const;

    // Explicit view renderers
    std::string RenderCityAndCorridorsView() const;
    std::string RenderAmbulanceAndHospitalsView() const;
    std::string RenderPerceptionAndCamerasView() const;
    std::string RenderPerformanceAndHardwareView() const;
    std::string RenderTimelineAndReplayView() const;

    // Full system unified JSON export
    std::string ExportUnifiedJson() const;

private:
    std::shared_ptr<CityModel> cityModel_;
    std::shared_ptr<TelemetryCollector> telemetryCollector_;
    std::shared_ptr<PerformanceMonitor> performanceMonitor_;
    std::shared_ptr<StatisticsEngine> statisticsEngine_;
    std::shared_ptr<BenchmarkRecorder> benchmarkRecorder_;
    std::shared_ptr<MissionTimeline> missionTimeline_;
    std::shared_ptr<MissionReplayEngine> replayEngine_;

    mutable std::mutex mutex_;
    DashboardView activeView_{DashboardView::CITY_AND_CORRIDORS};
};

} // namespace aegis::digital_twin

