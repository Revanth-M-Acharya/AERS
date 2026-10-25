/**
 * @file ambulance_mission_simulator.cpp
 * @brief Implementation of full incident ambulance mission simulation lifecycle.
 */

#include "aegis/edge/ambulance_mission_simulator.hpp"
#include "aegis/common/geo_utils.hpp"
#include "aegis/common/time_utils.hpp"
#include "aegis/logger/logger.hpp"

#include <cmath>

namespace aegis {
namespace edge {
namespace ambulance {

AmbulanceMissionSimulator::AmbulanceMissionSimulator(AmbulanceNode& node)
    : m_node(node) {}

void AmbulanceMissionSimulator::ConfigureMission(
    const std::string& incidentId,
    const common::GeoCoordinate& startLoc,
    const common::GeoCoordinate& patientLoc,
    const common::GeoCoordinate& hospitalLoc,
    const std::vector<common::GeoCoordinate>& toPatientWaypoints,
    const std::vector<common::GeoCoordinate>& toHospitalWaypoints) {
    m_incidentId = incidentId;
    m_startLoc = startLoc;
    m_patientLoc = patientLoc;
    m_hospitalLoc = hospitalLoc;

    m_toPatientWaypoints = toPatientWaypoints;
    if (m_toPatientWaypoints.empty()) {
        m_toPatientWaypoints.push_back(m_startLoc);
        // Interpolate an intermediate waypoint
        common::GeoCoordinate mid{
            (m_startLoc.latitude + m_patientLoc.latitude) / 2.0,
            (m_startLoc.longitude + m_patientLoc.longitude) / 2.0,
            0.0
        };
        m_toPatientWaypoints.push_back(mid);
        m_toPatientWaypoints.push_back(m_patientLoc);
    }

    m_toHospitalWaypoints = toHospitalWaypoints;
    if (m_toHospitalWaypoints.empty()) {
        m_toHospitalWaypoints.push_back(m_patientLoc);
        common::GeoCoordinate mid{
            (m_patientLoc.latitude + m_hospitalLoc.latitude) / 2.0,
            (m_patientLoc.longitude + m_hospitalLoc.longitude) / 2.0,
            0.0
        };
        m_toHospitalWaypoints.push_back(mid);
        m_toHospitalWaypoints.push_back(m_hospitalLoc);
    }

    m_phase = MissionPhase::IDLE;
    m_currentWaypointIdx = 0;
    m_onSceneDwellRemainingSec = 3.0f;
    m_arrivedDwellRemainingSec = 2.0f;
    m_speedKmH = 65.0f;

    m_node.Gps().InjectPosition(m_startLoc.latitude, m_startLoc.longitude, 0.0f, 0.0f);
}

std::string AmbulanceMissionSimulator::GetCurrentPhaseString() const {
    switch (m_phase) {
        case MissionPhase::IDLE: return "IDLE";
        case MissionPhase::DISPATCHED: return "DISPATCHED";
        case MissionPhase::EN_ROUTE_PATIENT: return "EN_ROUTE_PATIENT";
        case MissionPhase::ON_SCENE: return "ON_SCENE";
        case MissionPhase::EN_ROUTE_HOSPITAL: return "EN_ROUTE_HOSPITAL";
        case MissionPhase::ARRIVED: return "ARRIVED";
        case MissionPhase::COMPLETED: return "COMPLETED";
        default: return "UNKNOWN";
    }
}

float AmbulanceMissionSimulator::GetMissionProgress() const noexcept {
    switch (m_phase) {
        case MissionPhase::IDLE: return 0.0f;
        case MissionPhase::DISPATCHED: return 0.10f;
        case MissionPhase::EN_ROUTE_PATIENT: {
            if (m_toPatientWaypoints.size() <= 1) return 0.30f;
            float ratio = static_cast<float>(m_currentWaypointIdx) / static_cast<float>(m_toPatientWaypoints.size() - 1);
            return 0.10f + (ratio * 0.35f);
        }
        case MissionPhase::ON_SCENE: return 0.50f;
        case MissionPhase::EN_ROUTE_HOSPITAL: {
            if (m_toHospitalWaypoints.size() <= 1) return 0.75f;
            float ratio = static_cast<float>(m_currentWaypointIdx) / static_cast<float>(m_toHospitalWaypoints.size() - 1);
            return 0.55f + (ratio * 0.35f);
        }
        case MissionPhase::ARRIVED: return 0.95f;
        case MissionPhase::COMPLETED: return 1.0f;
        default: return 0.0f;
    }
}

bool AmbulanceMissionSimulator::StepSimulation(float deltaSeconds) {
    if (m_phase == MissionPhase::COMPLETED) {
        return false;
    }

    if (m_phase == MissionPhase::IDLE) {
        m_phase = MissionPhase::DISPATCHED;
        m_node.TransitionState(common::AmbulanceState::DISPATCHED);
        m_node.PublishStateOnce();
        return true;
    }

    if (m_phase == MissionPhase::DISPATCHED) {
        m_phase = MissionPhase::EN_ROUTE_PATIENT;
        m_currentWaypointIdx = 0;
        m_node.TransitionState(common::AmbulanceState::EN_ROUTE_PATIENT);
        m_node.PublishStateOnce();
        return true;
    }

    if (m_phase == MissionPhase::EN_ROUTE_PATIENT) {
        if (m_currentWaypointIdx >= m_toPatientWaypoints.size() - 1) {
            // Arrived on scene
            m_phase = MissionPhase::ON_SCENE;
            m_node.TransitionState(common::AmbulanceState::PATIENT_PICKED);
            m_node.SetPatientId("PATIENT-" + m_incidentId);
            m_node.Gps().InjectPosition(m_patientLoc.latitude, m_patientLoc.longitude, 0.0f, 0.0f);
            m_node.PublishKinematicsOnce();
            m_node.PublishStateOnce();
            return true;
        }

        AdvanceWaypoint(deltaSeconds);
        m_node.PublishKinematicsOnce();
        return true;
    }

    if (m_phase == MissionPhase::ON_SCENE) {
        m_onSceneDwellRemainingSec -= deltaSeconds;
        // Paramedics actively monitoring vitals on-scene
        m_node.PublishVitalsOnce();

        if (m_onSceneDwellRemainingSec <= 0.0f) {
            m_phase = MissionPhase::EN_ROUTE_HOSPITAL;
            m_currentWaypointIdx = 0;
            m_node.TransitionState(common::AmbulanceState::EN_ROUTE_HOSPITAL);
            m_node.PublishStateOnce();
        }
        return true;
    }

    if (m_phase == MissionPhase::EN_ROUTE_HOSPITAL) {
        if (m_currentWaypointIdx >= m_toHospitalWaypoints.size() - 1) {
            // Arrived at hospital emergency department
            m_phase = MissionPhase::ARRIVED;
            m_node.TransitionState(common::AmbulanceState::ARRIVED);
            m_node.Gps().InjectPosition(m_hospitalLoc.latitude, m_hospitalLoc.longitude, 0.0f, 0.0f);
            m_node.PublishKinematicsOnce();
            m_node.PublishVitalsOnce();
            m_node.PublishStateOnce();
            return true;
        }

        AdvanceWaypoint(deltaSeconds);
        m_node.PublishKinematicsOnce();
        m_node.PublishVitalsOnce();
        return true;
    }

    if (m_phase == MissionPhase::ARRIVED) {
        m_arrivedDwellRemainingSec -= deltaSeconds;
        if (m_arrivedDwellRemainingSec <= 0.0f) {
            m_phase = MissionPhase::COMPLETED;
            m_node.TransitionState(common::AmbulanceState::AVAILABLE);
            m_node.PublishStateOnce();
            return false; // Mission finished
        }
        return true;
    }

    return false;
}

void AmbulanceMissionSimulator::AdvanceWaypoint(float deltaSeconds) {
    const auto& waypoints = (m_phase == MissionPhase::EN_ROUTE_PATIENT)
        ? m_toPatientWaypoints
        : m_toHospitalWaypoints;

    if (m_currentWaypointIdx >= waypoints.size() - 1) return;

    auto currentPos = m_node.Gps().GetLatestKinematics().position;
    const auto& targetPos = waypoints[m_currentWaypointIdx + 1];

    double distMeters = common::CalculateHaversineDistance(currentPos, targetPos);
    double travelMeters = (static_cast<double>(m_speedKmH) / 3.6) * static_cast<double>(deltaSeconds);

    if (distMeters <= travelMeters || distMeters < 5.0) {
        m_currentWaypointIdx++;
        double bearing = (m_currentWaypointIdx < waypoints.size() - 1)
            ? common::CalculateBearing(targetPos, waypoints[m_currentWaypointIdx + 1])
            : 0.0;
        m_node.Gps().InjectPosition(targetPos.latitude, targetPos.longitude, m_speedKmH, static_cast<float>(bearing));
    } else {
        double bearingDeg = common::CalculateBearing(currentPos, targetPos);
        double bearingRad = bearingDeg * (M_PI / 180.0);
        double earthRadiusM = 6371000.0;

        double deltaLat = (travelMeters * std::cos(bearingRad)) / earthRadiusM * (180.0 / M_PI);
        double latRad = currentPos.latitude * (M_PI / 180.0);
        double deltaLon = (travelMeters * std::sin(bearingRad)) / (earthRadiusM * std::cos(latRad)) * (180.0 / M_PI);

        m_node.Gps().InjectPosition(
            currentPos.latitude + deltaLat,
            currentPos.longitude + deltaLon,
            m_speedKmH,
            static_cast<float>(bearingDeg)
        );
    }
}

bool AmbulanceMissionSimulator::RunFullMission(float stepIntervalSec, size_t maxSteps) {
    size_t steps = 0;
    while (StepSimulation(stepIntervalSec) && steps < maxSteps) {
        steps++;
    }
    return m_phase == MissionPhase::COMPLETED;
}

} // namespace ambulance
} // namespace edge
} // namespace aegis
