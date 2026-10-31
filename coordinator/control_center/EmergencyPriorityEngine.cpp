/**
 * @file EmergencyPriorityEngine.cpp
 * @brief Implementation of priority calculation and ambulance class determination.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/control_center/EmergencyPriorityEngine.hpp"

namespace aegis::coordinator::control_center {

PriorityLevel EmergencyPriorityEngine::CalculatePriority(const EmergencyRequest& req,
                                                         double estimatedDistanceKm,
                                                         bool hospitalDirectCapability) const {
    // 1. Inherent Critical Condition Overrides (Always Priority 1)
    if (req.type == EmergencyType::Cardiac ||
        req.type == EmergencyType::Stroke ||
        req.type == EmergencyType::MassCasualty ||
        req.severity == PatientSeverity::CRITICAL) {
        return PriorityLevel::Priority1;
    }

    // 2. High Severity / Fast Progression Overrides
    if (req.type == EmergencyType::Trauma ||
        req.type == EmergencyType::Fire ||
        req.type == EmergencyType::Accident ||
        req.severity == PatientSeverity::SEVERE) {
        // If distance is large or specialized hospital not directly capable, escalate to Priority 1
        if (estimatedDistanceKm > 20.0 || !hospitalDirectCapability) {
            return PriorityLevel::Priority1;
        }
        return PriorityLevel::Priority2;
    }

    // 3. Moderate Conditions (Pregnancy, Pediatric, Moderate Symptoms)
    if (req.type == EmergencyType::Pregnancy ||
        req.type == EmergencyType::Pediatric ||
        req.severity == PatientSeverity::MODERATE) {
        if (!hospitalDirectCapability) {
            return PriorityLevel::Priority2; // Escalate due to capability shortfall
        }
        return PriorityLevel::Priority3;
    }

    // 4. Low Priority / General Emergencies
    return PriorityLevel::Priority4;
}

AmbulanceClass EmergencyPriorityEngine::DetermineAmbulanceClass(const EmergencyRequest& req,
                                                                PriorityLevel priority) const {
    // Neonatal / Infant
    if (req.type == EmergencyType::Pediatric && req.patientAgeYears <= 1) {
        return AmbulanceClass::NEO;
    }

    // Critical ICU Care
    if (req.type == EmergencyType::MassCasualty ||
        (priority == PriorityLevel::Priority1 && req.severity == PatientSeverity::CRITICAL)) {
        return AmbulanceClass::MICU;
    }

    // Advanced Life Support for Priority 1 and 2
    if (priority == PriorityLevel::Priority1 || priority == PriorityLevel::Priority2) {
        return AmbulanceClass::ALS;
    }

    // Basic Life Support for Priority 3 and 4
    return AmbulanceClass::BLS;
}

std::string EmergencyPriorityEngine::DetermineRequiredDepartment(EmergencyType type) const {
    switch (type) {
        case EmergencyType::Cardiac:          return "CARDIOLOGY";
        case EmergencyType::Stroke:           return "NEUROLOGY";
        case EmergencyType::Trauma:           return "TRAUMA";
        case EmergencyType::Fire:             return "BURN";
        case EmergencyType::Accident:         return "TRAUMA";
        case EmergencyType::Pregnancy:        return "OBSTETRICS";
        case EmergencyType::Pediatric:        return "PEDIATRICS";
        case EmergencyType::MassCasualty:     return "TRAUMA";
        case EmergencyType::GeneralEmergency: return "GENERAL";
        default:                              return "GENERAL";
    }
}

} // namespace aegis::coordinator::control_center

