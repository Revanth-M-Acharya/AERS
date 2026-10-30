/**
 * @file VirtualVehicle.h
 * @brief SMART AEGIS (AERS) Phase 19 - Microscopic Vehicle Dynamics & Traffic Simulation.
 */

#ifndef AEGIS_SIMULATOR_VIRTUAL_VEHICLE_H
#define AEGIS_SIMULATOR_VIRTUAL_VEHICLE_H

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace aegis {
namespace simulator {

/**
 * @brief Classification of simulated traffic actors.
 */
enum class VehicleType : uint8_t {
    CIVILIAN_CAR = 0,
    BUS,
    TRUCK,
    MOTORCYCLE,
    EMERGENCY_AMBULANCE,
    PEDESTRIAN
};

std::string VehicleTypeToString(VehicleType type);

/**
 * @brief Base class for all simulated mobile entities in the road network.
 */
class BaseVehicle {
public:
    BaseVehicle(std::string id, VehicleType type, double maxSpeedKmh = 50.0);
    virtual ~BaseVehicle() = default;

    // Kinematics Update
    virtual void Update(double dtSec);

    // Navigation
    void SetRoute(const std::vector<std::pair<double, double>>& waypoints);
    void SetCurrentPosition(double lat, double lon);
    void SetSpeedKmh(double speed);
    void SetHeadingDeg(double heading);

    // Getters
    std::string GetId() const { return m_id; }
    VehicleType GetType() const { return m_type; }
    double GetLatitude() const { return m_lat; }
    double GetLongitude() const { return m_lon; }
    double GetSpeedKmh() const { return m_speedKmh; }
    double GetHeadingDeg() const { return m_headingDeg; }
    double GetAcceleration() const { return m_accelMps2; }
    bool HasArrived() const { return m_hasArrived; }
    std::string GetCurrentRoadId() const { return m_currentRoadId; }
    void SetCurrentRoadId(const std::string& roadId) { m_currentRoadId = roadId; }
    uint8_t GetCurrentLane() const { return m_currentLane; }
    void SetCurrentLane(uint8_t lane) { m_currentLane = lane; }

protected:
    std::string m_id;
    VehicleType m_type;
    double m_lat{12.9716};
    double m_lon{77.5946};
    double m_speedKmh{0.0};
    double m_maxSpeedKmh{50.0};
    double m_headingDeg{0.0};
    double m_accelMps2{0.0};
    std::string m_currentRoadId;
    uint8_t m_currentLane{0};

    std::vector<std::pair<double, double>> m_waypoints;
    size_t m_currentWaypointIdx{0};
    bool m_hasArrived{false};
};

/**
 * @brief Standard civilian passenger car.
 */
class CivilianCar : public BaseVehicle {
public:
    explicit CivilianCar(std::string id);
};

/**
 * @brief High-capacity municipal bus.
 */
class Bus : public BaseVehicle {
public:
    explicit Bus(std::string id);
};

/**
 * @brief Commercial heavy freight vehicle.
 */
class Truck : public BaseVehicle {
public:
    explicit Truck(std::string id);
};

/**
 * @brief Fast, agile two-wheeled motorcycle.
 */
class Motorcycle : public BaseVehicle {
public:
    explicit Motorcycle(std::string id);
};

/**
 * @brief Emergency vehicle with preemption authority.
 */
class EmergencyVehicle : public BaseVehicle {
public:
    explicit EmergencyVehicle(std::string id);
};

/**
 * @brief Simulated pedestrian crossing roads at crosswalks.
 */
class Pedestrian : public BaseVehicle {
public:
    explicit Pedestrian(std::string id);
    void Update(double dtSec) override;
};

} // namespace simulator
} // namespace aegis

#endif // AEGIS_SIMULATOR_VIRTUAL_VEHICLE_H

