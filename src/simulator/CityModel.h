/**
 * @file CityModel.h
 * @brief SMART AEGIS (AERS) Phase 19 - Municipal Digital Twin City Model & State Container.
 */

#ifndef AEGIS_SIMULATOR_CITY_MODEL_H
#define AEGIS_SIMULATOR_CITY_MODEL_H

#include "RoadGraph.h"
#include "VirtualIntersection.h"
#include "VirtualHospital.h"
#include "VirtualAmbulance.h"
#include "VirtualVehicle.h"
#include "EmergencyGenerator.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace aegis {
namespace simulator {

/**
 * @brief Dynamic atmospheric weather condition.
 */
enum class WeatherCondition : uint8_t {
    CLEAR = 0,
    HEAVY_RAIN,
    DENSE_FOG,
    THUNDERSTORM
};

std::string WeatherConditionToString(WeatherCondition weather);

/**
 * @brief Municipal dynamic disturbance event.
 */
struct LiveDisruptionEvent {
    std::string id;
    std::string type; // "ROAD_CLOSURE", "ACCIDENT", "SIGNAL_FAILURE", "HOSPITAL_OVERLOAD", "HEAVY_RAIN"
    std::string targetEntityId;
    std::string description;
    double durationSec{60.0};
    double elapsedSec{0.0};
    bool isActive{true};
};

/**
 * @brief Master digital twin container holding all static and dynamic elements of the city.
 */
class CityModel {
public:
    CityModel();
    ~CityModel() = default;

    // Initialization
    bool LoadFromConfigFile(const std::string& configFilePath = "config/simulation.json");
    void Reset();

    // World Update Loop
    void Update(double dtSec, double currentHourOfDay);

    // Dynamic Traffic & Vehicles
    void SpawnCivilianTraffic(size_t vehicleCount = 20);
    void SpawnPedestrians(size_t pedestrianCount = 10);
    std::vector<std::shared_ptr<BaseVehicle>> GetAllVehicles() const;

    // Entities Management
    RoadGraph& GetRoadGraph() { return m_roadGraph; }
    const RoadGraph& GetRoadGraph() const { return m_roadGraph; }

    std::shared_ptr<VirtualIntersection> GetIntersection(const std::string& id) const;
    std::vector<std::shared_ptr<VirtualIntersection>> GetAllIntersections() const;

    std::shared_ptr<VirtualHospital> GetHospital(const std::string& id) const;
    std::vector<std::shared_ptr<VirtualHospital>> GetAllHospitals() const;

    std::shared_ptr<VirtualAmbulance> GetAmbulance(const std::string& id) const;
    std::vector<std::shared_ptr<VirtualAmbulance>> GetAllAmbulances() const;

    // Live Events
    void TriggerLiveEvent(const LiveDisruptionEvent& event);
    std::vector<LiveDisruptionEvent> GetActiveEvents() const;
    void ClearEvents();

    // Weather & Traffic
    WeatherCondition GetWeather() const;
    void SetWeather(WeatherCondition weather);

    double GetTrafficDensity() const;
    void SetTrafficDensity(double density);

    // Emergency Incidents
    void AddEmergency(const GeneratedEmergency& emergency);
    std::vector<GeneratedEmergency> GetActiveEmergencies() const;

private:
    void UpdateTrafficCongestionPattern(double hourOfDay);

    mutable std::recursive_mutex m_mutex;
    RoadGraph m_roadGraph;

    std::unordered_map<std::string, std::shared_ptr<VirtualIntersection>> m_intersections;
    std::unordered_map<std::string, std::shared_ptr<VirtualHospital>> m_hospitals;
    std::unordered_map<std::string, std::shared_ptr<VirtualAmbulance>> m_ambulances;
    std::vector<std::shared_ptr<BaseVehicle>> m_civilianVehicles;
    std::vector<std::shared_ptr<Pedestrian>> m_pedestrians;

    std::vector<LiveDisruptionEvent> m_events;
    std::vector<GeneratedEmergency> m_emergencies;

    WeatherCondition m_weather{WeatherCondition::CLEAR};
    double m_baseTrafficDensity{0.45};
    double m_effectiveTrafficDensity{0.45};
};

} // namespace simulator
} // namespace aegis

#endif // AEGIS_SIMULATOR_CITY_MODEL_H
