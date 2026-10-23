/**
 * @file emergency_call_handler.hpp
 * @brief Emergency call intake, input validation, and triage qualification.
 */

#ifndef AEGIS_CORE_EMERGENCY_CALL_HANDLER_HPP
#define AEGIS_CORE_EMERGENCY_CALL_HANDLER_HPP

#include <string>
#include <vector>
#include <cstdint>
#include "aegis/common/types.hpp"
#include "aegis/common/geo_utils.hpp"
#include "aegis/common/result.hpp"

namespace aegis {
namespace core {
namespace coordinator {

struct EmergencyCallInput {
    std::string callerName;
    std::string callerContact;
    common::GeoCoordinate location;
    std::string description;
    std::string incidentTypeHint;
    std::vector<std::string> requestedCapabilities;
};

struct IncidentReport {
    std::string incidentId;
    uint64_t timestamp{0};
    std::string callerName;
    std::string callerContact;
    common::GeoCoordinate location;
    common::TriageCategory triage{common::TriageCategory::CODE_GREEN};
    std::string incidentType;
    std::vector<std::string> requiredCapabilities;
    std::string description;
};

class EmergencyCallHandler {
public:
    EmergencyCallHandler();
    ~EmergencyCallHandler() = default;

    // Metropolitan geofence configuration
    void SetGeofence(double minLat, double maxLat, double minLon, double maxLon) noexcept;

    // Full qualification API
    common::Result<IncidentReport> QualifyCall(const EmergencyCallInput& input) const;

    // Backward-compatible ingestion API
    IncidentReport IngestCall(const std::string& caller,
                              const common::GeoCoordinate& location,
                              const std::string& description);

    // Triage rule qualification
    common::TriageCategory EvaluateTriage(const std::string& description,
                                          const std::string& hint) const noexcept;

    std::vector<std::string> InferRequiredCapabilities(common::TriageCategory triage,
                                                       const std::string& incidentType) const;

private:
    double m_minLat{12.70};
    double m_maxLat{13.30};
    double m_minLon{77.30};
    double m_maxLon{77.90};
};

} // namespace coordinator
} // namespace core
} // namespace aegis

#endif // AEGIS_CORE_EMERGENCY_CALL_HANDLER_HPP
