/**
 * @file VirtualAmbulance.cpp
 * @brief SMART AEGIS (AERS) Phase 19 - Autonomous Emergency Ambulance Simulation & Lifecycle Engine Implementation.
 */

#include "VirtualAmbulance.h"
#include <cmath>

namespace aegis {
namespace simulator {

std::string AmbulanceLifecycleToString(AmbulanceLifecycle stage) {
    switch (stage) {
        case AmbulanceLifecycle::IDLE: return "IDLE";
        case AmbulanceLifecycle::DISPATCH: return "DISPATCH";
        case AmbulanceLifecycle::EN_ROUTE_TO_PATIENT: return "EN_ROUTE_TO_PATIENT";
        case AmbulanceLifecycle::PATIENT_PICKUP: return "PATIENT_PICKUP";
        case AmbulanceLifecycle::TRANSPORT: return "TRANSPORT";
        case AmbulanceLifecycle::HOSPITAL_ARRIVAL: return "HOSPITAL_ARRIVAL";
        case AmbulanceLifecycle::MISSION_COMPLETE: return "MISSION_COMPLETE";
        case AmbulanceLifecycle::RETURN_TO_BASE: return "RETURN_TO_BASE";
    }
    return "UNKNOWN";
}

VirtualAmbulance::VirtualAmbulance(std::string id, double stationLat, double stationLon)
    : BaseVehicle(std::move(id), VehicleType::EMERGENCY_AMBULANCE, 65.0)
    , m_baseLat(stationLat)
    , m_baseLon(stationLon)
{
    m_lat = stationLat;
    m_lon = stationLon;
}

void VirtualAmbulance::ResetToBase() {
    m_lat = m_baseLat;
    m_lon = m_baseLon;
    m_speedKmh = 0.0;
    m_headingDeg = 0.0;
    m_lifecycle = AmbulanceLifecycle::IDLE;
    m_hasPatientOnboard = false;
    m_currentMissionId.clear();
    m_currentIncidentId.clear();
    m_assignedHospitalId.clear();
    m_etaSeconds = 0.0;
    m_distanceRemainingMeters = 0.0;
    m_waypoints.clear();
}

void VirtualAmbulance::Dispatch(const std::string& missionId,
                               const std::string& incidentId,
                               double pickupLat,
                               double pickupLon,
                               const std::string& hospitalId,
                               double hospitalLat,
                               double hospitalLon,
                               const SimulatedPatientVitals& vitals,
                               const std::vector<std::pair<double, double>>& routeToHospital) {
    m_currentMissionId = missionId;
    m_currentIncidentId = incidentId;
    m_assignedHospitalId = hospitalId;
    m_hospitalLat = hospitalLat;
    m_hospitalLon = hospitalLon;
    m_patientVitals = vitals;
    m_hasPatientOnboard = false;
    m_patientToHospitalRoute = routeToHospital;

    // Route from current position to pickup point
    std::vector<std::pair<double, double>> pickupRoute = {
        {m_lat, m_lon},
        {pickupLat, pickupLon}
    };
    SetRoute(pickupRoute);
    SetSpeedKmh(60.0);
    m_lifecycle = AmbulanceLifecycle::DISPATCH;

    double dLat = pickupLat - m_lat;
    double dLon = pickupLon - m_lon;
    m_distanceRemainingMeters = std::hypot(dLat, dLon) * 111000.0;
    m_etaSeconds = m_distanceRemainingMeters / (60.0 * 0.277778);
}

void VirtualAmbulance::Update(double dtSec) {
    // Fuel consumption
    if (m_speedKmh > 0.0) {
        m_fuelPercent -= (0.002 * dtSec);
        if (m_fuelPercent < 5.0) m_fuelPercent = 5.0;
    }

    switch (m_lifecycle) {
        case AmbulanceLifecycle::IDLE:
            m_speedKmh = 0.0;
            break;

        case AmbulanceLifecycle::DISPATCH:
            m_lifecycle = AmbulanceLifecycle::EN_ROUTE_TO_PATIENT;
            break;

        case AmbulanceLifecycle::EN_ROUTE_TO_PATIENT: {
            BaseVehicle::Update(dtSec);
            if (m_etaSeconds > 0.0) {
                m_etaSeconds -= dtSec;
                if (m_etaSeconds < 0.0) m_etaSeconds = 0.0;
            }
            if (HasArrived()) {
                m_lifecycle = AmbulanceLifecycle::PATIENT_PICKUP;
                m_pickupWaitTimer = 2.0; // 2 sec pickup stabilization
                m_speedKmh = 0.0;
            }
            break;
        }

        case AmbulanceLifecycle::PATIENT_PICKUP: {
            m_pickupWaitTimer -= dtSec;
            if (m_pickupWaitTimer <= 0.0) {
                m_hasPatientOnboard = true;
                m_lifecycle = AmbulanceLifecycle::TRANSPORT;
                SetRoute(m_patientToHospitalRoute);
                SetSpeedKmh(62.0);

                double dLat = m_hospitalLat - m_lat;
                double dLon = m_hospitalLon - m_lon;
                m_distanceRemainingMeters = std::hypot(dLat, dLon) * 111000.0;
                m_etaSeconds = m_distanceRemainingMeters / (62.0 * 0.277778);
            }
            break;
        }

        case AmbulanceLifecycle::TRANSPORT: {
            BaseVehicle::Update(dtSec);
            if (m_etaSeconds > 0.0) {
                m_etaSeconds -= dtSec;
                if (m_etaSeconds < 0.0) m_etaSeconds = 0.0;
            }
            if (HasArrived()) {
                m_lifecycle = AmbulanceLifecycle::HOSPITAL_ARRIVAL;
                m_hospitalWaitTimer = 2.0; // 2 sec triage handoff
                m_speedKmh = 0.0;
            }
            break;
        }

        case AmbulanceLifecycle::HOSPITAL_ARRIVAL: {
            m_hospitalWaitTimer -= dtSec;
            if (m_hospitalWaitTimer <= 0.0) {
                m_hasPatientOnboard = false;
                m_lifecycle = AmbulanceLifecycle::MISSION_COMPLETE;
            }
            break;
        }

        case AmbulanceLifecycle::MISSION_COMPLETE: {
            m_lifecycle = AmbulanceLifecycle::RETURN_TO_BASE;
            std::vector<std::pair<double, double>> returnRoute = {
                {m_lat, m_lon},
                {m_baseLat, m_baseLon}
            };
            SetRoute(returnRoute);
            SetSpeedKmh(45.0);
            break;
        }

        case AmbulanceLifecycle::RETURN_TO_BASE: {
            BaseVehicle::Update(dtSec);
            if (HasArrived()) {
                m_lifecycle = AmbulanceLifecycle::IDLE;
                m_speedKmh = 0.0;
            }
            break;
        }
    }
}

} // namespace simulator
} // namespace aegis
