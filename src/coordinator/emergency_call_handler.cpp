#include "aegis/core/emergency_call_handler.hpp"

namespace aegis {
namespace core {
namespace coordinator {

EmergencyCallHandler::EmergencyCallHandler() = default;
EmergencyCallHandler::~EmergencyCallHandler() = default;

IncidentReport EmergencyCallHandler::IngestCall(const std::string& /*caller*/,
                                                const common::GeoCoordinate& location,
                                                const std::string& /*description*/) {
    IncidentReport report;
    report.incidentId = "INC-SKELETON-001";
    report.location = location;
    report.triage = common::TriageCategory::CODE_RED;
    report.incidentType = "CARDIAC";
    // TODO: Validate coordinates against city boundary and assign standardized triage code
    return report;
}

} // namespace coordinator
} // namespace core
} // namespace aegis
