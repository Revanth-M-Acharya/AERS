#include "emergency_call_handler.hpp"

namespace aegis {
namespace coordinator {

IncidentReport EmergencyCallHandler::IngestCall(const std::string& /*caller*/,
                                                const common::GeoCoordinate& loc,
                                                const std::string& /*description*/) {
    IncidentReport report;
    report.incidentId = "INC-TEST-001";
    report.location = loc;
    report.triage = common::TriageCategory::CODE_RED;
    report.incidentType = "CARDIAC";
    return report;
}

} // namespace coordinator
} // namespace aegis
