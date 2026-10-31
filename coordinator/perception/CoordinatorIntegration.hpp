/**
 * @file CoordinatorIntegration.hpp
 * @brief Interfaces IRSA perception events with the Coordinator and Dynamic Route Replanner.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/perception/RoadSituationAwarenessEngine.hpp"
#include "coordinator/orchestration/event_bus/IEventBus.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <optional>

namespace aegis::coordinator::perception {

enum class ReplanningDecisionStatus {
    PENDING_COORDINATOR_DECISION,
    APPROVED_BY_COORDINATOR,
    REJECTED_BY_COORDINATOR
};

std::string ReplanningDecisionStatusToString(ReplanningDecisionStatus status);

struct ReplanningRecommendation {
    std::string recommendationId;
    std::string emergencyId;
    std::string corridorId;
    std::string obstructedSegment;
    std::string reason;
    double estimatedDelaySeconds{0.0};
    bool isCompleteBlockage{false};
    ReplanningDecisionStatus decisionStatus{ReplanningDecisionStatus::PENDING_COORDINATOR_DECISION};
    std::string coordinatorNotes;
    uint64_t generatedMonoMs{0};
    uint64_t decidedMonoMs{0};
};

class CoordinatorIntegration {
public:
    CoordinatorIntegration(std::shared_ptr<RoadSituationAwarenessEngine> irsaEngine,
                           std::shared_ptr<orchestration::IEventBus> eventBus = nullptr,
                           double replanDelayThresholdSeconds = 120.0);
    ~CoordinatorIntegration() = default;

    /**
     * @brief Evaluate an active corridor's route segments against current IRSA road events.
     * @param emergencyId The active emergency identifier.
     * @param corridorId The active green corridor identifier.
     * @param routeSegments The sequential list of road segments along the ambulance corridor.
     * @return Optional ReplanningRecommendation if a segment warrants advisory replanning.
     */
    std::optional<ReplanningRecommendation> EvaluateCorridorConditions(
        const std::string& emergencyId,
        const std::string& corridorId,
        const std::vector<std::string>& routeSegments);

    /**
     * @brief Submit the advisory replanning recommendation to the Coordinator via EventBus.
     */
    bool SubmitRecommendation(const ReplanningRecommendation& recommendation);

    /**
     * @brief Coordinator records its authoritative decision to approve or reject replanning.
     * NOTE: Enforces rule "AI NEVER directly changes routes. Only Coordinator approves/rejects."
     */
    bool RecordCoordinatorDecision(const std::string& recommendationId,
                                   bool approve,
                                   const std::string& notes = "");

    std::optional<ReplanningRecommendation> GetRecommendation(const std::string& recommendationId) const;
    std::vector<ReplanningRecommendation> GetAllRecommendations() const;
    std::vector<ReplanningRecommendation> GetPendingRecommendations() const;

    void SetReplanDelayThreshold(double seconds) { replanDelayThresholdSeconds_ = seconds; }
    double GetReplanDelayThreshold() const { return replanDelayThresholdSeconds_; }

    void Reset();

private:
    std::shared_ptr<RoadSituationAwarenessEngine> irsaEngine_;
    std::shared_ptr<orchestration::IEventBus> eventBus_;
    double replanDelayThresholdSeconds_{120.0};

    mutable std::mutex mutex_;
    std::unordered_map<std::string, ReplanningRecommendation> recommendations_;
    uint64_t counter_{7000};
};

} // namespace aegis::coordinator::perception

