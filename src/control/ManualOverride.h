/**
 * @file ManualOverride.h
 * @brief Data models and records for Control Center manual overrides in SMART AEGIS.
 */

#ifndef AEGIS_CONTROL_MANUAL_OVERRIDE_H
#define AEGIS_CONTROL_MANUAL_OVERRIDE_H

#include <string>
#include <vector>
#include <cstdint>
#include <sstream>

#include "medical/PriorityEngine.h"

namespace aegis {
namespace control {

/**
 * @brief Categorical manual intervention actions executable by a human operator.
 */
enum class OverrideType {
    INCREASE_PRIORITY,
    DECREASE_PRIORITY,
    CANCEL_CORRIDOR,
    CREATE_CORRIDOR_MANUAL,
    PAUSE_CORRIDOR,
    RESUME_CORRIDOR,
    FORCE_GREEN,
    FORCE_RED,
    EMERGENCY_STOP
};

inline std::string OverrideTypeToString(OverrideType type) {
    switch (type) {
        case OverrideType::INCREASE_PRIORITY:      return "INCREASE_PRIORITY";
        case OverrideType::DECREASE_PRIORITY:      return "DECREASE_PRIORITY";
        case OverrideType::CANCEL_CORRIDOR:        return "CANCEL_CORRIDOR";
        case OverrideType::CREATE_CORRIDOR_MANUAL: return "CREATE_CORRIDOR_MANUAL";
        case OverrideType::PAUSE_CORRIDOR:         return "PAUSE_CORRIDOR";
        case OverrideType::RESUME_CORRIDOR:        return "RESUME_CORRIDOR";
        case OverrideType::FORCE_GREEN:            return "FORCE_GREEN";
        case OverrideType::FORCE_RED:              return "FORCE_RED";
        case OverrideType::EMERGENCY_STOP:         return "EMERGENCY_STOP";
        default:                                   return "UNKNOWN";
    }
}

inline OverrideType StringToOverrideType(const std::string& str) {
    if (str == "INCREASE_PRIORITY")      return OverrideType::INCREASE_PRIORITY;
    if (str == "DECREASE_PRIORITY")      return OverrideType::DECREASE_PRIORITY;
    if (str == "CANCEL_CORRIDOR")        return OverrideType::CANCEL_CORRIDOR;
    if (str == "CREATE_CORRIDOR_MANUAL") return OverrideType::CREATE_CORRIDOR_MANUAL;
    if (str == "PAUSE_CORRIDOR")         return OverrideType::PAUSE_CORRIDOR;
    if (str == "RESUME_CORRIDOR")        return OverrideType::RESUME_CORRIDOR;
    if (str == "FORCE_GREEN")            return OverrideType::FORCE_GREEN;
    if (str == "FORCE_RED")              return OverrideType::FORCE_RED;
    if (str == "EMERGENCY_STOP")         return OverrideType::EMERGENCY_STOP;
    return OverrideType::INCREASE_PRIORITY;
}

/**
 * @brief Lifecycle status of a manual override record.
 */
enum class OverrideStatus {
    PENDING,
    ACTIVE,
    CLEARED,
    EXPIRED
};

inline std::string OverrideStatusToString(OverrideStatus status) {
    switch (status) {
        case OverrideStatus::PENDING: return "PENDING";
        case OverrideStatus::ACTIVE:  return "ACTIVE";
        case OverrideStatus::CLEARED: return "CLEARED";
        case OverrideStatus::EXPIRED: return "EXPIRED";
        default:                      return "UNKNOWN";
    }
}

/**
 * @brief Immutable audit log record capturing every manual dispatcher intervention.
 */
struct ManualOverrideRecord {
    std::string overrideId{""};
    std::string operatorId{"OPERATOR_1"};
    OverrideType type{OverrideType::INCREASE_PRIORITY};
    std::string targetAmbulanceId{""};
    std::string targetIntersectionId{""};
    std::string targetCorridorId{""};
    medical::PriorityLevel previousPriority{medical::PriorityLevel::LOW};
    medical::PriorityLevel newPriority{medical::PriorityLevel::LOW};
    std::vector<std::string> route{};
    uint32_t durationSeconds{0};
    std::string reason{""};
    uint64_t timestampEpochMs{0};
    OverrideStatus status{OverrideStatus::ACTIVE};

    std::string ToString() const;
    std::string ToJson() const;
};

/**
 * @brief Encapsulates a manual intervention directive applied to the system.
 */
class ManualOverride {
public:
    ManualOverride() = default;
    explicit ManualOverride(ManualOverrideRecord record);
    ~ManualOverride() = default;

    const ManualOverrideRecord& GetRecord() const noexcept { return m_record; }
    ManualOverrideRecord& GetRecord() noexcept { return m_record; }

    const std::string& GetOverrideId() const noexcept { return m_record.overrideId; }
    const std::string& GetOperatorId() const noexcept { return m_record.operatorId; }
    OverrideType GetType() const noexcept { return m_record.type; }
    std::string GetTypeString() const { return OverrideTypeToString(m_record.type); }

    const std::string& GetTargetAmbulanceId() const noexcept { return m_record.targetAmbulanceId; }
    const std::string& GetTargetIntersectionId() const noexcept { return m_record.targetIntersectionId; }
    const std::string& GetTargetCorridorId() const noexcept { return m_record.targetCorridorId; }

    bool IsActive() const noexcept { return m_record.status == OverrideStatus::ACTIVE; }
    void Clear() noexcept { m_record.status = OverrideStatus::CLEARED; }

    std::string ToString() const { return m_record.ToString(); }
    std::string ToJson() const { return m_record.ToJson(); }

private:
    ManualOverrideRecord m_record{};
};

} // namespace control
} // namespace aegis

#endif // AEGIS_CONTROL_MANUAL_OVERRIDE_H
