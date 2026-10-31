/**
 * @file CityModel.cpp
 * @brief SMART AEGIS (AERS) Phase 19 - Municipal Digital Twin City Model Implementation.
 */

#include "CityModel.h"
#include <algorithm>
#include <cmath>

namespace aegis {
namespace simulator {

std::string WeatherConditionToString(WeatherCondition weather) {
    switch (weather) {
        case WeatherCondition::CLEAR: return "CLEAR";
        case WeatherCondition::HEAVY_RAIN: return "HEAVY_RAIN";
        case WeatherCondition::DENSE_FOG: return "DENSE_FOG";
        case WeatherCondition::THUNDERSTORM: return "THUNDERSTORM";
    }
    return "CLEAR";
}

CityModel::CityModel() {
    LoadFromConfigFile();
}

void CityModel::Reset() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_roadGraph.Clear();
    m_intersections.clear();
    m_hospitals.clear();
    m_ambulances.clear();
    m_civilianVehicles.clear();
    m_pedestrians.clear();
    m_events.clear();
    m_emergencies.clear();
    m_weather = WeatherCondition::CLEAR;
    m_baseTrafficDensity = 0.45;
    m_effectiveTrafficDensity = 0.45;
    LoadFromConfigFile();
}

bool CityModel::LoadFromConfigFile(const std::string& /*configFilePath*/) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    // 1. Build Road Graph Nodes
    const std::vector<RoadNode> defaultNodes = {
        {"AMB_STATION_1", "Central Municipal Depot", 12.9680, 77.5900, false},
        {"AMB_STATION_2", "East Sector Outpost", 12.9770, 77.6010, false},
        {"POLICE_STATION_1", "Central Police HQ", 12.9710, 77.5920, false},
        {"POLICE_STATION_2", "East Traffic Command", 12.9780, 77.5980, false},
        {"FIRE_STATION_1", "Central Fire Response", 12.9695, 77.5935, false},
        {"INT_A", "Brigade Road / MG Road", 12.9716, 77.5946, true},
        {"INT_B", "MG Road / Residency Road", 12.9725, 77.5958, true},
        {"INT_C", "Hospital Approach Avenue", 12.9738, 77.5972, true},
        {"INT_01", "MG Road - Brigade Junction", 12.9716, 77.5946, true},
        {"INT_02", "Residency Road Cross", 12.9725, 77.5958, true},
        {"INT_03", "Richmond Circle", 12.9738, 77.5972, true},
        {"INT_04", "Trinity Circle", 12.9745, 77.5990, true},
        {"INT_05", "Victoria Hospital Junction", 12.9675, 77.5940, true},
        {"INT_06", "Koramangala Ring Road", 12.9685, 77.5960, true},
        {"INT_07", "Indiranagar 100ft Junction", 12.9700, 77.5980, true},
        {"INT_08", "Old Airport Expressway", 12.9715, 77.6000, true},
        {"INT_09", "Cubbon Park North Cross", 12.9755, 77.5935, true},
        {"INT_10", "Queens Road Interlink", 12.9765, 77.5950, true},
        {"INT_11", "Cantonment Central", 12.9775, 77.5965, true},
        {"INT_12", "Halasuru Lake Overpass", 12.9785, 77.5985, true},
        {"HOSP_CITY_TRAUMA", "City Trauma Center", 12.9750, 77.5985, false},
        {"HOSP_METRO_GENERAL", "Metro General Hospital", 12.9690, 77.5910, false},
        {"HOSP_ST_JUDE", "St. Jude Heart Institute", 12.9765, 77.5995, false}
    };

    for (const auto& n : defaultNodes) {
        m_roadGraph.AddNode(n);
    }

    // 2. Build Road Edges (Bidirectional city avenues)
    const std::vector<RoadEdge> defaultEdges = {
        {"EDGE_DEPOT_A", "AMB_STATION_1", "INT_A", 650.0, 50.0, 2, 1.0, false, false, false},
        {"EDGE_A_DEPOT", "INT_A", "AMB_STATION_1", 650.0, 50.0, 2, 1.0, false, false, false},
        {"EDGE_A_B", "INT_A", "INT_B", 420.0, 50.0, 3, 1.0, false, false, false},
        {"EDGE_B_A", "INT_B", "INT_A", 420.0, 50.0, 3, 1.0, false, false, false},
        {"EDGE_B_C", "INT_B", "INT_C", 480.0, 50.0, 3, 1.0, false, false, false},
        {"EDGE_C_B", "INT_C", "INT_B", 480.0, 50.0, 3, 1.0, false, false, false},
        {"EDGE_C_TRAUMA", "INT_C", "HOSP_CITY_TRAUMA", 350.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_TRAUMA_C", "HOSP_CITY_TRAUMA", "INT_C", 350.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_TRAUMA_JUDE", "HOSP_CITY_TRAUMA", "HOSP_ST_JUDE", 280.0, 40.0, 2, 1.0, false, false, false},
        {"EDGE_JUDE_TRAUMA", "HOSP_ST_JUDE", "HOSP_CITY_TRAUMA", 280.0, 40.0, 2, 1.0, false, false, false},
        {"EDGE_DEPOT_METRO", "AMB_STATION_1", "HOSP_METRO_GENERAL", 250.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_METRO_DEPOT", "HOSP_METRO_GENERAL", "AMB_STATION_1", 250.0, 45.0, 2, 1.0, false, false, false},

        // Grid Arterials 1 - 12
        {"EDGE_01_02", "INT_01", "INT_02", 420.0, 50.0, 3, 1.0, false, false, false},
        {"EDGE_02_01", "INT_02", "INT_01", 420.0, 50.0, 3, 1.0, false, false, false},
        {"EDGE_02_03", "INT_02", "INT_03", 480.0, 50.0, 3, 1.0, false, false, false},
        {"EDGE_03_02", "INT_03", "INT_02", 480.0, 50.0, 3, 1.0, false, false, false},
        {"EDGE_03_04", "INT_03", "INT_04", 450.0, 50.0, 3, 1.0, false, false, false},
        {"EDGE_04_03", "INT_04", "INT_03", 450.0, 50.0, 3, 1.0, false, false, false},

        {"EDGE_01_05", "INT_01", "INT_05", 510.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_05_01", "INT_05", "INT_01", 510.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_02_06", "INT_02", "INT_06", 520.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_06_02", "INT_06", "INT_02", 520.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_03_07", "INT_03", "INT_07", 530.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_07_03", "INT_07", "INT_03", 530.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_04_08", "INT_04", "INT_08", 540.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_08_04", "INT_08", "INT_04", 540.0, 45.0, 2, 1.0, false, false, false},

        {"EDGE_05_06", "INT_05", "INT_06", 430.0, 50.0, 2, 1.0, false, false, false},
        {"EDGE_06_05", "INT_06", "INT_05", 430.0, 50.0, 2, 1.0, false, false, false},
        {"EDGE_06_07", "INT_06", "INT_07", 440.0, 50.0, 2, 1.0, false, false, false},
        {"EDGE_07_06", "INT_07", "INT_06", 440.0, 50.0, 2, 1.0, false, false, false},
        {"EDGE_07_08", "INT_07", "INT_08", 460.0, 50.0, 2, 1.0, false, false, false},
        {"EDGE_08_07", "INT_08", "INT_07", 460.0, 50.0, 2, 1.0, false, false, false},

        {"EDGE_09_10", "INT_09", "INT_10", 410.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_10_09", "INT_10", "INT_09", 410.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_10_11", "INT_10", "INT_11", 420.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_11_10", "INT_11", "INT_10", 420.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_11_12", "INT_11", "INT_12", 430.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_12_11", "INT_12", "INT_11", 430.0, 45.0, 2, 1.0, false, false, false},

        {"EDGE_01_09", "INT_01", "INT_09", 480.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_09_01", "INT_09", "INT_01", 480.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_02_10", "INT_02", "INT_10", 490.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_10_02", "INT_10", "INT_02", 490.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_03_11", "INT_03", "INT_11", 500.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_11_03", "INT_11", "INT_03", 500.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_04_12", "INT_04", "INT_12", 510.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_12_04", "INT_12", "INT_04", 510.0, 45.0, 2, 1.0, false, false, false},

        {"EDGE_03_TRAUMA", "INT_03", "HOSP_CITY_TRAUMA", 350.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_TRAUMA_03", "HOSP_CITY_TRAUMA", "INT_03", 350.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_05_METRO", "INT_05", "HOSP_METRO_GENERAL", 290.0, 40.0, 2, 1.0, false, false, false},
        {"EDGE_METRO_05", "HOSP_METRO_GENERAL", "INT_05", 290.0, 40.0, 2, 1.0, false, false, false},
        {"EDGE_12_JUDE", "INT_12", "HOSP_ST_JUDE", 310.0, 40.0, 2, 1.0, false, false, false},
        {"EDGE_JUDE_12", "HOSP_ST_JUDE", "INT_12", 310.0, 40.0, 2, 1.0, false, false, false},
        {"EDGE_08_OUTPOST", "INT_08", "AMB_STATION_2", 360.0, 45.0, 2, 1.0, false, false, false},
        {"EDGE_OUTPOST_08", "AMB_STATION_2", "INT_08", 360.0, 45.0, 2, 1.0, false, false, false}
    };

    for (const auto& e : defaultEdges) {
        m_roadGraph.AddEdge(e);
    }

    // 3. Virtual Intersections
    m_intersections["INT_A"] = std::make_shared<VirtualIntersection>("INT_A", "Brigade Road / MG Road", 12.9716, 77.5946);
    m_intersections["INT_B"] = std::make_shared<VirtualIntersection>("INT_B", "MG Road / Residency Road", 12.9725, 77.5958);
    m_intersections["INT_C"] = std::make_shared<VirtualIntersection>("INT_C", "Hospital Approach Avenue", 12.9738, 77.5972);

    m_intersections["INT_01"] = m_intersections["INT_A"];
    m_intersections["INT_02"] = m_intersections["INT_B"];
    m_intersections["INT_03"] = m_intersections["INT_C"];
    m_intersections["INT_04"] = std::make_shared<VirtualIntersection>("INT_04", "Trinity Circle", 12.9745, 77.5990);
    m_intersections["INT_05"] = std::make_shared<VirtualIntersection>("INT_05", "Victoria Hospital Junction", 12.9675, 77.5940);
    m_intersections["INT_06"] = std::make_shared<VirtualIntersection>("INT_06", "Koramangala Ring Road", 12.9685, 77.5960);
    m_intersections["INT_07"] = std::make_shared<VirtualIntersection>("INT_07", "Indiranagar 100ft Junction", 12.9700, 77.5980);
    m_intersections["INT_08"] = std::make_shared<VirtualIntersection>("INT_08", "Old Airport Expressway", 12.9715, 77.6000);
    m_intersections["INT_09"] = std::make_shared<VirtualIntersection>("INT_09", "Cubbon Park North Cross", 12.9755, 77.5935);
    m_intersections["INT_10"] = std::make_shared<VirtualIntersection>("INT_10", "Queens Road Interlink", 12.9765, 77.5950);
    m_intersections["INT_11"] = std::make_shared<VirtualIntersection>("INT_11", "Cantonment Central", 12.9775, 77.5965);
    m_intersections["INT_12"] = std::make_shared<VirtualIntersection>("INT_12", "Halasuru Lake Overpass", 12.9785, 77.5985);

    // 4. Virtual Hospitals
    m_hospitals["HOSP_CITY_TRAUMA"] = std::make_shared<VirtualHospital>("HOSP_CITY_TRAUMA", "City Trauma Center", 12.9750, 77.5985, 1, 12, 24, 8);
    m_hospitals["HOSP_METRO_GENERAL"] = std::make_shared<VirtualHospital>("HOSP_METRO_GENERAL", "Metro General Hospital", 12.9690, 77.5910, 2, 16, 32, 10);
    m_hospitals["HOSP_ST_JUDE"] = std::make_shared<VirtualHospital>("HOSP_ST_JUDE", "St. Jude Heart Institute", 12.9765, 77.5995, 3, 8, 16, 5);

    // 5. Virtual Ambulances
    m_ambulances["AMB_01"] = std::make_shared<VirtualAmbulance>("AMB_01", 12.9680, 77.5900);
    m_ambulances["AMB_02"] = std::make_shared<VirtualAmbulance>("AMB_02", 12.9680, 77.5900);
    m_ambulances["AMB_03"] = std::make_shared<VirtualAmbulance>("AMB_03", 12.9770, 77.6010);

    // 6. Spawn Initial Civilian Traffic & Pedestrians
    SpawnCivilianTraffic(15);
    SpawnPedestrians(8);

    return true;
}

void CityModel::SpawnCivilianTraffic(size_t vehicleCount) {
    m_civilianVehicles.clear();

    const std::vector<std::pair<double, double>> corridorRoute = {
        {12.9680, 77.5900},
        {12.9716, 77.5946},
        {12.9725, 77.5958},
        {12.9738, 77.5972},
        {12.9750, 77.5985}
    };

    for (size_t i = 0; i < vehicleCount; ++i) {
        std::string vid = "CIV_VEH_" + std::to_string(i + 1);
        std::shared_ptr<BaseVehicle> v;
        if (i % 4 == 0) {
            v = std::make_shared<Bus>(vid);
        } else if (i % 5 == 0) {
            v = std::make_shared<Truck>(vid);
        } else if (i % 3 == 0) {
            v = std::make_shared<Motorcycle>(vid);
        } else {
            v = std::make_shared<CivilianCar>(vid);
        }

        // Stagger positions along corridor
        double frac = static_cast<double>(i) / static_cast<double>(vehicleCount);
        size_t pIdx = static_cast<size_t>(frac * static_cast<double>(corridorRoute.size() - 1));
        v->SetRoute({corridorRoute.begin() + static_cast<ptrdiff_t>(pIdx), corridorRoute.end()});
        m_civilianVehicles.push_back(v);
    }
}

void CityModel::SpawnPedestrians(size_t pedestrianCount) {
    m_pedestrians.clear();
    for (size_t i = 0; i < pedestrianCount; ++i) {
        std::string pid = "PED_" + std::to_string(i + 1);
        auto p = std::make_shared<Pedestrian>(pid);
        double lat = 12.9716 + (static_cast<double>(i % 3) * 0.0010);
        double lon = 77.5946 + (static_cast<double>(i % 3) * 0.0010);
        p->SetRoute({{lat, lon}, {lat + 0.0003, lon + 0.0003}});
        m_pedestrians.push_back(p);
    }
}

void CityModel::UpdateTrafficCongestionPattern(double hourOfDay) {
    // Diurnal traffic curve
    // Peak 1: 08:00 - 10:30 (Morning commute)
    // Peak 2: 17:00 - 20:00 (Evening commute)
    // Night: 22:00 - 05:00 (Low density)
    if ((hourOfDay >= 8.0 && hourOfDay <= 10.5) || (hourOfDay >= 17.0 && hourOfDay <= 20.0)) {
        m_effectiveTrafficDensity = std::min(0.95, m_baseTrafficDensity * 1.8);
    } else if (hourOfDay >= 22.0 || hourOfDay <= 5.0) {
        m_effectiveTrafficDensity = m_baseTrafficDensity * 0.35;
    } else {
        m_effectiveTrafficDensity = m_baseTrafficDensity;
    }

    // Apply to road edges
    for (auto& edge : m_roadGraph.GetAllEdges()) {
        m_roadGraph.SetCongestion(edge.id, 1.0 + (m_effectiveTrafficDensity * 2.0));
    }
}

void CityModel::Update(double dtSec, double currentHourOfDay) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    UpdateTrafficCongestionPattern(currentHourOfDay);

    // 1. Update Intersections
    for (auto& [id, inter] : m_intersections) {
        (void)id;
        inter->Update(dtSec);
    }

    // 2. Update Hospitals
    for (auto& [id, hosp] : m_hospitals) {
        (void)id;
        hosp->Update(dtSec);
    }

    // 3. Update Ambulances
    for (auto& [id, amb] : m_ambulances) {
        (void)id;
        amb->Update(dtSec);

        // Check proximity to intersections to trigger IR sensors
        for (auto& [nodeId, inter] : m_intersections) {
            (void)nodeId;
            double dLat = inter->GetLatitude() - amb->GetLatitude();
            double dLon = inter->GetLongitude() - amb->GetLongitude();
            double dist = std::hypot(dLat, dLon) * 111000.0;

            if (dist < 80.0 && dist > 15.0) {
                inter->TriggerApproachIR();
            } else if (dist <= 15.0) {
                inter->TriggerDepartureIR();
            }
        }
    }

    // 4. Update Civilian Vehicles
    for (auto& veh : m_civilianVehicles) {
        veh->Update(dtSec);
    }

    // 5. Update Pedestrians
    for (auto& ped : m_pedestrians) {
        ped->Update(dtSec);
    }

    // 6. Update Live Events
    for (auto& ev : m_events) {
        if (ev.isActive) {
            ev.elapsedSec += dtSec;
            if (ev.elapsedSec >= ev.durationSec) {
                ev.isActive = false;
                // Clear event effect
                if (ev.type == "ROAD_CLOSURE") {
                    m_roadGraph.SetRoadClosed(ev.targetEntityId, false);
                } else if (ev.type == "ACCIDENT") {
                    m_roadGraph.SetAccident(ev.targetEntityId, false);
                } else if (ev.type == "HEAVY_RAIN") {
                    m_weather = WeatherCondition::CLEAR;
                }
            }
        }
    }
}

void CityModel::TriggerLiveEvent(const LiveDisruptionEvent& event) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_events.push_back(event);

    if (event.type == "ROAD_CLOSURE") {
        m_roadGraph.SetRoadClosed(event.targetEntityId, true);
    } else if (event.type == "ACCIDENT") {
        m_roadGraph.SetAccident(event.targetEntityId, true);
    } else if (event.type == "HEAVY_RAIN") {
        m_weather = WeatherCondition::HEAVY_RAIN;
    } else if (event.type == "HOSPITAL_OVERLOAD") {
        auto it = m_hospitals.find(event.targetEntityId);
        if (it != m_hospitals.end()) {
            it->second->SetDiversion(true);
        }
    }
}

std::vector<LiveDisruptionEvent> CityModel::GetActiveEvents() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<LiveDisruptionEvent> active;
    for (const auto& ev : m_events) {
        if (ev.isActive) {
            active.push_back(ev);
        }
    }
    return active;
}

void CityModel::ClearEvents() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    for (auto& ev : m_events) {
        if (ev.isActive) {
            if (ev.type == "ROAD_CLOSURE") {
                m_roadGraph.SetRoadClosed(ev.targetEntityId, false);
            } else if (ev.type == "ACCIDENT") {
                m_roadGraph.SetAccident(ev.targetEntityId, false);
            }
        }
    }
    m_events.clear();
    m_weather = WeatherCondition::CLEAR;
}

WeatherCondition CityModel::GetWeather() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_weather;
}

void CityModel::SetWeather(WeatherCondition weather) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_weather = weather;
}

double CityModel::GetTrafficDensity() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_effectiveTrafficDensity;
}

void CityModel::SetTrafficDensity(double density) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_baseTrafficDensity = density;
    m_effectiveTrafficDensity = density;
}

void CityModel::AddEmergency(const GeneratedEmergency& emergency) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_emergencies.push_back(emergency);
}

std::vector<GeneratedEmergency> CityModel::GetActiveEmergencies() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_emergencies;
}

std::shared_ptr<VirtualIntersection> CityModel::GetIntersection(const std::string& id) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_intersections.find(id);
    return (it != m_intersections.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<VirtualIntersection>> CityModel::GetAllIntersections() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<std::shared_ptr<VirtualIntersection>> list;
    for (const auto& [id, inter] : m_intersections) {
        (void)id;
        list.push_back(inter);
    }
    return list;
}

std::shared_ptr<VirtualHospital> CityModel::GetHospital(const std::string& id) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_hospitals.find(id);
    return (it != m_hospitals.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<VirtualHospital>> CityModel::GetAllHospitals() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<std::shared_ptr<VirtualHospital>> list;
    for (const auto& [id, hosp] : m_hospitals) {
        (void)id;
        list.push_back(hosp);
    }
    return list;
}

std::shared_ptr<VirtualAmbulance> CityModel::GetAmbulance(const std::string& id) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_ambulances.find(id);
    return (it != m_ambulances.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<VirtualAmbulance>> CityModel::GetAllAmbulances() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<std::shared_ptr<VirtualAmbulance>> list;
    for (const auto& [id, amb] : m_ambulances) {
        (void)id;
        list.push_back(amb);
    }
    return list;
}

std::vector<std::shared_ptr<BaseVehicle>> CityModel::GetAllVehicles() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_civilianVehicles;
}

} // namespace simulator
} // namespace aegis
