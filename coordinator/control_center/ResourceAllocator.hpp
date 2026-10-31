/**
 * @file ResourceAllocator.hpp
 * @brief Dispatch resource allocation, mutual exclusion, and conflict prevention manager.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/orchestration/event_bus/IEventBus.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <memory>
#include <optional>

namespace aegis::coordinator::control_center {

struct AllocationDetails {
    std::string incidentId;
    std::string primaryAmbulanceId;
    std::string backupAmbulanceId;
    std::string hospitalId;
    std::string dispatcherId;
    std::vector<std::string> corridorIntersections;
    uint64_t allocatedAtMonoMs{0};
};

class ResourceAllocator {
public:
    explicit ResourceAllocator(std::shared_ptr<orchestration::IEventBus> eventBus = nullptr);
    ~ResourceAllocator() = default;

    /**
     * @brief Allocate ambulance, backup ambulance, hospital, and dispatcher to an incident.
     * Prevents double assignment.
     */
    bool Allocate(const std::string& incidentId,
                  const std::string& primaryAmbulanceId,
                  const std::string& backupAmbulanceId,
                  const std::string& hospitalId,
                  const std::string& dispatcherId,
                  const std::vector<std::string>& corridorIntersections,
                  uint64_t monoMs = 0);

    /**
     * @brief Release all resources held by an incident upon completion or cancellation.
     */
    bool Release(const std::string& incidentId, uint64_t monoMs = 0);

    /**
     * @brief Reassign primary ambulance (e.g. vehicle breakdown or dynamic diversion).
     */
    bool ReassignAmbulance(const std::string& incidentId,
                           const std::string& newAmbulanceId,
                           uint64_t monoMs = 0);

    /**
     * @brief Reassign target hospital (e.g. ER diversion or trauma escalation).
     */
    bool ReassignHospital(const std::string& incidentId,
                          const std::string& newHospitalId,
                          uint64_t monoMs = 0);

    /**
     * @brief Promote backup ambulance to primary if primary fails.
     */
    bool FailoverToBackupAmbulance(const std::string& incidentId, uint64_t monoMs = 0);

    bool IsAmbulanceBusy(const std::string& ambulanceId) const;
    bool IsDispatcherBusy(const std::string& dispatcherId) const;
    std::optional<AllocationDetails> GetAllocation(const std::string& incidentId) const;

    void Reset();

private:
    std::shared_ptr<orchestration::IEventBus> eventBus_;
    mutable std::mutex mutex_;

    std::unordered_map<std::string, AllocationDetails> allocationsByIncident_;
    std::unordered_map<std::string, std::string> ambulanceToIncident_;
    std::unordered_map<std::string, std::string> dispatcherToIncident_;
};

} // namespace aegis::coordinator::control_center

