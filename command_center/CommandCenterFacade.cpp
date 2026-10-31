/**
 * @file CommandCenterFacade.cpp
 * @brief Implementation of unified Command Center facade.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "command_center/CommandCenterFacade.hpp"

namespace aegis::command_center {

CommandCenterFacade::CommandCenterFacade() {
    Initialize(nullptr);
}

CommandCenterFacade::CommandCenterFacade(std::shared_ptr<coordinator::orchestration::IEventBus> eventBus) {
    Initialize(std::move(eventBus));
}

CommandCenterFacade::~CommandCenterFacade() {
    Stop();
}

void CommandCenterFacade::Initialize(std::shared_ptr<coordinator::orchestration::IEventBus> eventBus) {
    eventBus_ = std::move(eventBus);

    fleet_ = std::make_shared<EmsFleetManager>();
    tracker_ = std::make_shared<LiveAmbulanceTracker>();
    missions_ = std::make_shared<MissionDatabase>();
    hospitals_ = std::make_shared<HospitalOperationsDashboard>();
    traffic_ = std::make_shared<TrafficOperationsDashboard>();
    cluster_ = std::make_shared<ClusterMonitor>();
    health_ = std::make_shared<SystemHealthMonitor>();
    analytics_ = std::make_shared<RealTimeAnalyticsEngine>();
    replay_ = std::make_shared<MissionReplayController>();

    console_ = std::make_shared<CommandCenterConsole>(
        fleet_, tracker_, missions_, hospitals_, traffic_, cluster_, health_, analytics_
    );

    if (eventBus_) {
        eventBridge_ = std::make_shared<CommandCenterEventBridge>(
            eventBus_, fleet_, tracker_, missions_, hospitals_, traffic_, cluster_, health_, analytics_
        );
    }
}

void CommandCenterFacade::Start() {
    if (isRunning_) return;

    if (eventBridge_) {
        eventBridge_->Connect();
    }
    isRunning_ = true;
}

void CommandCenterFacade::Stop() {
    if (!isRunning_) return;

    if (eventBridge_) {
        eventBridge_->Disconnect();
    }
    isRunning_ = false;
}

bool CommandCenterFacade::IsRunning() const {
    return isRunning_;
}

bool CommandCenterFacade::LoadMissionForReplay(const std::string& missionId) {
    if (!missions_ || !replay_) return false;

    auto missionOpt = missions_->GetMission(missionId);
    if (!missionOpt) return false;

    return replay_->LoadMission(*missionOpt);
}

std::string CommandCenterFacade::RenderDashboard() const {
    if (!console_) return "[Command Center Console Not Initialized]\n";
    return console_->RenderFullDashboard();
}

void CommandCenterFacade::PrintDashboard(std::ostream& os) const {
    if (console_) {
        console_->Print(os);
    } else {
        os << "[Command Center Console Not Initialized]\n";
    }
}

} // namespace aegis::command_center
