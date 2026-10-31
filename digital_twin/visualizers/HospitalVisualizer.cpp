/**
 * @file HospitalVisualizer.cpp
 * @brief Implementation of HospitalVisualizer.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "digital_twin/visualizers/HospitalVisualizer.hpp"
#include <sstream>
#include <iomanip>

namespace aegis::digital_twin::visualizers {

std::string HospitalVisualizer::RenderAsciiHospitalBoard(const std::vector<HospitalFacility>& hospitals) {
    std::ostringstream ss;
    ss << "=== REGIONAL HOSPITAL EMERGENCY STAGING ========================================\n";
    if (hospitals.empty()) {
        ss << "  (No hospitals registered)\n";
    } else {
        for (const auto& h : hospitals) {
            ss << " [HOSP] " << std::left << std::setw(18) << h.hospitalId
               << " [" << std::setw(28) << h.name << "]\n"
               << "        Trauma Level: " << h.traumaLevel
               << " | Available Beds: " << h.availableBeds << "/" << h.totalBeds
               << " | ICU: " << h.availableIcuBeds << "/" << h.totalIcuBeds << "\n"
               << "        ER Capacity : " << std::fixed << std::setprecision(1) << h.erCapacityPercent << "%"
               << " | Triage Score: " << std::setprecision(1) << h.triageReadinessScore << "/100"
               << " | Incoming: " << h.incomingPatientsCount << " unit(s)"
               << (h.isDiversionActive ? " [DIVERSION ACTIVE]" : " [OPEN]") << "\n";
        }
    }
    ss << "================================================================================\n";
    return ss.str();
}

std::string HospitalVisualizer::ToJson(const HospitalFacility& hospital) {
    std::ostringstream ss;
    ss << "{\"id\":\"" << hospital.hospitalId << "\",\"name\":\"" << hospital.name
       << "\",\"level\":" << hospital.traumaLevel
       << ",\"beds\":" << hospital.availableBeds << ",\"icu\":" << hospital.availableIcuBeds
       << ",\"erCap\":" << std::fixed << std::setprecision(1) << hospital.erCapacityPercent
       << ",\"diversion\":" << (hospital.isDiversionActive ? "true" : "false") << "}";
    return ss.str();
}

} // namespace aegis::digital_twin::visualizers

