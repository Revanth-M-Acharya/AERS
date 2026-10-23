/**
 * @file incident_manager.hpp
 * @brief Thread-safe incident lifecycle storage and tracking.
 */

#ifndef AEGIS_CORE_INCIDENT_MANAGER_HPP
#define AEGIS_CORE_INCIDENT_MANAGER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include "aegis/core/emergency_call_handler.hpp"
#include "aegis/common/result.hpp"

namespace aegis {
namespace core {
namespace coordinator {

enum class IncidentStatus : uint8_t {
    REPORTED = 0,
    ASSIGNED,
    EN_ROUTE_SCENE,
    ON_SCENE,
    EN_ROUTE_HOSPITAL,
    RESOLVED,
    CANCELLED
};

struct IncidentRecord {
    std::string incidentId;
    uint64_t createdAtMs{0};
    uint64_t assignedAtMs{0};
    uint64_t resolvedAtMs{0};
    std::string callerName;
    std::string callerContact;
    common::GeoCoordinate location;
    common::TriageCategory triageCategory{common::TriageCategory::CODE_GREEN};
    std::string incidentType;
    std::vector<std::string> requiredCapabilities;
    std::string description;
    std::string assignedUnitId;
    std::string activeRouteId;
    IncidentStatus status{IncidentStatus::REPORTED};
};

class IncidentManager {
public:
    IncidentManager() = default;
    ~IncidentManager() = default;

    common::Result<std::string> CreateIncident(const IncidentReport& report);
    common::Result<IncidentRecord> GetIncident(const std::string& incidentId) const;
    bool UpdateStatus(const std::string& incidentId, IncidentStatus newStatus);
    bool AssignAmbulance(const std::string& incidentId, const std::string& unitId, const std::string& routeId);
    
    std::vector<IncidentRecord> GetActiveIncidents() const;
    std::vector<IncidentRecord> GetAllIncidents() const;
    size_t GetIncidentCount() const noexcept;
    void Clear();

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, IncidentRecord> m_incidents;
};

} // namespace coordinator
} // namespace core
} // namespace aegis

#endif // AEGIS_CORE_INCIDENT_MANAGER_HPP
