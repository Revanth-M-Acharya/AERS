/**
 * @file DoctorPreparationManager.hpp
 * @brief Clinical readiness milestone tracking for surgical, trauma, and ER teams.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <optional>
#include <cstdint>

namespace aegis::coordinator::hospital {

enum class PreparationStage {
    NOT_STARTED,
    PREPARATION_STARTED,
    EQUIPMENT_READY,
    OPERATION_THEATRE_READY,
    EMERGENCY_ROOM_READY,
    MEDICAL_TEAM_READY,
    PREPARATION_COMPLETE
};

inline std::string PreparationStageToString(PreparationStage stage) {
    switch (stage) {
        case PreparationStage::NOT_STARTED:             return "NOT_STARTED";
        case PreparationStage::PREPARATION_STARTED:     return "PREPARATION_STARTED";
        case PreparationStage::EQUIPMENT_READY:         return "EQUIPMENT_READY";
        case PreparationStage::OPERATION_THEATRE_READY: return "OPERATION_THEATRE_READY";
        case PreparationStage::EMERGENCY_ROOM_READY:    return "EMERGENCY_ROOM_READY";
        case PreparationStage::MEDICAL_TEAM_READY:      return "MEDICAL_TEAM_READY";
        case PreparationStage::PREPARATION_COMPLETE:    return "PREPARATION_COMPLETE";
        default:                                        return "UNKNOWN";
    }
}

struct DoctorReadinessRecord {
    std::string emergencyId;
    std::string hospitalId;
    std::string leadDoctorName;
    std::string assignedDepartment;
    PreparationStage currentStage{PreparationStage::NOT_STARTED};
    std::vector<PreparationStage> completedStages;
    std::string readinessAckCode;
    uint64_t startedAtMonoMs{0};
    uint64_t completedAtMonoMs{0};
    bool isReady{false};
};

class DoctorPreparationManager {
public:
    DoctorPreparationManager() = default;
    ~DoctorPreparationManager() = default;

    /**
     * @brief Initialize preparation tracking for an incoming emergency case.
     */
    bool StartPreparation(const std::string& emergencyId,
                          const std::string& hospitalId,
                          const std::string& leadDoctorName,
                          const std::string& department,
                          uint64_t monoMs = 0);

    /**
     * @brief Advance clinical milestone.
     */
    bool AdvanceStage(const std::string& emergencyId, PreparationStage stage, uint64_t monoMs = 0);

    /**
     * @brief Check whether hospital medical team is 100% prepared.
     */
    bool IsPreparationComplete(const std::string& emergencyId) const;

    /**
     * @brief Generate readiness acknowledgement code (e.g. ACK-DOC-XXXX).
     */
    std::string GenerateReadinessAck(const std::string& emergencyId);

    std::optional<DoctorReadinessRecord> GetRecord(const std::string& emergencyId) const;

    void Reset();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, DoctorReadinessRecord> records_;
};

} // namespace aegis::coordinator::hospital

