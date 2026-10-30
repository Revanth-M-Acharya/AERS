/**
 * @file VirtualVehicle.cpp
 * @brief SMART AEGIS (AERS) Phase 19 - Microscopic Vehicle Dynamics & Traffic Simulation Implementation.
 */

#include "VirtualVehicle.h"
#include <cmath>

namespace aegis {
namespace simulator {

std::string VehicleTypeToString(VehicleType type) {
    switch (type) {
        case VehicleType::CIVILIAN_CAR: return "CIVILIAN_CAR";
        case VehicleType::BUS: return "BUS";
        case VehicleType::TRUCK: return "TRUCK";
        case VehicleType::MOTORCYCLE: return "MOTORCYCLE";
        case VehicleType::EMERGENCY_AMBULANCE: return "EMERGENCY_AMBULANCE";
        case VehicleType::PEDESTRIAN: return "PEDESTRIAN";
    }
    return "UNKNOWN";
}

BaseVehicle::BaseVehicle(std::string id, VehicleType type, double maxSpeedKmh)
    : m_id(std::move(id))
    , m_type(type)
    , m_speedKmh(maxSpeedKmh * 0.7)
    , m_maxSpeedKmh(maxSpeedKmh)
{
}

void BaseVehicle::SetRoute(const std::vector<std::pair<double, double>>& waypoints) {
    m_waypoints = waypoints;
    m_currentWaypointIdx = 0;
    m_hasArrived = waypoints.empty();
    if (!waypoints.empty()) {
        m_lat = waypoints[0].first;
        m_lon = waypoints[0].second;
    }
}

void BaseVehicle::SetCurrentPosition(double lat, double lon) {
    m_lat = lat;
    m_lon = lon;
}

void BaseVehicle::SetSpeedKmh(double speed) {
    m_speedKmh = (speed < 0.0) ? 0.0 : ((speed > m_maxSpeedKmh) ? m_maxSpeedKmh : speed);
}

void BaseVehicle::SetHeadingDeg(double heading) {
    m_headingDeg = heading;
}

void BaseVehicle::Update(double dtSec) {
    if (m_hasArrived || m_waypoints.empty()) {
        m_speedKmh = 0.0;
        return;
    }

    if (m_currentWaypointIdx >= m_waypoints.size()) {
        m_hasArrived = true;
        m_speedKmh = 0.0;
        return;
    }

    auto [targetLat, targetLon] = m_waypoints[m_currentWaypointIdx];
    double dLat = targetLat - m_lat;
    double dLon = targetLon - m_lon;
    double dist = std::hypot(dLat, dLon);

    // If within ~10 meters (approx 0.0001 deg)
    if (dist < 0.0001) {
        m_currentWaypointIdx++;
        if (m_currentWaypointIdx >= m_waypoints.size()) {
            m_hasArrived = true;
            m_speedKmh = 0.0;
            return;
        }
        targetLat = m_waypoints[m_currentWaypointIdx].first;
        targetLon = m_waypoints[m_currentWaypointIdx].second;
        dLat = targetLat - m_lat;
        dLon = targetLon - m_lon;
        dist = std::hypot(dLat, dLon);
    }

    // Update heading
    constexpr double radToDeg = 180.0 / 3.14159265358979323846;
    m_headingDeg = std::atan2(dLon, dLat) * radToDeg;
    if (m_headingDeg < 0.0) m_headingDeg += 360.0;

    // Advance position: 1 deg lat is ~111,000 meters
    double speedMps = m_speedKmh * 0.277778;
    double distanceTraveledMeters = speedMps * dtSec;
    double degStep = distanceTraveledMeters / 111000.0;

    if (dist > 0.0) {
        double step = std::min(degStep, dist);
        m_lat += (dLat / dist) * step;
        m_lon += (dLon / dist) * step;
    }
}

// Derived vehicle implementations
CivilianCar::CivilianCar(std::string id)
    : BaseVehicle(std::move(id), VehicleType::CIVILIAN_CAR, 50.0)
{
}

Bus::Bus(std::string id)
    : BaseVehicle(std::move(id), VehicleType::BUS, 40.0)
{
}

Truck::Truck(std::string id)
    : BaseVehicle(std::move(id), VehicleType::TRUCK, 35.0)
{
}

Motorcycle::Motorcycle(std::string id)
    : BaseVehicle(std::move(id), VehicleType::MOTORCYCLE, 55.0)
{
}

EmergencyVehicle::EmergencyVehicle(std::string id)
    : BaseVehicle(std::move(id), VehicleType::EMERGENCY_AMBULANCE, 65.0)
{
}

Pedestrian::Pedestrian(std::string id)
    : BaseVehicle(std::move(id), VehicleType::PEDESTRIAN, 5.0)
{
}

void Pedestrian::Update(double dtSec) {
    BaseVehicle::Update(dtSec);
}

} // namespace simulator
} // namespace aegis
