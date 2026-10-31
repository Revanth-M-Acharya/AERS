/**
 * @file EmergencyPriorityEngine.hpp
 * @brief Clinical priority scoring and ambulance class dispatch recommendation engine.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/control_center/EmergencyValidator.hpp"
#include <string>
#include <cstdint>

namespace aegis::coordinator::control_center {

enum class PriorityLevel : uint32_t {
    Priority1 = 1, // Critical (Immediate life threat)
    Priority2 = 2, // Severe (High risk)
    Priority3 = 3, // Moderate (Urgent)
    Priority4 = 4  // Low / Minor (Non-emergency)
};

inline std::string PriorityLevelToString(PriorityLevel prio) {
    switch (prio) {
        case PriorityLevel::Priority1: return "Priority 1 (Critical)";
        case PriorityLevel::Priority2: return "Priority 2 (Severe)";
        case PriorityLevel::Priority3: return "Priority 3 (Moderate)";
        case PriorityLevel::Priority4: return "Priority 4 (Low)";
        default:                       return "Priority Unknown";
    }
}

enum class AmbulanceClass {
    ALS,   // Advanced Life Support (Paramedic, cardiac monitor, intubation)
    BLS,   // Basic Life Support (EMT, oxygen, AED, basic splinting)
    MICU,  // Mobile Intensive Care Unit (Critical care transport, ventilators)
    NEO    // Neonatal / Pediatric Intensive Care Unit
};

inline std::string AmbulanceClassToString(AmbulanceClass ac) {
    switch (ac) {
        case AmbulanceClass::ALS:  return "ALS";
        case AmbulanceClass::BLS:  return "BLS";
        case AmbulanceClass::MICU: return "MICU";
        case AmbulanceClass::NEO:  return "NEO";
        default:                   return "UNKNOWN";
    }
}

class EmergencyPriorityEngine {
public:
    EmergencyPriorityEngine() = default;
    ~EmergencyPriorityEngine() = default;

    /**
     * @brief Compute final priority level based on clinical criteria and logistics.
     */
    PriorityLevel CalculatePriority(const EmergencyRequest& req,
                                    double estimatedDistanceKm = 0.0,
                                    bool hospitalDirectCapability = true) const;

    /**
     * @brief Determine optimal ambulance specification.
     */
    AmbulanceClass DetermineAmbulanceClass(const EmergencyRequest& req,
                                           PriorityLevel priority) const;

    /**
     * @brief Map emergency type to required specialized medical department.
     */
    std::string DetermineRequiredDepartment(EmergencyType type) const;
};

} // namespace aegis::coordinator::control_center

