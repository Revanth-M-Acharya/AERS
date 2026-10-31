/**
 * @file CoordinatorIntegration.cpp
 * @brief Implementation of CoordinatorIntegration advisory replanning logic.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/perception/CoordinatorIntegration.hpp"
#include "common/utils/TimeUtils.hpp"
#include <sstream>

namespace aegis::coordinator::perception {

std::string ReplanningDecisionStatusToString(ReplanningDecisionStatus status) {
    switch (status) {
        case ReplanningDecisionStatus::PENDING_COORDINATOR_DECISION: return "PENDING_COORDINATOR_DECISION";
        case ReplanningDecisionStatus::APPROVED_BY_COORDINATOR:      return "APPROVED_BY_COORDINATOR";
        case ReplanningDecisionStatus::REJECTED_BY_COORDINATOR:      return "REJECTED_BY_COORDINATOR";
        default:                                                    return "UNKNOWN";
    }
}

CoordinatorIntegration::CoordinatorIntegration(std::shared_ptr<RoadSituationAwarenessEngine> irsaEngine,
                                               std::shared_ptr<orchestration::IEventBus> eventBus,
                                               double replanDelayThresholdSeconds)
    : irsaEngine_(std::move(irsaEngine)),
      eventBus_(std::move(eventBus)),
      replanDelayThresholdSeconds_(replanDelayThresholdSeconds) {}

std::optional<ReplanningRecommendation> CoordinatorIntegration::EvaluateCorridorConditions(
    const std::string& emergencyId,
    const std::string& corridorId,
    const std::vector<std::string>& routeSegments) {

    if (!irsaEngine_) return std::nullopt;

    uint64_t now = common::TimeUtils::GetMonotonicTimeMs();

    for (const auto& segment : routeSegments) {
        auto state = irsaEngine_->GetSegmentState(segment);

        bool needsReplan = false;
        std::string reason;

        if (state.blockage == analytics::BlockageType::COMPLETE_BLOCKAGE ||
            state.statusColor == RoadStatusColor::RED) {
            needsReplan = true;
            reason = "Segment " + segment + " is completely impassable/blocked";
        } else if (state.estimatedTransitDelaySec >= replanDelayThresholdSeconds_) {
            needsReplan = true;
            reason = "Severe congestion delay (" + std::to_string(static_cast<int>(state.estimatedTransitDelaySec)) +
                     "s) on " + segment;
        } else if (!state.activeEvents.empty()) {
            for (const auto& ev : state.activeEvents) {
                if (ev.isBlocking || ev.type == RoadEventType::WRONG_WAY_VEHICLE ||
                    ev.type == RoadEventType::FLOODING || ev.type == RoadEventType::ROAD_CLOSED) {
                    needsReplan = true;
                    reason = "Critical hazard: " + RoadEventTypeToString(ev.type) + " on " + segment;
                    break;
                }
            }
        }

        if (needsReplan) {
            ReplanningRecommendation rec;
            rec.emergencyId = emergencyId;
            rec.corridorId = corridorId;
            rec.obstructedSegment = segment;
            rec.reason = reason;
            rec.estimatedDelaySeconds = state.estimatedTransitDelaySec;
            rec.isCompleteBlockage = (state.blockage == analytics::BlockageType::COMPLETE_BLOCKAGE);
            rec.decisionStatus = ReplanningDecisionStatus::PENDING_COORDINATOR_DECISION;
            rec.generatedMonoMs = now;

            std::lock_guard<std::mutex> lock(mutex_);
            rec.recommendationId = "REPLAN-REC-" + std::to_string(++counter_);
            recommendations_[rec.recommendationId] = rec;

            SubmitRecommendation(rec);
            return rec;
        }
    }

    return std::nullopt;
}

bool CoordinatorIntegration::SubmitRecommendation(const ReplanningRecommendation& recommendation) {
    if (!eventBus_) return false;

    std::ostringstream payload;
    payload << "{\"recommendationId\":\"" << recommendation.recommendationId << "\","
            << "\"emergencyId\":\"" << recommendation.emergencyId << "\","
            << "\"corridorId\":\"" << recommendation.corridorId << "\","
            << "\"obstructedSegment\":\"" << recommendation.obstructedSegment << "\","
            << "\"reason\":\"" << recommendation.reason << "\","
            << "\"estimatedDelaySeconds\":" << recommendation.estimatedDelaySeconds << ","
            << "\"isCompleteBlockage\":" << (recommendation.isCompleteBlockage ? "true" : "false") << "}";

    eventBus_->Publish(orchestration::Event(
        orchestration::EventType::CorridorReplanningAdvised,
        "IRSA_COORDINATOR_INTEGRATION",
        recommendation.emergencyId,
        payload.str(),
        recommendation.generatedMonoMs
    ));

    return true;
}

bool CoordinatorIntegration::RecordCoordinatorDecision(
    const std::string& recommendationId,
    bool approve,
    const std::string& notes) {

    uint64_t now = common::TimeUtils::GetMonotonicTimeMs();

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = recommendations_.find(recommendationId);
    if (it == recommendations_.end()) return false;

    it->second.decisionStatus = approve
        ? ReplanningDecisionStatus::APPROVED_BY_COORDINATOR
        : ReplanningDecisionStatus::REJECTED_BY_COORDINATOR;
    it->second.coordinatorNotes = notes;
    it->second.decidedMonoMs = now;

    return true;
}

std::optional<ReplanningRecommendation> CoordinatorIntegration::GetRecommendation(
    const std::string& recommendationId) const {

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = recommendations_.find(recommendationId);
    if (it != recommendations_.end()) return it->second;
    return std::nullopt;
}

std::vector<ReplanningRecommendation> CoordinatorIntegration::GetAllRecommendations() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ReplanningRecommendation> list;
    list.reserve(recommendations_.size());
    for (const auto& [_, rec] : recommendations_) {
        list.push_back(rec);
    }
    return list;
}

std::vector<ReplanningRecommendation> CoordinatorIntegration::GetPendingRecommendations() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ReplanningRecommendation> list;
    for (const auto& [_, rec] : recommendations_) {
        if (rec.decisionStatus == ReplanningDecisionStatus::PENDING_COORDINATOR_DECISION) {
            list.push_back(rec);
        }
    }
    return list;
}

void CoordinatorIntegration::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    recommendations_.clear();
}

} // namespace aegis::coordinator::perception

