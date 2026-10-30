/**
 * @file SimulationEngine.cpp
 * @brief SMART AEGIS (AERS) Phase 19 - Master Municipal City Simulation Engine Implementation.
 */

#include "SimulationEngine.h"
#include <chrono>
#include <cmath>
#include <utility>

namespace aegis {
namespace simulator {

std::string DemonstrationModeToString(DemonstrationMode mode) {
    switch (mode) {
        case DemonstrationMode::MODE_1_SOFTWARE_SIMULATION:
            return "MODE_1_SOFTWARE_SIMULATION";
        case DemonstrationMode::MODE_2_SINGLE_AMBULANCE_HARDWARE:
            return "MODE_2_SINGLE_AMBULANCE_HARDWARE";
        case DemonstrationMode::MODE_3_DUAL_AMBULANCE_CONFLICT:
            return "MODE_3_DUAL_AMBULANCE_CONFLICT";
    }
    return "MODE_1_SOFTWARE_SIMULATION";
}

SimulationEngine::SimulationEngine(const std::string& configFilePath)
    : m_configFilePath(configFilePath)
{
    Initialize(configFilePath);
}

SimulationEngine::~SimulationEngine() {
    Stop();
}

bool SimulationEngine::Initialize(const std::string& configFilePath) {
    std::lock_guard<std::recursive_mutex> lock(m_engineMutex);
    m_configFilePath = configFilePath;
    m_clock.Reset(8.0);
    bool ok = m_cityModel.LoadFromConfigFile(m_configFilePath);
    ConfigureMode1();
    return ok;
}

void SimulationEngine::Start() {
    if (m_running.load()) {
        return;
    }
    m_running.store(true);

    m_clockThread = std::thread(&SimulationEngine::MasterClockWorker, this);
    m_trafficThread = std::thread(&SimulationEngine::TrafficWorker, this);
    m_vehicleThread = std::thread(&SimulationEngine::VehicleWorker, this);
    m_emergencyThread = std::thread(&SimulationEngine::EmergencyWorker, this);
    m_hospitalThread = std::thread(&SimulationEngine::HospitalWorker, this);
    m_dashboardThread = std::thread(&SimulationEngine::DashboardWorker, this);
}

void SimulationEngine::Stop() {
    if (!m_running.load()) {
        return;
    }
    m_running.store(false);

    {
        std::lock_guard<std::mutex> lock(m_stopMutex);
        m_stopCv.notify_all();
    }

    if (m_clockThread.joinable()) {
        m_clockThread.join();
    }
    if (m_trafficThread.joinable()) {
        m_trafficThread.join();
    }
    if (m_vehicleThread.joinable()) {
        m_vehicleThread.join();
    }
    if (m_emergencyThread.joinable()) {
        m_emergencyThread.join();
    }
    if (m_hospitalThread.joinable()) {
        m_hospitalThread.join();
    }
    if (m_dashboardThread.joinable()) {
        m_dashboardThread.join();
    }
}

bool SimulationEngine::IsRunning() const {
    return m_running.load();
}

void SimulationEngine::Pause() {
    m_clock.Pause();
}

void SimulationEngine::Resume() {
    m_clock.Resume();
}

void SimulationEngine::TogglePause() {
    m_clock.TogglePause();
}

bool SimulationEngine::IsPaused() const {
    return m_clock.IsPaused();
}

void SimulationEngine::Reset() {
    std::lock_guard<std::recursive_mutex> lock(m_engineMutex);
    m_clock.Reset(8.0);
    m_cityModel.Reset();
    if (m_demoMode == DemonstrationMode::MODE_1_SOFTWARE_SIMULATION) {
        ConfigureMode1();
    } else if (m_demoMode == DemonstrationMode::MODE_2_SINGLE_AMBULANCE_HARDWARE) {
        ConfigureMode2();
    } else if (m_demoMode == DemonstrationMode::MODE_3_DUAL_AMBULANCE_CONFLICT) {
        ConfigureMode3();
    }
}

void SimulationEngine::Step(double dtSec) {
    std::lock_guard<std::recursive_mutex> lock(m_engineMutex);
    m_clock.StepSingleTick(dtSec);
    m_cityModel.Update(dtSec, m_clock.GetCurrentHourOfDay());

    if (m_demoMode == DemonstrationMode::MODE_3_DUAL_AMBULANCE_CONFLICT) {
        UpdateConflictArbitration();
    }

    m_dashboardAdapter.PublishSnapshot(m_cityModel, m_clock.GetElapsedSimSeconds());
}

void SimulationEngine::SetSpeedMultiplier(SimulationSpeedMultiplier multiplier) {
    m_clock.SetSpeedMultiplier(multiplier);
}

SimulationSpeedMultiplier SimulationEngine::GetSpeedMultiplier() const {
    return m_clock.GetSpeedMultiplier();
}

void SimulationEngine::SetTimeScale(double scale) {
    m_clock.SetTimeScale(scale);
}

double SimulationEngine::GetTimeScale() const {
    return m_clock.GetTimeScale();
}

void SimulationEngine::SetDemonstrationMode(DemonstrationMode mode) {
    std::lock_guard<std::recursive_mutex> lock(m_engineMutex);
    m_demoMode = mode;
    switch (mode) {
        case DemonstrationMode::MODE_1_SOFTWARE_SIMULATION:
            ConfigureMode1();
            break;
        case DemonstrationMode::MODE_2_SINGLE_AMBULANCE_HARDWARE:
            ConfigureMode2();
            break;
        case DemonstrationMode::MODE_3_DUAL_AMBULANCE_CONFLICT:
            ConfigureMode3();
            break;
    }
}

DemonstrationMode SimulationEngine::GetDemonstrationMode() const {
    std::lock_guard<std::recursive_mutex> lock(m_engineMutex);
    return m_demoMode;
}

ConflictArbitrationStatus SimulationEngine::GetConflictStatus() const {
    std::lock_guard<std::recursive_mutex> lock(m_engineMutex);
    return m_conflictStatus;
}

void SimulationEngine::ConfigureMode1() {
    m_cityModel.Reset();
    m_cityModel.SpawnCivilianTraffic(20);
    m_cityModel.SpawnPedestrians(10);
    m_conflictStatus = ConflictArbitrationStatus{};
}

void SimulationEngine::ConfigureMode2() {
    m_cityModel.Reset();
    m_cityModel.SpawnCivilianTraffic(10);
    m_conflictStatus = ConflictArbitrationStatus{};

    // Single Ambulance Corridor: AMB_01 dispatched from AMB_STATION_1 to HOSP_CITY_TRAUMA
    auto amb = m_cityModel.GetAmbulance("AMB_01");
    if (amb) {
        SimulatedPatientVitals vitals;
        vitals.patientId = "PAT_MODE2_01";
        vitals.name = "Cardiac Patient Alpha";
        vitals.age = 58;
        vitals.heartRateBpm = 142;
        vitals.spo2Pct = 86;
        vitals.systolicBP = 88;
        vitals.diastolicBP = 54;
        vitals.gcsScore = 13;

        std::vector<std::pair<double, double>> corridorRoute = {
            {12.9680, 77.5900}, // AMB_STATION_1
            {12.9716, 77.5946}, // INT_A
            {12.9725, 77.5958}, // INT_B
            {12.9738, 77.5972}, // INT_C
            {12.9750, 77.5985}  // HOSP_CITY_TRAUMA
        };

        amb->Dispatch("MISSION_HW_MODE2",
                      "INC_MODE2_01",
                      12.9700, 77.5925,
                      "HOSP_CITY_TRAUMA",
                      12.9750, 77.5985,
                      vitals,
                      corridorRoute);
    }

    // Preempt all corridor intersections
    auto intA = m_cityModel.GetIntersection("INT_A");
    auto intB = m_cityModel.GetIntersection("INT_B");
    auto intC = m_cityModel.GetIntersection("INT_C");
    if (intA) intA->PreemptForCorridor("CORRIDOR_GREEN_WAVE_01", 90.0);
    if (intB) intB->PreemptForCorridor("CORRIDOR_GREEN_WAVE_01", 90.0);
    if (intC) intC->PreemptForCorridor("CORRIDOR_GREEN_WAVE_01", 90.0);
}

void SimulationEngine::ConfigureMode3() {
    m_cityModel.Reset();
    m_cityModel.SpawnCivilianTraffic(12);

    // Ambulance 1: Critical Cardiac (Score 95.0)
    auto amb1 = m_cityModel.GetAmbulance("AMB_01");
    if (amb1) {
        SimulatedPatientVitals v1;
        v1.patientId = "PAT_CRITICAL_01";
        v1.name = "Acute STEMI Patient";
        v1.age = 62;
        v1.heartRateBpm = 150;
        v1.spo2Pct = 82;
        v1.systolicBP = 80;
        v1.diastolicBP = 50;
        v1.gcsScore = 11;

        std::vector<std::pair<double, double>> route1 = {
            {12.9680, 77.5900}, // AMB_STATION_1
            {12.9716, 77.5946}, // INT_A
            {12.9725, 77.5958}, // INT_B (Conflict point)
            {12.9750, 77.5985}  // HOSP_CITY_TRAUMA
        };

        amb1->Dispatch("MISSION_CONFLICT_01",
                       "INC_CARDIAC_CRIT",
                       12.9700, 77.5920,
                       "HOSP_CITY_TRAUMA",
                       12.9750, 77.5985,
                       v1,
                       route1);
    }

    // Ambulance 2: High Trauma (Score 80.0)
    auto amb2 = m_cityModel.GetAmbulance("AMB_02");
    if (amb2) {
        SimulatedPatientVitals v2;
        v2.patientId = "PAT_TRAUMA_02";
        v2.name = "Polytrauma Patient";
        v2.age = 34;
        v2.heartRateBpm = 115;
        v2.spo2Pct = 93;
        v2.systolicBP = 110;
        v2.diastolicBP = 70;
        v2.gcsScore = 14;

        std::vector<std::pair<double, double>> route2 = {
            {12.9770, 77.6010}, // AMB_STATION_2
            {12.9738, 77.5972}, // INT_C
            {12.9725, 77.5958}, // INT_B (Conflict point)
            {12.9690, 77.5910}  // HOSP_METRO_GENERAL
        };

        amb2->Dispatch("MISSION_CONFLICT_02",
                       "INC_TRAUMA_HIGH",
                       12.9750, 77.5990,
                       "HOSP_METRO_GENERAL",
                       12.9690, 77.5910,
                       v2,
                       route2);
    }

    // Initialize conflict arbitration state
    m_conflictStatus.conflictDetected = true;
    m_conflictStatus.intersectionId = "INT_B";
    m_conflictStatus.grantedAmbulanceId = "AMB_01";
    m_conflictStatus.heldAmbulanceId = "AMB_02";
    m_conflictStatus.grantedPriorityScore = 95.0;
    m_conflictStatus.heldPriorityScore = 80.0;
    m_conflictStatus.isCleared = false;

    // Grant initial preemption to higher priority AMB_01
    auto intB = m_cityModel.GetIntersection("INT_B");
    if (intB) {
        intB->PreemptForCorridor("CORRIDOR_AMB_01", 60.0);
        intB->SetApproachingAmbulance("AMB_01", 12.0);
    }
}

void SimulationEngine::UpdateConflictArbitration() {
    auto amb1 = m_cityModel.GetAmbulance("AMB_01");
    auto amb2 = m_cityModel.GetAmbulance("AMB_02");
    auto intB = m_cityModel.GetIntersection("INT_B");

    if (!amb1 || !amb2 || !intB) {
        return;
    }

    // Check if AMB_01 has cleared INT_B (passed through and moved towards hospital)
    double distAmb1ToIntB = std::hypot(amb1->GetLatitude() - intB->GetLatitude(),
                                      amb1->GetLongitude() - intB->GetLongitude()) * 111000.0;

    if (!m_conflictStatus.isCleared) {
        // If AMB_01 is past INT_B or arrived
        if (amb1->GetLatitude() > intB->GetLatitude() || distAmb1ToIntB > 120.0 || amb1->HasArrived()) {
            // AMB_01 cleared INT_B -> Transfer corridor to held AMB_02
            m_conflictStatus.isCleared = true;
            intB->ReleasePreemption();
            intB->PreemptForCorridor("CORRIDOR_AMB_02", 60.0);
            intB->SetApproachingAmbulance("AMB_02", 8.0);
        }
    }
}

void SimulationEngine::SetControlCentreDataModel(std::shared_ptr<dashboard::DashboardDataModel> model) {
    m_dashboardAdapter.SetControlCentreDataModel(std::move(model));
}

void SimulationEngine::SetTrafficDashboardDataModel(std::shared_ptr<dashboard::traffic::TrafficDashboardDataModel> model) {
    m_dashboardAdapter.SetTrafficDashboardDataModel(std::move(model));
}

void SimulationEngine::TriggerDisruption(const LiveDisruptionEvent& event) {
    m_cityModel.TriggerLiveEvent(event);
}

void SimulationEngine::ClearDisruptions() {
    m_cityModel.ClearEvents();
}

void SimulationEngine::SetWeather(WeatherCondition weather) {
    m_cityModel.SetWeather(weather);
}

WeatherCondition SimulationEngine::GetWeather() const {
    return m_cityModel.GetWeather();
}

void SimulationEngine::SetTrafficDensity(double density) {
    m_cityModel.SetTrafficDensity(density);
}

double SimulationEngine::GetTrafficDensity() const {
    return m_cityModel.GetTrafficDensity();
}

GeneratedEmergency SimulationEngine::DispatchEmergency(EmergencyType type,
                                                       const std::string& ambulanceId,
                                                       const std::string& hospitalId) {
    std::lock_guard<std::recursive_mutex> lock(m_engineMutex);

    GeneratedEmergency emg;
    switch (type) {
        case EmergencyType::CARDIAC_ARREST:
        case EmergencyType::HEART_ATTACK:
            emg = m_emergencyGenerator.GenerateCardiacArrest();
            break;
        case EmergencyType::ACUTE_STROKE:
            emg = m_emergencyGenerator.GenerateStroke();
            break;
        case EmergencyType::ROAD_ACCIDENT:
            emg = m_emergencyGenerator.GenerateRoadAccident();
            break;
        case EmergencyType::MASS_CASUALTY:
            emg = m_emergencyGenerator.GenerateMassCasualty();
            break;
        default:
            emg = m_emergencyGenerator.GenerateRandomEmergency();
            break;
    }

    auto amb = m_cityModel.GetAmbulance(ambulanceId);
    auto hosp = m_cityModel.GetHospital(hospitalId);

    if (amb && hosp) {
        std::vector<std::pair<double, double>> route = {
            {amb->GetLatitude(), amb->GetLongitude()},
            {emg.latitude, emg.longitude},
            {hosp->GetLatitude(), hosp->GetLongitude()}
        };

        amb->Dispatch("MISSION_" + emg.incidentId,
                      emg.incidentId,
                      emg.latitude,
                      emg.longitude,
                      hospitalId,
                      hosp->GetLatitude(),
                      hosp->GetLongitude(),
                      emg.patient,
                      route);

        hosp->ReserveBed(emg.requiresIcu);
    }

    m_cityModel.AddEmergency(emg);
    return emg;
}

// ==============================================================================
// Multi-threaded Worker Thread Loops
// ==============================================================================

void SimulationEngine::MasterClockWorker() {
    while (m_running.load()) {
        std::unique_lock<std::mutex> lock(m_stopMutex);
        m_stopCv.wait_for(lock, std::chrono::milliseconds(25), [this] {
            return !m_running.load();
        });

        if (!m_running.load()) {
            break;
        }

        if (!m_clock.IsPaused()) {
            double wallDt = 0.025;
            m_clock.Advance(wallDt);
        }
    }
}

void SimulationEngine::TrafficWorker() {
    while (m_running.load()) {
        std::unique_lock<std::mutex> lock(m_stopMutex);
        m_stopCv.wait_for(lock, std::chrono::milliseconds(200), [this] {
            return !m_running.load();
        });

        if (!m_running.load()) {
            break;
        }

        if (!m_clock.IsPaused()) {
            std::lock_guard<std::recursive_mutex> engLock(m_engineMutex);
            (void)engLock; // Used for synchronization
        }
    }
}

void SimulationEngine::VehicleWorker() {
    while (m_running.load()) {
        std::unique_lock<std::mutex> lock(m_stopMutex);
        m_stopCv.wait_for(lock, std::chrono::milliseconds(40), [this] {
            return !m_running.load();
        });

        if (!m_running.load()) {
            break;
        }

        if (!m_clock.IsPaused()) {
            double dtSec = 0.040 * m_clock.GetTimeScale();
            std::lock_guard<std::recursive_mutex> engLock(m_engineMutex);
            m_cityModel.Update(dtSec, m_clock.GetCurrentHourOfDay());
            if (m_demoMode == DemonstrationMode::MODE_3_DUAL_AMBULANCE_CONFLICT) {
                UpdateConflictArbitration();
            }
        }
    }
}

void SimulationEngine::EmergencyWorker() {
    int cycleCount = 0;
    while (m_running.load()) {
        std::unique_lock<std::mutex> lock(m_stopMutex);
        m_stopCv.wait_for(lock, std::chrono::milliseconds(500), [this] {
            return !m_running.load();
        });

        if (!m_running.load()) {
            break;
        }

        if (!m_clock.IsPaused() && m_demoMode == DemonstrationMode::MODE_1_SOFTWARE_SIMULATION) {
            cycleCount++;
            if (cycleCount >= 20) { // Every ~10 seconds
                cycleCount = 0;
                std::lock_guard<std::recursive_mutex> engLock(m_engineMutex);
                auto emg = m_emergencyGenerator.GenerateRandomEmergency();
                m_cityModel.AddEmergency(emg);
            }
        }
    }
}

void SimulationEngine::HospitalWorker() {
    while (m_running.load()) {
        std::unique_lock<std::mutex> lock(m_stopMutex);
        m_stopCv.wait_for(lock, std::chrono::milliseconds(300), [this] {
            return !m_running.load();
        });

        if (!m_running.load()) {
            break;
        }
    }
}

void SimulationEngine::DashboardWorker() {
    while (m_running.load()) {
        std::unique_lock<std::mutex> lock(m_stopMutex);
        m_stopCv.wait_for(lock, std::chrono::milliseconds(100), [this] {
            return !m_running.load();
        });

        if (!m_running.load()) {
            break;
        }

        std::lock_guard<std::recursive_mutex> engLock(m_engineMutex);
        m_dashboardAdapter.PublishSnapshot(m_cityModel, m_clock.GetElapsedSimSeconds());
    }
}

} // namespace simulator
} // namespace aegis
