/**
 * @file HospitalRegistry.hpp
 * @brief Registry and capability directory for regional hospitals and trauma centers.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/orchestration/state/CoordinatorStateManager.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <memory>
#include <cstdint>

namespace aegis::coordinator::hospital {

enum class HospitalStatus {
    ONLINE,
    OFFLINE,
    FULL,
    EMERGENCY_ONLY,
    UNDER_MAINTENANCE
};

inline std::string HospitalStatusToString(HospitalStatus status) {
    switch (status) {
        case HospitalStatus::ONLINE:            return "ONLINE";
        case HospitalStatus::OFFLINE:           return "OFFLINE";
        case HospitalStatus::FULL:              return "FULL";
        case HospitalStatus::EMERGENCY_ONLY:    return "EMERGENCY_ONLY";
        case HospitalStatus::UNDER_MAINTENANCE: return "UNDER_MAINTENANCE";
        default:                                return "UNKNOWN";
    }
}

inline HospitalStatus StringToHospitalStatus(const std::string& str) {
    if (str == "ONLINE")            return HospitalStatus::ONLINE;
    if (str == "OFFLINE")           return HospitalStatus::OFFLINE;
    if (str == "FULL")              return HospitalStatus::FULL;
    if (str == "EMERGENCY_ONLY")    return HospitalStatus::EMERGENCY_ONLY;
    if (str == "UNDER_MAINTENANCE") return HospitalStatus::UNDER_MAINTENANCE;
    return HospitalStatus::ONLINE;
}

struct HospitalData {
    std::string hospitalId;
    std::string hospitalName;
    double latitude{0.0};
    double longitude{0.0};
    uint32_t traumaLevel{1};
    std::vector<std::string> departments;
    uint32_t availableBeds{0};
    uint32_t icuBeds{0};
    uint32_t ventilatorCount{0};
    uint32_t emergencyCapacity{0};
    uint32_t avgPrepTimeSec{180};
    bool isAvailable{true};
    HospitalStatus status{HospitalStatus::ONLINE};

    bool HasDepartment(const std::string& dept) const;
};

class HospitalRegistry {
public:
    explicit HospitalRegistry(std::shared_ptr<orchestration::CoordinatorStateManager> stateManager = nullptr);
    ~HospitalRegistry() = default;

    /**
     * @brief Register or update hospital details.
     */
    bool RegisterHospital(const HospitalData& hospital);

    /**
     * @brief Update status of hospital (ONLINE, FULL, EMERGENCY_ONLY, etc.).
     */
    bool UpdateStatus(const std::string& hospitalId, HospitalStatus status);

    /**
     * @brief Update bed counts.
     */
    bool UpdateBeds(const std::string& hospitalId, uint32_t availableBeds, uint32_t icuBeds = 0);

    std::optional<HospitalData> GetHospital(const std::string& hospitalId) const;
    std::vector<HospitalData> GetAllHospitals() const;
    std::vector<HospitalData> GetHospitalsByDepartment(const std::string& department) const;
    size_t GetHospitalCount() const;

    /**
     * @brief Load hospital configurations from Hospital.conf.
     */
    bool LoadFromConfigFile(const std::string& filePath);

    void Reset();

private:
    std::shared_ptr<orchestration::CoordinatorStateManager> stateManager_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, HospitalData> hospitals_;
};

} // namespace aegis::coordinator::hospital

