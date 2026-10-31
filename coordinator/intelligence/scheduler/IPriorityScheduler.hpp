/**
 * @file IPriorityScheduler.hpp
 * @brief Abstract interface for multi-ambulance priority scheduling and intersection reservation.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace aegis::coordinator::intelligence {

enum class EmergencyPriorityLevel : uint32_t {
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3,
    CRITICAL = 4
};

inline std::string PriorityLevelToString(EmergencyPriorityLevel level) {
    switch (level) {
        case EmergencyPriorityLevel::LOW: return "LOW";
        case EmergencyPriorityLevel::MEDIUM: return "MEDIUM";
        case EmergencyPriorityLevel::HIGH: return "HIGH";
        case EmergencyPriorityLevel::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

struct AmbulanceRequest {
    std::string ambulanceId;
    EmergencyPriorityLevel priority{EmergencyPriorityLevel::MEDIUM};
    std::string startIntersection;
    std::string destinationIntersection;
    uint64_t requestTimeMonoMs{0};
    uint64_t lastScheduleTimeMonoMs{0};
    uint32_t priorityInheritanceLevel{0};
};

struct IntersectionReservation {
    std::string intersectionId;
    std::string ambulanceId;
    uint64_t holdStartMonoMs{0};
    uint64_t holdEndMonoMs{0};
};

class IPriorityScheduler {
public:
    virtual ~IPriorityScheduler() = default;

    virtual bool RegisterAmbulance(const AmbulanceRequest& request) = 0;
    virtual bool DeregisterAmbulance(const std::string& ambulanceId) = 0;
    virtual bool UpdateAmbulancePriority(const std::string& ambulanceId, EmergencyPriorityLevel newPriority) = 0;

    /**
     * @brief Produce ordered execution ranking for all active emergency vehicles.
     */
    virtual std::vector<AmbulanceRequest> ScheduleAmbulances(uint64_t currentMonoMs) = 0;

    virtual bool ReserveIntersection(const std::string& intersectionId,
                                     const std::string& ambulanceId,
                                     uint64_t holdStartMs,
                                     uint64_t holdEndMs) = 0;

    virtual void ReleaseIntersection(const std::string& intersectionId, const std::string& ambulanceId) = 0;

    virtual bool IsIntersectionReserved(const std::string& intersectionId,
                                        uint64_t atTimeMs,
                                        const std::string& requestingAmbulanceId) const = 0;
};

} // namespace aegis::coordinator::intelligence

