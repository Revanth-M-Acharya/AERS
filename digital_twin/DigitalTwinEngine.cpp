/**
 * @file DigitalTwinEngine.cpp
 * @brief Implementation of DigitalTwinEngine master subsystem facade.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "digital_twin/DigitalTwinEngine.hpp"
#include "common/utils/TimeUtils.hpp"

namespace aegis::digital_twin {

DigitalTwinEngine::DigitalTwinEngine(std::shared_ptr<coordinator::orchestration::IEventBus> eventBus)
    : eventBus_(std::move(eventBus)) {
    Initialize();
}

DigitalTwinEngine::~DigitalTwinEngine() {
    Stop();
}

void DigitalTwinEngine::Initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    cityModel_ = std::make_shared<CityModel>();
    clock_ = std::make_shared<SimulationClock>();
    telemetryCollector_ = std::make_shared<TelemetryCollector>();
    timeline_ = std::make_shared<MissionTimeline>();
    replayEngine_ = std::make_shared<MissionReplayEngine>();
    performanceMonitor_ = std::make_shared<PerformanceMonitor>(telemetryCollector_);
    statisticsEngine_ = std::make_shared<StatisticsEngine>();
    benchmarkRecorder_ = std::make_shared<BenchmarkRecorder>();

    eventBridge_ = std::make_shared<DashboardEventBridge>(eventBus_, cityModel_, telemetryCollector_);
    scenarioManager_ = std::make_shared<SimulationScenarioManager>(cityModel_, timeline_);

    dashboardController_ = std::make_shared<DashboardController>(
        cityModel_,
        telemetryCollector_,
        performanceMonitor_,
        statisticsEngine_,
        benchmarkRecorder_,
        timeline_,
        replayEngine_);

    isRunning_ = false;
    lastReplayFrameMonoMs_ = 0;
}

void DigitalTwinEngine::Start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (clock_) clock_->Start();
    if (eventBridge_) eventBridge_->StartListening();
    isRunning_ = true;
}

void DigitalTwinEngine::Pause() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (clock_) clock_->Pause();
}

void DigitalTwinEngine::Resume() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (clock_) clock_->Resume();
    isRunning_ = true;
}

void DigitalTwinEngine::Stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (clock_) clock_->Stop();
    if (eventBridge_) eventBridge_->StopListening();
    isRunning_ = false;
}

void DigitalTwinEngine::Step(double stepSeconds) {
    if (clock_) clock_->Step(stepSeconds);
    Update();
}

void DigitalTwinEngine::Update() {
    if (clock_) clock_->Update();

    double simSec = clock_ ? clock_->GetElapsedSimSeconds() : 0.0;
    if (scenarioManager_) {
        scenarioManager_->Update(simSec);
    }

    // Record periodic replay snapshot
    uint64_t nowMono = common::TimeUtils::GetMonotonicTimeMs();
    if (replayEngine_ && cityModel_) {
        if (nowMono >= lastReplayFrameMonoMs_ + 250) { // 4 FPS replay recording
            lastReplayFrameMonoMs_ = nowMono;
            std::string emgId = "";
            auto ambs = cityModel_->GetAllAmbulances();
            if (!ambs.empty()) emgId = ambs[0].activeEmergencyId;
            replayEngine_->RecordFrame(nowMono, simSec, *cityModel_, emgId, "Tick Snapshot");
        }
    }

    if (performanceMonitor_) {
        performanceMonitor_->Sample();
    }
}

void DigitalTwinEngine::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (cityModel_) cityModel_->Reset();
    if (clock_) clock_->Reset();
    if (telemetryCollector_) telemetryCollector_->Reset();
    if (timeline_) timeline_->Clear();
    if (replayEngine_) replayEngine_->ClearRecordings();
    if (scenarioManager_) scenarioManager_->Reset();
    if (statisticsEngine_) statisticsEngine_->Reset();
    if (benchmarkRecorder_) benchmarkRecorder_->Reset();
    lastReplayFrameMonoMs_ = 0;
}

std::string DigitalTwinEngine::ExportMarkdownReport(const std::string& title) const {
    if (!cityModel_ || !timeline_ || !telemetryCollector_ || !performanceMonitor_ || !statisticsEngine_ || !benchmarkRecorder_) {
        return "";
    }
    return ReportExporter::ExportMarkdownReport(
        *cityModel_,
        *timeline_,
        *telemetryCollector_,
        *performanceMonitor_,
        *statisticsEngine_,
        *benchmarkRecorder_,
        title);
}

std::string DigitalTwinEngine::ExportJsonReport() const {
    if (!cityModel_ || !timeline_ || !telemetryCollector_ || !performanceMonitor_ || !statisticsEngine_ || !benchmarkRecorder_) {
        return "{}";
    }
    return ReportExporter::ExportJsonReport(
        *cityModel_,
        *timeline_,
        *telemetryCollector_,
        *performanceMonitor_,
        *statisticsEngine_,
        *benchmarkRecorder_);
}

std::string DigitalTwinEngine::ExportAsciiDashboard() const {
    if (dashboardController_) {
        return dashboardController_->RenderActiveView();
    }
    return "";
}

} // namespace aegis::digital_twin

