/**
 * @file EmergencyGenerator.h
 * @brief SMART AEGIS (AERS) Phase 19 - Stochastic & Deterministic Emergency Incident Generator.
 */

#ifndef AEGIS_SIMULATOR_EMERGENCY_GENERATOR_H
#define AEGIS_SIMULATOR_EMERGENCY_GENERATOR_H

#include "VirtualAmbulance.h"
#include <string>
#include <vector>
#include <random>
#include <mutex>

namespace aegis {
namespace simulator {

/**
 * @brief Medical emergency clinical classification.
 */
enum class EmergencyType : uint8_t {
    CARDIAC_ARREST = 0,
    HEART_ATTACK,
    ACUTE_STROKE,
    SEVERE_TRAUMA,
    ROAD_ACCIDENT,
    PEDIATRIC_EMERGENCY,
    STRUCTURE_FIRE,
    MASS_CASUALTY
};

std::string EmergencyTypeToString(EmergencyType type);

/**
 * @brief Complete diagnostic record of a generated emergency call.
 */
struct GeneratedEmergency {
    std::string incidentId;
    std::string callId;
    EmergencyType type;
    std::string typeString;
    std::string description;
    std::string priorityLevel{"P1_CRITICAL"};
    double clinicalPriorityScore{90.0};

    double latitude{12.9716};
    double longitude{77.5946};
    std::string locationName{"Brigade Road / MG Road Junction"};

    std::string requiredHospitalCapability{"LEVEL_1_TRAUMA"};
    bool requiresIcu{true};
    bool requiresAls{true};

    SimulatedPatientVitals patient;
    std::string timestampIso;
};

/**
 * @brief Generates clinically realistic emergency calls for simulation playback.
 */
class EmergencyGenerator {
public:
    explicit EmergencyGenerator(uint32_t seed = 42);
    ~EmergencyGenerator() = default;

    GeneratedEmergency GenerateRandomEmergency();
    GeneratedEmergency GenerateCardiacArrest(double lat = 12.9716, double lon = 77.5946);
    GeneratedEmergency GenerateStroke(double lat = 12.9725, double lon = 77.5958);
    GeneratedEmergency GenerateRoadAccident(double lat = 12.9738, double lon = 77.5972);
    GeneratedEmergency GenerateMassCasualty(double lat = 12.9720, double lon = 77.5950);

private:
    std::string NextIncidentId();

    mutable std::mutex m_mutex;
    std::mt19937 m_rng;
    uint64_t m_incidentCounter{0};
};

} // namespace simulator
} // namespace aegis

#endif // AEGIS_SIMULATOR_EMERGENCY_GENERATOR_H

