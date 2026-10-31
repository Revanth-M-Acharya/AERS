/**
 * @file RoadBlockageAnalyzer.hpp
 * @brief Assesses total vs partial physical road passage blockages.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/perception/RoadConditionEvent.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace aegis::coordinator::perception::analytics {

enum class BlockageType {
    NONE,
    PARTIAL_BLOCKAGE,   // At least one lane open
    COMPLETE_BLOCKAGE   // All lanes obstructed / road closed
};

std::string BlockageTypeToString(BlockageType type);

struct SegmentBlockageReport {
    std::string segmentId;
    BlockageType blockageType{BlockageType::NONE};
    std::vector<uint32_t> closedLanes;
    std::vector<uint32_t> openLanes;
    std::string primaryObstacleReason;
    double estimatedClearanceTimeSec{0.0};
    bool isRerouteMandatory{false};
};

class RoadBlockageAnalyzer {
public:
    RoadBlockageAnalyzer() = default;
    ~RoadBlockageAnalyzer() = default;

    SegmentBlockageReport EvaluateBlockage(const std::string& segmentId,
                                          uint32_t totalLanes,
                                          const std::vector<RoadConditionEvent>& activeEvents);

    SegmentBlockageReport GetBlockageReport(const std::string& segmentId) const;
    bool IsRerouteRequired(const std::string& segmentId) const;

    void Reset();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, SegmentBlockageReport> reports_;
};

} // namespace aegis::coordinator::perception::analytics

