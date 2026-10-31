/**
 * @file RoadBlockageAnalyzer.cpp
 * @brief Implementation of RoadBlockageAnalyzer blockage classification.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/perception/analytics/RoadBlockageAnalyzer.hpp"
#include <algorithm>
#include <unordered_set>

namespace aegis::coordinator::perception::analytics {

std::string BlockageTypeToString(BlockageType type) {
    switch (type) {
        case BlockageType::NONE:              return "NONE";
        case BlockageType::PARTIAL_BLOCKAGE:  return "PARTIAL_BLOCKAGE";
        case BlockageType::COMPLETE_BLOCKAGE: return "COMPLETE_BLOCKAGE";
        default:                              return "UNKNOWN";
    }
}

SegmentBlockageReport RoadBlockageAnalyzer::EvaluateBlockage(
    const std::string& segmentId,
    uint32_t totalLanes,
    const std::vector<RoadConditionEvent>& activeEvents) {

    if (totalLanes == 0) totalLanes = 2;

    SegmentBlockageReport report;
    report.segmentId = segmentId;

    std::unordered_set<uint32_t> blockedLanesSet;
    bool forceCompleteBlockage = false;
    double maxDelay = 0.0;
    std::string primaryReason;

    for (const auto& evt : activeEvents) {
        if (evt.verificationStatus == VerificationStatus::RESOLVED ||
            evt.verificationStatus == VerificationStatus::FALSE_POSITIVE) {
            continue;
        }

        if (evt.type == RoadEventType::ROAD_CLOSED ||
            evt.type == RoadEventType::FLOODING ||
            evt.type == RoadEventType::WRONG_WAY_VEHICLE ||
            evt.isBlocking) {
            forceCompleteBlockage = true;
            primaryReason = RoadEventTypeToString(evt.type);
            maxDelay = std::max(maxDelay, evt.estimatedDelaySeconds);
            break;
        }

        if (!evt.affectedLanes.empty()) {
            for (uint32_t lane : evt.affectedLanes) {
                blockedLanesSet.insert(lane);
            }
            if (primaryReason.empty()) {
                primaryReason = RoadEventTypeToString(evt.type);
            }
            maxDelay = std::max(maxDelay, evt.estimatedDelaySeconds);
        }
    }

    if (forceCompleteBlockage || blockedLanesSet.size() >= totalLanes) {
        report.blockageType = BlockageType::COMPLETE_BLOCKAGE;
        report.isRerouteMandatory = true;
        for (uint32_t i = 1; i <= totalLanes; ++i) {
            report.closedLanes.push_back(i);
        }
    } else if (!blockedLanesSet.empty()) {
        report.blockageType = BlockageType::PARTIAL_BLOCKAGE;
        report.isRerouteMandatory = false;
        for (uint32_t i = 1; i <= totalLanes; ++i) {
            if (blockedLanesSet.find(i) != blockedLanesSet.end()) {
                report.closedLanes.push_back(i);
            } else {
                report.openLanes.push_back(i);
            }
        }
    } else {
        report.blockageType = BlockageType::NONE;
        report.isRerouteMandatory = false;
        for (uint32_t i = 1; i <= totalLanes; ++i) {
            report.openLanes.push_back(i);
        }
    }

    report.primaryObstacleReason = primaryReason;
    report.estimatedClearanceTimeSec = maxDelay;

    std::lock_guard<std::mutex> lock(mutex_);
    reports_[segmentId] = report;
    return report;
}

SegmentBlockageReport RoadBlockageAnalyzer::GetBlockageReport(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = reports_.find(segmentId);
    if (it != reports_.end()) return it->second;
    SegmentBlockageReport def;
    def.segmentId = segmentId;
    return def;
}

bool RoadBlockageAnalyzer::IsRerouteRequired(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = reports_.find(segmentId);
    if (it != reports_.end()) return it->second.isRerouteMandatory;
    return false;
}

void RoadBlockageAnalyzer::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    reports_.clear();
}

} // namespace aegis::coordinator::perception::analytics
