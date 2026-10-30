/**
 * @file EmergencyGenerator.cpp
 * @brief SMART AEGIS (AERS) Phase 19 - Stochastic & Deterministic Emergency Incident Generator Implementation.
 */

#include "EmergencyGenerator.h"
#include <iomanip>
#include <sstream>

namespace aegis {
namespace simulator {

std::string EmergencyTypeToString(EmergencyType type) {
    switch (type) {
        case EmergencyType::CARDIAC_ARREST: return "CARDIAC_ARREST";
        case EmergencyType::HEART_ATTACK: return "HEART_ATTACK";
        case EmergencyType::ACUTE_STROKE: return "ACUTE_STROKE";
        case EmergencyType::SEVERE_TRAUMA: return "SEVERE_TRAUMA";
        case EmergencyType::ROAD_ACCIDENT: return "ROAD_ACCIDENT";
        case EmergencyType::PEDIATRIC_EMERGENCY: return "PEDIATRIC_EMERGENCY";
        case EmergencyType::STRUCTURE_FIRE: return "STRUCTURE_FIRE";
        case EmergencyType::MASS_CASUALTY: return "MASS_CASUALTY";
    }
    return "UNKNOWN";
}

EmergencyGenerator::EmergencyGenerator(uint32_t seed)
    : m_rng(seed)
{
}

std::string EmergencyGenerator::NextIncidentId() {
    uint64_t idx = ++m_incidentCounter;
    std::ostringstream oss;
    oss << "INC_20261029_" << std::setfill('0') << std::setw(4) << idx;
    return oss.str();
}

GeneratedEmergency EmergencyGenerator::GenerateCardiacArrest(double lat, double lon) {
    std::lock_guard<std::mutex> lock(m_mutex);
    GeneratedEmergency e;
    e.incidentId = NextIncidentId();
    e.callId = "CALL_" + std::to_string(m_incidentCounter);
    e.type = EmergencyType::CARDIAC_ARREST;
    e.typeString = "CARDIAC_ARREST";
    e.description = "Out-of-hospital acute ventricular fibrillation and cardiac arrest with CPR in progress";
    e.priorityLevel = "P1_CRITICAL";
    e.clinicalPriorityScore = 96.0;
    e.latitude = lat;
    e.longitude = lon;
    e.locationName = "Brigade Road / MG Road Junction";
    e.requiredHospitalCapability = "CARDIAC_CATH_LAB";
    e.requiresIcu = true;
    e.requiresAls = true;

    e.patient.name = "Robert Vance";
    e.patient.age = 58;
    e.patient.gender = "Male";
    e.patient.heartRateBpm = 145;
    e.patient.spo2Pct = 84;
    e.patient.systolicBP = 75;
    e.patient.diastolicBP = 45;
    e.patient.gcsScore = 6;
    e.timestampIso = "08:35:10";
    return e;
}

GeneratedEmergency EmergencyGenerator::GenerateStroke(double lat, double lon) {
    std::lock_guard<std::mutex> lock(m_mutex);
    GeneratedEmergency e;
    e.incidentId = NextIncidentId();
    e.callId = "CALL_" + std::to_string(m_incidentCounter);
    e.type = EmergencyType::ACUTE_STROKE;
    e.typeString = "ACUTE_STROKE";
    e.description = "Acute ischemic stroke with right hemiparesis and sudden severe aphasia";
    e.priorityLevel = "P1_CRITICAL";
    e.clinicalPriorityScore = 92.0;
    e.latitude = lat;
    e.longitude = lon;
    e.locationName = "MG Road / Residency Road Junction";
    e.requiredHospitalCapability = "STROKE_INTERVENTION";
    e.requiresIcu = true;
    e.requiresAls = true;

    e.patient.name = "Eleanor Campbell";
    e.patient.age = 67;
    e.patient.gender = "Female";
    e.patient.heartRateBpm = 98;
    e.patient.spo2Pct = 96;
    e.patient.systolicBP = 185;
    e.patient.diastolicBP = 110;
    e.patient.gcsScore = 11;
    e.timestampIso = "08:42:25";
    return e;
}

GeneratedEmergency EmergencyGenerator::GenerateRoadAccident(double lat, double lon) {
    std::lock_guard<std::mutex> lock(m_mutex);
    GeneratedEmergency e;
    e.incidentId = NextIncidentId();
    e.callId = "CALL_" + std::to_string(m_incidentCounter);
    e.type = EmergencyType::ROAD_ACCIDENT;
    e.typeString = "ROAD_ACCIDENT";
    e.description = "High-speed multi-vehicle collision with severe blunt thoracic trauma";
    e.priorityLevel = "P1_CRITICAL";
    e.clinicalPriorityScore = 94.5;
    e.latitude = lat;
    e.longitude = lon;
    e.locationName = "Hospital Approach Avenue";
    e.requiredHospitalCapability = "LEVEL_1_TRAUMA";
    e.requiresIcu = true;
    e.requiresAls = true;

    e.patient.name = "David Miller";
    e.patient.age = 34;
    e.patient.gender = "Male";
    e.patient.heartRateBpm = 128;
    e.patient.spo2Pct = 89;
    e.patient.systolicBP = 88;
    e.patient.diastolicBP = 52;
    e.patient.gcsScore = 9;
    e.timestampIso = "08:50:00";
    return e;
}

GeneratedEmergency EmergencyGenerator::GenerateMassCasualty(double lat, double lon) {
    std::lock_guard<std::mutex> lock(m_mutex);
    GeneratedEmergency e;
    e.incidentId = NextIncidentId();
    e.callId = "CALL_" + std::to_string(m_incidentCounter);
    e.type = EmergencyType::MASS_CASUALTY;
    e.typeString = "MASS_CASUALTY";
    e.description = "Major commercial structural collapse resulting in multiple severe trauma victims";
    e.priorityLevel = "P1_CRITICAL";
    e.clinicalPriorityScore = 99.0;
    e.latitude = lat;
    e.longitude = lon;
    e.locationName = "Central Business District";
    e.requiredHospitalCapability = "LEVEL_1_TRAUMA";
    e.requiresIcu = true;
    e.requiresAls = true;

    e.patient.name = "Multiple Patients";
    e.patient.age = 40;
    e.patient.gender = "Unspecified";
    e.patient.heartRateBpm = 138;
    e.patient.spo2Pct = 87;
    e.patient.systolicBP = 80;
    e.patient.diastolicBP = 50;
    e.patient.gcsScore = 8;
    e.timestampIso = "08:55:12";
    return e;
}

GeneratedEmergency EmergencyGenerator::GenerateRandomEmergency() {
    std::uniform_int_distribution<int> dist(0, 3);
    int choice = dist(m_rng);
    switch (choice) {
        case 0: return GenerateCardiacArrest();
        case 1: return GenerateStroke();
        case 2: return GenerateRoadAccident();
        default: return GenerateMassCasualty();
    }
}

} // namespace simulator
} // namespace aegis

