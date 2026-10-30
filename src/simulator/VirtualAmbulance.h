/**
 * @file VirtualAmbulance.h
 * @brief SMART AEGIS (AERS) Phase 19 - Autonomous Emergency Ambulance Simulation & Lifecycle Engine.
 */

#ifndef AEGIS_SIMULATOR_VIRTUAL_AMBULANCE_H
#define AEGIS_SIMULATOR_VIRTUAL_AMBULANCE_H

#include "VirtualVehicle.h"
#include <string>
#include <vector>
#include <cstdint>

namespace aegis {
namespace simulator {

/**
 * @brief Sequential lifecycle progression stages for an emergency ambulance mission.
 */
enum class AmbulanceLifecycle : uint8_t {
    IDLE = 0,
    DISPATCH,
    EN_ROUTE_TO_PATIENT,
    PATIENT_PICKUP,
    TRANSPORT,
    HOSPITAL_ARRIVAL,
    MISSION_COMPLETE,
    RETURN_TO_BASE
};

std::string AmbulanceLifecycleToString(AmbulanceLifecycle stage);

/**
 * @brief Clinical patient telemetry monitored onboard the virtual ambulance.
 */
struct SimulatedPatientVitals {
    std::string patientId{"PAT_001"};
    std::string name{"Patient"};
    uint16_t age{55};
    std::string gender{"Male"};
    uint16_t heartRateBpm{135};
    uint16_t spo2Pct{88};
    uint16_t systolicBP{85};
    uint16_t diastolicBP{55};
    uint16_t gcsScore{14};
};

/**
 * @brief Autonomous emergency vehicle implementing full mission lifecycle and vitals telemetry.
 */
class VirtualAmbulance : public BaseVehicle {
public:
    explicit VirtualAmbulance(std::string id, double stationLat = 12.9680, double stationLon = 77.5900);
    ~VirtualAmbulance() override = default;

    void Update(double dtSec) override;

    // Mission Control
    void Dispatch(const std::string& missionId,
                  const std::string& incidentId,
                  double pickupLat,
                  double pickupLon,
                  const std::string& hospitalId,
                  double hospitalLat,
                  double hospitalLon,
                  const SimulatedPatientVitals& vitals,
                  const std::vector<std::pair<double, double>>& routeToHospital);

    void ResetToBase();

    // Getters & Setters
    AmbulanceLifecycle GetLifecycle() const { return m_lifecycle; }
    void SetLifecycle(AmbulanceLifecycle stage) { m_lifecycle = stage; }

    std::string GetCurrentMissionId() const { return m_currentMissionId; }
    std::string GetCurrentIncidentId() const { return m_currentIncidentId; }
    std::string GetAssignedHospitalId() const { return m_assignedHospitalId; }
    SimulatedPatientVitals GetVitals() const { return m_patientVitals; }
    double GetEtaSeconds() const { return m_etaSeconds; }
    double GetDistanceRemainingMeters() const { return m_distanceRemainingMeters; }
    double GetFuelPercent() const { return m_fuelPercent; }
    bool HasPatientOnboard() const { return m_hasPatientOnboard; }

private:
    AmbulanceLifecycle m_lifecycle{AmbulanceLifecycle::IDLE};
    double m_baseLat{12.9680};
    double m_baseLon{77.5900};

    std::string m_currentMissionId;
    std::string m_currentIncidentId;
    std::string m_assignedHospitalId;
    double m_hospitalLat{12.9750};
    double m_hospitalLon{77.5985};

    SimulatedPatientVitals m_patientVitals;
    bool m_hasPatientOnboard{false};
    double m_etaSeconds{0.0};
    double m_distanceRemainingMeters{0.0};
    double m_fuelPercent{96.0};

    std::vector<std::pair<double, double>> m_patientToHospitalRoute;
    double m_pickupWaitTimer{0.0};
    double m_hospitalWaitTimer{0.0};
};

} // namespace simulator
} // namespace aegis

#endif // AEGIS_SIMULATOR_VIRTUAL_AMBULANCE_H

