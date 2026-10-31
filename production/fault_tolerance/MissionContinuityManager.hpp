/**
 * @file MissionContinuityManager.hpp
 * @brief Mission state checkpointing and failover continuity manager.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace aegis::production {

struct MissionCheckpoint {
    std::string missionId;
    std::string ambulanceId;
    std::string targetHospitalId;
    std::vector<std::string> corridorIntersections;
    std::string currentSegment;
    double currentEtaSeconds{0.0};
    uint64_t checkpointTimestampMs{0};
    std::string triageLevel{"RED"};
    bool isActive{true};
};

class MissionContinuityManager {
public:
    MissionContinuityManager();
    ~MissionContinuityManager() = default;

    /**
     * @brief Save a periodic checkpoint for an active mission.
     */
    void SaveCheckpoint(const MissionCheckpoint& checkpoint);

    /**
     * @brief Retrieve checkpoint for an active mission.
     */
    bool GetCheckpoint(const std::string& missionId, MissionCheckpoint& outCheckpoint) const;

    /**
     * @brief Restore all active missions from the checkpoint journal.
     */
    std::vector<MissionCheckpoint> RestoreActiveMissions() const;

    void MarkMissionCompleted(const std::string& missionId);
    void Clear();

    size_t GetCheckpointCount() const;
    std::string ExportJournalJson() const;
    bool ImportJournalJson(const std::string& json);

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, MissionCheckpoint> checkpoints_;
};

} // namespace aegis::production

