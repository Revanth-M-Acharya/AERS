#ifndef AEGIS_CORE_EMERGENCY_CALL_HANDLER_HPP
#define AEGIS_CORE_EMERGENCY_CALL_HANDLER_HPP

#include <string>
#include "aegis/common/types.hpp"

namespace aegis {
namespace core {
namespace coordinator {

struct IncidentReport {
    std::string incidentId;
    common::GeoCoordinate location;
    common::TriageCategory triage{common::TriageCategory::CODE_GREEN};
    std::string incidentType;
};

/**
 * @brief Ingests, validates, and qualifies 911 distress calls and automated crash reports.
 * TODO: Implement boundary checking and standardized triage qualification.
 */
class EmergencyCallHandler {
public:
    EmergencyCallHandler();
    ~EmergencyCallHandler();

    IncidentReport IngestCall(const std::string& caller, const common::GeoCoordinate& location,
                              const std::string& description);
};

} // namespace coordinator
} // namespace core
} // namespace aegis

#endif // AEGIS_CORE_EMERGENCY_CALL_HANDLER_HPP
