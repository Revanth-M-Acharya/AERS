/**
 * @file EmergencyValidator.hpp
 * @brief Validation engine for incoming emergency calls and triage requests.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <cstdint>

namespace aegis::coordinator::control_center {

enum class EmergencyType {
    Cardiac,
    Stroke,
    Trauma,
    Fire,
    Accident,
    Pregnancy,
    Pediatric,
    MassCasualty,
    GeneralEmergency
};

inline std::string EmergencyTypeToString(EmergencyType type) {
    switch (type) {
        case EmergencyType::Cardiac:          return "Cardiac";
        case EmergencyType::Stroke:           return "Stroke";
        case EmergencyType::Trauma:           return "Trauma";
        case EmergencyType::Fire:             return "Fire";
        case EmergencyType::Accident:         return "Accident";
        case EmergencyType::Pregnancy:        return "Pregnancy";
        case EmergencyType::Pediatric:        return "Pediatric";
        case EmergencyType::MassCasualty:     return "Mass Casualty";
        case EmergencyType::GeneralEmergency: return "General Emergency";
        default:                              return "Unknown";
    }
}

enum class PatientSeverity {
    CRITICAL,    // Immediate life-threat (Priority 1)
    SEVERE,      // High risk of deterioration (Priority 2)
    MODERATE,    // Urgent care required (Priority 3)
    MINOR        // Non-life-threatening (Priority 4)
};

inline std::string PatientSeverityToString(PatientSeverity severity) {
    switch (severity) {
        case PatientSeverity::CRITICAL: return "CRITICAL";
        case PatientSeverity::SEVERE:   return "SEVERE";
        case PatientSeverity::MODERATE: return "MODERATE";
        case PatientSeverity::MINOR:    return "MINOR";
        default:                        return "UNKNOWN";
    }
}

struct EmergencyRequest {
    std::string callerId;
    std::string callerPhone;
    std::string locationDescription;
    double latitude{0.0};
    double longitude{0.0};
    std::string closestIntersectionId;
    EmergencyType type{EmergencyType::GeneralEmergency};
    PatientSeverity severity{PatientSeverity::MODERATE};
    uint32_t patientAgeYears{30};
    std::string symptomsDescription;
    uint32_t casualtyCount{1};
    uint64_t requestTimestampMonoMs{0};
};

struct ValidationResult {
    bool isValid{false};
    std::string rejectionReason;
    std::string sanitizedLocation;
};

class EmergencyValidator {
public:
    EmergencyValidator() = default;
    ~EmergencyValidator() = default;

    /**
     * @brief Validate emergency call payload.
     * @return ValidationResult with pass/fail and descriptive reason.
     */
    ValidationResult Validate(const EmergencyRequest& req) const;

    void SetMaxResponseDistanceKm(double maxKm) { maxDistanceKm_ = maxKm; }
    double GetMaxResponseDistanceKm() const { return maxDistanceKm_; }

private:
    double maxDistanceKm_{50.0};
};

} // namespace aegis::coordinator::control_center

