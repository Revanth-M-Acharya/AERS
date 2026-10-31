/**
 * @file DashboardController.cpp
 * @brief Implementation of DashboardController multi-view console engine.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "digital_twin/DashboardController.hpp"
#include <sstream>

namespace aegis::digital_twin {

std::string DashboardViewToString(DashboardView view) {
    switch (view) {
        case DashboardView::CITY_AND_CORRIDORS:       return "CITY_AND_CORRIDORS";
        case DashboardView::AMBULANCE_AND_HOSPITALS:  return "AMBULANCE_AND_HOSPITALS";
        case DashboardView::PERCEPTION_AND_CAMERAS:   return "PERCEPTION_AND_CAMERAS";
        case DashboardView::PERFORMANCE_AND_HARDWARE: return "PERFORMANCE_AND_HARDWARE";
        case DashboardView::TIMELINE_AND_REPLAY:      return "TIMELINE_AND_REPLAY";
    }
    return "UNKNOWN";
}

DashboardController::DashboardController(
    std::shared_ptr<CityModel> cityModel,
    std::shared_ptr<TelemetryCollector> telemetryCollector,
    std::shared_ptr<PerformanceMonitor> performanceMonitor,
    std::shared_ptr<StatisticsEngine> statisticsEngine,
    std::shared_ptr<BenchmarkRecorder> benchmarkRecorder,
    std::shared_ptr<MissionTimeline> missionTimeline,
    std::shared_ptr<MissionReplayEngine> replayEngine)
    : cityModel_(std::move(cityModel)),
      telemetryCollector_(std::move(telemetryCollector)),
      performanceMonitor_(std::move(performanceMonitor)),
      statisticsEngine_(std::move(statisticsEngine)),
      benchmarkRecorder_(std::move(benchmarkRecorder)),
      missionTimeline_(std::move(missionTimeline)),
      replayEngine_(std::move(replayEngine)) {}

void DashboardController::SetActiveView(DashboardView view) {
    std::lock_guard<std::mutex> lock(mutex_);
    activeView_ = view;
}

DashboardView DashboardController::GetActiveView() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return activeView_;
}

std::string DashboardController::RenderActiveView() const {
    DashboardView view;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        view = activeView_;
    }

    switch (view) {
        case DashboardView::CITY_AND_CORRIDORS:       return RenderCityAndCorridorsView();
        case DashboardView::AMBULANCE_AND_HOSPITALS:  return RenderAmbulanceAndHospitalsView();
        case DashboardView::PERCEPTION_AND_CAMERAS:   return RenderPerceptionAndCamerasView();
        case DashboardView::PERFORMANCE_AND_HARDWARE: return RenderPerformanceAndHardwareView();
        case DashboardView::TIMELINE_AND_REPLAY:      return RenderTimelineAndReplayView();
    }
    return "";
}

std::string DashboardController::RenderCityAndCorridorsView() const {
    std::ostringstream ss;
    visualizers::CoordinatorOverview coord;
    ss << visualizers::CoordinatorVisualizer::RenderAsciiCoordinatorStatus(coord) << "\n";

    if (cityModel_) {
        ss << visualizers::TrafficSignalVisualizer::RenderIntersectionsSummary(cityModel_->GetAllIntersections()) << "\n";

        visualizers::CorridorStateOverview corr;
        corr.corridorId = "CORRIDOR_GREEN_WAVE_01";
        corr.emergencyId = "EMG_CARDIAC_01";
        corr.path = {"INT_A", "INT_B", "INT_C"};
        corr.totalDistanceMeters = 1350.0;
        corr.estimatedTransitSeconds = 72.0;
        corr.isFullyClear = true;
        ss << visualizers::CorridorVisualizer::RenderAsciiCorridor(corr, *cityModel_) << "\n";
    }
    return ss.str();
}

std::string DashboardController::RenderAmbulanceAndHospitalsView() const {
    std::ostringstream ss;
    if (cityModel_) {
        ss << visualizers::AmbulanceVisualizer::RenderFleetDashboard(cityModel_->GetAllAmbulances()) << "\n";
        ss << visualizers::HospitalVisualizer::RenderAsciiHospitalBoard(cityModel_->GetAllHospitals()) << "\n";
    }
    return ss.str();
}

std::string DashboardController::RenderPerceptionAndCamerasView() const {
    std::ostringstream ss;
    if (cityModel_) {
        ss << visualizers::CameraVisualizer::RenderAsciiCameraGrid(cityModel_->GetAllCameras(), cityModel_->GetAllRoadConditions()) << "\n";
    }
    return ss.str();
}

std::string DashboardController::RenderPerformanceAndHardwareView() const {
    std::ostringstream ss;
    if (performanceMonitor_) {
        ss << performanceMonitor_->RenderAsciiDashboard() << "\n";
    }
    if (benchmarkRecorder_) {
        ss << benchmarkRecorder_->ToAsciiTable() << "\n";
    }
    if (statisticsEngine_) {
        ss << statisticsEngine_->GenerateReport().ToSummaryString() << "\n";
    }
    return ss.str();
}

std::string DashboardController::RenderTimelineAndReplayView() const {
    std::ostringstream ss;
    if (missionTimeline_) {
        ss << missionTimeline_->ToAsciiTimeline(20) << "\n";
    }
    if (replayEngine_) {
        ss << "=== MISSION REPLAY ENGINE STATUS ===============================================\n"
           << " Replay State : " << ReplayStateToString(replayEngine_->GetState())
           << " | Speed: " << replayEngine_->GetPlaybackSpeed() << "x"
           << " | Frame: " << replayEngine_->GetCurrentFrameIndex() << "/" << replayEngine_->GetTotalFrames() << "\n"
           << "================================================================================\n";
    }
    return ss.str();
}

std::string DashboardController::ExportUnifiedJson() const {
    std::ostringstream ss;
    ss << "{"
       << "\"activeView\":\"" << DashboardViewToString(GetActiveView()) << "\"";
    if (cityModel_) ss << ",\"city\":" << cityModel_->ToJson();
    if (telemetryCollector_) ss << ",\"telemetry\":" << telemetryCollector_->GetSnapshot().ToJson();
    if (performanceMonitor_) ss << ",\"performance\":" << performanceMonitor_->ToJson();
    if (statisticsEngine_) ss << ",\"statistics\":" << statisticsEngine_->GenerateReport().ToJson();
    if (benchmarkRecorder_) ss << ",\"benchmarks\":" << benchmarkRecorder_->ToJson();
    if (missionTimeline_) ss << ",\"timeline\":" << missionTimeline_->ToJson();
    if (replayEngine_) ss << ",\"replay\":" << replayEngine_->ExportArchiveJson();
    ss << "}";
    return ss.str();
}

} // namespace aegis::digital_twin

