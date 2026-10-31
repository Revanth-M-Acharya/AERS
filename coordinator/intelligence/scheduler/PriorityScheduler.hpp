/**
 * @file PriorityScheduler.hpp
 * @brief Priority scheduler with FIFO tie-breaking, starvation prevention, and priority inheritance.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/intelligence/scheduler/IPriorityScheduler.hpp"
#include <unordered_map>
#include <vector>
#include <mutex>

namespace aegis::coordinator::intelligence {

class PriorityScheduler : public IPriorityScheduler {
public:
    explicit PriorityScheduler(uint64_t starvationThresholdMs = 10000);
    ~PriorityScheduler() override = default;

    bool RegisterAmbulance(const AmbulanceRequest& request) override;
    bool DeregisterAmbulance(const std::string& ambulanceId) override;
    bool UpdateAmbulancePriority(const std::string& ambulanceId, EmergencyPriorityLevel newPriority) override;

    std::vector<AmbulanceRequest> ScheduleAmbulances(uint64_t currentMonoMs) override;

    bool ReserveIntersection(const std::string& intersectionId,
                             const std::string& ambulanceId,
                             uint64_t holdStartMs,
                             uint64_t holdEndMs) override;

    void ReleaseIntersection(const std::string& intersectionId, const std::string& ambulanceId) override;

    bool IsIntersectionReserved(const std::string& intersectionId,
                                uint64_t atTimeMs,
                                const std::string& requestingAmbulanceId) const override;

    uint32_t ComputeEffectivePriority(const AmbulanceRequest& req, uint64_t currentMonoMs) const;

    size_t GetActiveAmbulanceCount() const;
    size_t GetActiveReservationCount() const;
    void Clear();

private:
    bool DetectCircularDependency(const std::string& requestingAmb, const std::string& holdingAmb) const;

    uint64_t starvationThresholdMs_{10000};
    mutable std::mutex mutex_;
    std::unordered_map<std::string, AmbulanceRequest> requests_;
    std::vector<IntersectionReservation> reservations_;
};

} // namespace aegis::coordinator::intelligence

