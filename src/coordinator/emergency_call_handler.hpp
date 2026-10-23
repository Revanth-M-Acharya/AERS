#ifndef AEGIS_COORDINATOR_EMERGENCY_CALL_HANDLER_HPP
#define AEGIS_COORDINATOR_EMERGENCY_CALL_HANDLER_HPP

#include <string>
#include "aegis/common/types.hpp"

namespace aegis {
namespace coordinator {

struct IncidentReport {
    std::string incidentId;
    common::GeoCoordinate location;
    common::TriageCategory triage;
    std::string incidentType;
};

class EmergencyCallHandler {
public:
    EmergencyCallHandler() = default;
    ~EmergencyCallHandler() = default;

    IncidentReport IngestCall(const std::string& caller, const common::GeoCoordinate& loc,
                              const std::string& description);
};

} // namespace coordinator
} // namespace aegis

#endif // AEGIS_COORDINATOR_EMERGENCY_CALL_HANDLER_HPP
