/**
 * @file vitals_monitor.hpp
 * @brief Interfaces with biomedical monitors and detects acute clinical deterioration.
 */

#ifndef AEGIS_EDGE_VITALS_MONITOR_HPP
#define AEGIS_EDGE_VITALS_MONITOR_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>
#include <thread>
#include "aegis/common/types.hpp"

namespace aegis {
namespace edge {
namespace ambulance {

struct PatientVitals {
    uint16_t heartRate{75};
    uint8_t spO2{98};
    uint16_t systolicBP{120};
    uint16_t diastolicBP{80};
    uint8_t respirationRate{16};
    uint8_t gcsScore{15};
    float temperatureCelsius{37.0f};
    uint64_t timestampMs{0};
};

enum class VitalsAlarmType : uint8_t {
    NONE = 0,
    LOW_SPO2,
    CARDIAC_ARREST,
    HIGH_TEMPERATURE,
    CRITICAL_BP
};

struct VitalsAlert {
    VitalsAlarmType alarmType{VitalsAlarmType::NONE};
    std::string description;
    std::string triggeredValue;
    std::string severity; ///< "WARNING", "CRITICAL"
    uint64_t timestampMs{0};
};

enum class PatientCondition : uint8_t {
    NORMAL = 0,
    CARDIAC_CRISIS,
    HYPOXIA,
    SEPTIC_FEVER,
    HYPOTENSIVE_SHOCK
};

/**
 * @brief Physiological telemetry monitor with clinical alert triggers and simulation engine.
 */
class VitalsMonitor {
public:
    using AlarmCallback = std::function<void(const VitalsAlert&)>;

    VitalsMonitor();
    ~VitalsMonitor();

    // Lifecycle
    bool Initialize(const std::string& sensorDevicePath = "");
    bool Start();
    void Stop();
    bool IsRunning() const noexcept;

    // Sampling & Alerts
    PatientVitals SampleVitals();
    PatientVitals GetLatestVitals() const;
    bool DetectAcuteDeterioration(const PatientVitals& v) const;
    std::vector<VitalsAlert> EvaluateAlarms(const PatientVitals& v) const;

    // Simulation & Fault Injection API
    void SetSimulationMode(bool enabled);
    void SetSimulationProfile(PatientCondition condition);
    void InjectVitals(const PatientVitals& vitals);
    void TriggerAcuteEvent(VitalsAlarmType alarmType);

    // Callbacks
    void RegisterAlarmCallback(AlarmCallback callback);

private:
    void RunSamplingLoop();
    void GenerateSimulatedVitals();

    std::string m_sensorDevicePath;
    mutable std::mutex m_mutex;
    PatientVitals m_currentVitals;
    bool m_simulationMode{true};
    PatientCondition m_simCondition{PatientCondition::NORMAL};

    std::atomic<bool> m_isRunning{false};
    std::thread m_samplingThread;
    AlarmCallback m_alarmCallback{nullptr};
};

} // namespace ambulance
} // namespace edge
} // namespace aegis

#endif // AEGIS_EDGE_VITALS_MONITOR_HPP
