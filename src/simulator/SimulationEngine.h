/**
 * @file SimulationEngine.h
 * @brief SMART AEGIS (AERS) Phase 19 - Master Municipal City Simulation Engine (Digital Twin).
 */

#ifndef AEGIS_SIMULATOR_SIMULATION_ENGINE_H
#define AEGIS_SIMULATOR_SIMULATION_ENGINE_H

#include "CityClock.h"
#include "CityModel.h"
#include "EmergencyGenerator.h"
#include "DashboardAdapter.h"

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace aegis {
namespace simulator {

/**
 * @brief Three official simulation demonstration modes.
 */
enum class DemonstrationMode : uint8_t {
    MODE_1_SOFTWARE_SIMULATION = 1,
    MODE_2_SINGLE_AMBULANCE_HARDWARE = 2,
    MODE_3_DUAL_AMBULANCE_CONFLICT = 3
};

std::string DemonstrationModeToString(DemonstrationMode mode);

/**
 * @brief Telemetry status for Mode 3 intersection conflict arbitration.
 */
struct ConflictArbitrationStatus {
    bool conflictDetected{false};
    std::string intersectionId;
    std::string grantedAmbulanceId;
    std::string heldAmbulanceId;
    double grantedPriorityScore{0.0};
    double heldPriorityScore{0.0};
    bool isCleared{false};
};

/**
 * @brief Master simulation engine governing digital twin execution, multi-threading,
 * clock synchronization, demonstration modes, and dashboard publication.
 */
class SimulationEngine {
public:
    explicit SimulationEngine(const std::string& configFilePath = "config/simulation.json");
    ~SimulationEngine();

    // Prevent copy/move
    SimulationEngine(const SimulationEngine&) = delete;
    SimulationEngine& operator=(const SimulationEngine&) = delete;
    SimulationEngine(SimulationEngine&&) = delete;
    SimulationEngine& operator=(SimulationEngine&&) = delete;

    // Initialization & Lifecycle
    bool Initialize(const std::string& configFilePath = "config/simulation.json");
    void Start();
    void Stop();
    bool IsRunning() const;

    // Clock & Speed Controls
    void Pause();
    void Resume();
    void TogglePause();
    bool IsPaused() const;
    void Reset();
    void Step(double dtSec = 0.05);

    void SetSpeedMultiplier(SimulationSpeedMultiplier multiplier);
    SimulationSpeedMultiplier GetSpeedMultiplier() const;
    void SetTimeScale(double scale);
    double GetTimeScale() const;

    // Demonstration Modes
    void SetDemonstrationMode(DemonstrationMode mode);
    DemonstrationMode GetDemonstrationMode() const;
    ConflictArbitrationStatus GetConflictStatus() const;

    // Subsystem Accessors
    CityClock& GetClock() { return m_clock; }
    const CityClock& GetClock() const { return m_clock; }

    CityModel& GetCityModel() { return m_cityModel; }
    const CityModel& GetCityModel() const { return m_cityModel; }

    EmergencyGenerator& GetEmergencyGenerator() { return m_emergencyGenerator; }
    DashboardAdapter& GetDashboardAdapter() { return m_dashboardAdapter; }

    // Dashboard Integration
    void SetControlCentreDataModel(std::shared_ptr<dashboard::DashboardDataModel> model);
    void SetTrafficDashboardDataModel(std::shared_ptr<dashboard::traffic::TrafficDashboardDataModel> model);

    // Dynamic Disturbance & Environmental Controls
    void TriggerDisruption(const LiveDisruptionEvent& event);
    void ClearDisruptions();

    void SetWeather(WeatherCondition weather);
    WeatherCondition GetWeather() const;

    void SetTrafficDensity(double density);
    double GetTrafficDensity() const;

    // Emergency Mission Dispatching
    GeneratedEmergency DispatchEmergency(EmergencyType type,
                                         const std::string& ambulanceId,
                                         const std::string& hospitalId);

private:
    // Setup handlers for Demonstration Modes
    void ConfigureMode1();
    void ConfigureMode2();
    void ConfigureMode3();

    // Mode 3 Arbitration logic
    void UpdateConflictArbitration();

    // Multi-threaded worker loops
    void MasterClockWorker();
    void TrafficWorker();
    void VehicleWorker();
    void EmergencyWorker();
    void HospitalWorker();
    void DashboardWorker();

    std::string m_configFilePath;
    mutable std::recursive_mutex m_engineMutex;

    CityClock m_clock;
    CityModel m_cityModel;
    EmergencyGenerator m_emergencyGenerator;
    DashboardAdapter m_dashboardAdapter;

    DemonstrationMode m_demoMode{DemonstrationMode::MODE_1_SOFTWARE_SIMULATION};
    ConflictArbitrationStatus m_conflictStatus;

    // Concurrency control
    std::atomic<bool> m_running{false};
    std::condition_variable m_stopCv;
    std::mutex m_stopMutex;

    // Independent worker threads
    std::thread m_clockThread;
    std::thread m_trafficThread;
    std::thread m_vehicleThread;
    std::thread m_emergencyThread;
    std::thread m_hospitalThread;
    std::thread m_dashboardThread;
};

} // namespace simulator
} // namespace aegis

#endif // AEGIS_SIMULATOR_SIMULATION_ENGINE_H
