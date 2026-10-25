/**
 * @file ambulance_mission_simulator.hpp
 * @brief Autonomous simulator executing the full ambulance incident mission lifecycle.
 */

#ifndef AEGIS_EDGE_AMBULANCE_MISSION_SIMULATOR_HPP
#define AEGIS_EDGE_AMBULANCE_MISSION_SIMULATOR_HPP

#include <string>
#include <vector>
#include <memory>
#include "aegis/common/types.hpp"
#include "aegis/edge/ambulance_node.hpp"

namespace aegis {
namespace edge {
namespace ambulance {

enum class MissionPhase : uint8_t {
    IDLE = 0,
    DISPATCHED,
    EN_ROUTE_PATIENT,
    ON_SCENE,
    EN_ROUTE_HOSPITAL,
    ARRIVED,
    COMPLETED
};

/**
 * @brief Mission simulator advancing ambulance kinematics, physiological vitals, and state machine.
 */
class AmbulanceMissionSimulator {
public:
    explicit AmbulanceMissionSimulator(AmbulanceNode& node);
    ~AmbulanceMissionSimulator() = default;

    // Configuration
    void ConfigureMission(const std::string& incidentId,
                          const common::GeoCoordinate& startLoc,
                          const common::GeoCoordinate& patientLoc,
                          const common::GeoCoordinate& hospitalLoc,
                          const std::vector<common::GeoCoordinate>& toPatientWaypoints = {},
                          const std::vector<common::GeoCoordinate>& toHospitalWaypoints = {});

    // Execution
    bool StepSimulation(float deltaSeconds);
    bool RunFullMission(float stepIntervalSec = 0.5f, size_t maxSteps = 500);

    // Queries
    MissionPhase GetCurrentPhase() const noexcept { return m_phase; }
    std::string GetCurrentPhaseString() const;
    float GetMissionProgress() const noexcept;
    bool IsCompleted() const noexcept { return m_phase == MissionPhase::COMPLETED; }

private:
    void AdvanceWaypoint(float deltaSeconds);

    AmbulanceNode& m_node;
    std::string m_incidentId;
    common::GeoCoordinate m_startLoc{12.971598, 77.594562, 0.0};
    common::GeoCoordinate m_patientLoc{12.976000, 77.601000, 0.0};
    common::GeoCoordinate m_hospitalLoc{12.982000, 77.610000, 0.0};

    std::vector<common::GeoCoordinate> m_toPatientWaypoints;
    std::vector<common::GeoCoordinate> m_toHospitalWaypoints;

    MissionPhase m_phase{MissionPhase::IDLE};
    size_t m_currentWaypointIdx{0};
    float m_onSceneDwellRemainingSec{3.0f};
    float m_arrivedDwellRemainingSec{2.0f};
    float m_speedKmH{65.0f};
};

} // namespace ambulance
} // namespace edge
} // namespace aegis

#endif // AEGIS_EDGE_AMBULANCE_MISSION_SIMULATOR_HPP
