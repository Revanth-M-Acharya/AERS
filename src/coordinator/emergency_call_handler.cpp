#include "aegis/core/emergency_call_handler.hpp"
#include "aegis/common/time_utils.hpp"
#include "aegis/common/uuid_utils.hpp"
#include "aegis/common/validator.hpp"
#include <algorithm>

namespace aegis {
namespace core {
namespace coordinator {

EmergencyCallHandler::EmergencyCallHandler() = default;

void EmergencyCallHandler::SetGeofence(double minLat, double maxLat, double minLon, double maxLon) noexcept {
    m_minLat = minLat;
    m_maxLat = maxLat;
    m_minLon = minLon;
    m_maxLon = maxLon;
}

common::TriageCategory EmergencyCallHandler::EvaluateTriage(const std::string& description,
                                                            const std::string& hint) const noexcept {
    if (hint == "CODE_RED") return common::TriageCategory::CODE_RED;
    if (hint == "CODE_AMBER") return common::TriageCategory::CODE_AMBER;
    if (hint == "CODE_GREEN") return common::TriageCategory::CODE_GREEN;

    std::string text = description;
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (text.find("cardiac") != std::string::npos ||
        text.find("heart") != std::string::npos ||
        text.find("unconscious") != std::string::npos ||
        text.find("stroke") != std::string::npos ||
        text.find("arrest") != std::string::npos ||
        text.find("severe bleeding") != std::string::npos ||
        text.find("choking") != std::string::npos ||
        text.find("critical") != std::string::npos ||
        text.find("crash") != std::string::npos) {
        return common::TriageCategory::CODE_RED;
    }

    if (text.find("fracture") != std::string::npos ||
        text.find("broken") != std::string::npos ||
        text.find("burn") != std::string::npos ||
        text.find("moderate") != std::string::npos ||
        text.find("asthma") != std::string::npos ||
        text.find("seizure") != std::string::npos) {
        return common::TriageCategory::CODE_AMBER;
    }

    return common::TriageCategory::CODE_GREEN;
}

std::vector<std::string> EmergencyCallHandler::InferRequiredCapabilities(
    common::TriageCategory triage,
    const std::string& incidentType) const {

    std::vector<std::string> caps;
    caps.push_back("OXYGEN");

    if (triage == common::TriageCategory::CODE_RED) {
        caps.push_back("ECG");
        caps.push_back("DEFIBRILLATOR");
        if (incidentType.find("CARDIAC") != std::string::npos || incidentType.find("ARREST") != std::string::npos) {
            caps.push_back("VENTILATOR");
        }
    } else if (triage == common::TriageCategory::CODE_AMBER) {
        caps.push_back("TRAUMA_KIT");
    }

    return caps;
}

common::Result<IncidentReport> EmergencyCallHandler::QualifyCall(const EmergencyCallInput& input) const {
    if (!common::IsValidCoordinate(input.location)) {
        return common::Result<IncidentReport>::Err(common::ErrorCode::ERR_INVALID_MESSAGE);
    }

    // Metropolitan geofence validation
    if (input.location.latitude < m_minLat || input.location.latitude > m_maxLat ||
        input.location.longitude < m_minLon || input.location.longitude > m_maxLon) {
        return common::Result<IncidentReport>::Err(common::ErrorCode::ERR_INVALID_MESSAGE);
    }

    if (input.callerContact.empty()) {
        return common::Result<IncidentReport>::Err(common::ErrorCode::ERR_INVALID_MESSAGE);
    }

    IncidentReport report;
    report.incidentId = common::GenerateIncidentId();
    report.timestamp = common::GetUtcMilliseconds();
    report.callerName = input.callerName.empty() ? "Anonymous" : input.callerName;
    report.callerContact = input.callerContact;
    report.location = input.location;
    report.description = input.description;

    std::string incType = input.incidentTypeHint.empty() ? "GENERAL_EMERGENCY" : input.incidentTypeHint;
    report.incidentType = incType;
    report.triage = EvaluateTriage(input.description, input.incidentTypeHint);

    if (!input.requestedCapabilities.empty()) {
        report.requiredCapabilities = input.requestedCapabilities;
    } else {
        report.requiredCapabilities = InferRequiredCapabilities(report.triage, incType);
    }

    return common::Result<IncidentReport>::Ok(report);
}

IncidentReport EmergencyCallHandler::IngestCall(const std::string& caller,
                                                const common::GeoCoordinate& location,
                                                const std::string& description) {
    EmergencyCallInput input;
    input.callerName = caller;
    input.callerContact = "+91-0000000000";
    input.location = location;
    input.description = description;

    auto res = QualifyCall(input);
    if (res.IsOk()) {
        return res.Value();
    }

    // Fallback safe default
    IncidentReport rep;
    rep.incidentId = common::GenerateIncidentId();
    rep.timestamp = common::GetUtcMilliseconds();
    rep.callerName = caller;
    rep.callerContact = input.callerContact;
    rep.location = location;
    rep.triage = common::TriageCategory::CODE_GREEN;
    rep.incidentType = "GENERAL_EMERGENCY";
    rep.description = description;
    return rep;
}

} // namespace coordinator
} // namespace core
} // namespace aegis
