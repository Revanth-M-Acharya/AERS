/**
 * @file vitals_monitor.cpp
 * @brief Implementation of biomedical vitals monitoring, physiological simulation, and alarm triggers.
 */

#include "aegis/edge/vitals_monitor.hpp"
#include "aegis/common/time_utils.hpp"
#include "aegis/logger/logger.hpp"

#include <chrono>

namespace aegis {
namespace edge {
namespace ambulance {

VitalsMonitor::VitalsMonitor() {
    m_currentVitals.heartRate = 75;
    m_currentVitals.spO2 = 98;
    m_currentVitals.systolicBP = 120;
    m_currentVitals.diastolicBP = 80;
    m_currentVitals.respirationRate = 16;
    m_currentVitals.gcsScore = 15;
    m_currentVitals.temperatureCelsius = 37.0f;
    m_currentVitals.timestampMs = common::GetUtcMilliseconds();
}

VitalsMonitor::~VitalsMonitor() {
    Stop();
}

bool VitalsMonitor::Initialize(const std::string& sensorDevicePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sensorDevicePath = sensorDevicePath;
    m_currentVitals.timestampMs = common::GetUtcMilliseconds();
    return true;
}

bool VitalsMonitor::Start() {
    if (m_isRunning.load()) {
        return true;
    }
    m_isRunning.store(true);
    m_samplingThread = std::thread(&VitalsMonitor::RunSamplingLoop, this);
    return true;
}

void VitalsMonitor::Stop() {
    if (m_isRunning.load()) {
        m_isRunning.store(false);
        if (m_samplingThread.joinable()) {
            m_samplingThread.join();
        }
    }
}

bool VitalsMonitor::IsRunning() const noexcept {
    return m_isRunning.load();
}

PatientVitals VitalsMonitor::SampleVitals() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_simulationMode) {
        GenerateSimulatedVitals();
    }
    m_currentVitals.timestampMs = common::GetUtcMilliseconds();
    return m_currentVitals;
}

PatientVitals VitalsMonitor::GetLatestVitals() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentVitals;
}

bool VitalsMonitor::DetectAcuteDeterioration(const PatientVitals& v) const {
    // 1. Hypoxia threshold: SpO2 < 88%
    if (v.spO2 > 0 && v.spO2 < 88) {
        return true;
    }
    // 2. Cardiac arrest / severe arrhythmia
    if (v.heartRate == 0 || v.heartRate > 200 || (v.heartRate < 40 && v.heartRate > 0)) {
        return true;
    }
    // 3. Hypotensive crisis or malignant hypertension
    if (v.systolicBP > 0 && (v.systolicBP < 80 || v.systolicBP > 190)) {
        return true;
    }
    if (v.diastolicBP > 0 && (v.diastolicBP < 50 || v.diastolicBP > 120)) {
        return true;
    }
    // 4. Critical Hyperthermia / Hypothermia
    if (v.temperatureCelsius >= 39.5f || (v.temperatureCelsius < 34.0f && v.temperatureCelsius > 0.0f)) {
        return true;
    }
    return false;
}

std::vector<VitalsAlert> VitalsMonitor::EvaluateAlarms(const PatientVitals& v) const {
    std::vector<VitalsAlert> alerts;
    uint64_t now = common::GetUtcMilliseconds();

    // Cardiac Arrest
    if (v.heartRate == 0) {
        VitalsAlert alert;
        alert.alarmType = VitalsAlarmType::CARDIAC_ARREST;
        alert.description = "Asystole / Cardiac Arrest detected: Heart rate is 0 bpm";
        alert.triggeredValue = "HR=0";
        alert.severity = "CRITICAL";
        alert.timestampMs = now;
        alerts.push_back(alert);
    } else if (v.heartRate > 200) {
        VitalsAlert alert;
        alert.alarmType = VitalsAlarmType::CARDIAC_ARREST;
        alert.description = "Severe tachyarrhythmia detected: Heart rate exceeds 200 bpm";
        alert.triggeredValue = "HR=" + std::to_string(v.heartRate);
        alert.severity = "CRITICAL";
        alert.timestampMs = now;
        alerts.push_back(alert);
    }

    // Low SpO2
    if (v.spO2 > 0 && v.spO2 < 88) {
        VitalsAlert alert;
        alert.alarmType = VitalsAlarmType::LOW_SPO2;
        alert.description = "Acute Hypoxia: SpO2 saturation below critical safety threshold (88%)";
        alert.triggeredValue = "SpO2=" + std::to_string(v.spO2) + "%";
        alert.severity = "CRITICAL";
        alert.timestampMs = now;
        alerts.push_back(alert);
    }

    // Critical Blood Pressure
    if (v.systolicBP > 0 && v.systolicBP < 80) {
        VitalsAlert alert;
        alert.alarmType = VitalsAlarmType::CRITICAL_BP;
        alert.description = "Acute Hypotensive Shock: Systolic blood pressure below 80 mmHg";
        alert.triggeredValue = "BP=" + std::to_string(v.systolicBP) + "/" + std::to_string(v.diastolicBP);
        alert.severity = "CRITICAL";
        alert.timestampMs = now;
        alerts.push_back(alert);
    } else if (v.systolicBP > 190) {
        VitalsAlert alert;
        alert.alarmType = VitalsAlarmType::CRITICAL_BP;
        alert.description = "Hypertensive Emergency: Systolic blood pressure exceeds 190 mmHg";
        alert.triggeredValue = "BP=" + std::to_string(v.systolicBP) + "/" + std::to_string(v.diastolicBP);
        alert.severity = "CRITICAL";
        alert.timestampMs = now;
        alerts.push_back(alert);
    }

    // High Temperature
    if (v.temperatureCelsius >= 39.5f) {
        VitalsAlert alert;
        alert.alarmType = VitalsAlarmType::HIGH_TEMPERATURE;
        alert.description = "Severe Hyperthermia / Septic Pyrexia: Core temperature exceeds 39.5 C";
        alert.triggeredValue = "Temp=" + std::to_string(v.temperatureCelsius) + "C";
        alert.severity = "CRITICAL";
        alert.timestampMs = now;
        alerts.push_back(alert);
    }

    return alerts;
}

void VitalsMonitor::SetSimulationMode(bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_simulationMode = enabled;
}

void VitalsMonitor::SetSimulationProfile(PatientCondition condition) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_simCondition = condition;
    GenerateSimulatedVitals();
}

void VitalsMonitor::InjectVitals(const PatientVitals& vitals) {
    AlarmCallback callbackCopy{nullptr};
    std::vector<VitalsAlert> alerts;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_currentVitals = vitals;
        m_currentVitals.timestampMs = common::GetUtcMilliseconds();
        alerts = EvaluateAlarms(m_currentVitals);
        callbackCopy = m_alarmCallback;
    }

    if (callbackCopy) {
        for (const auto& alert : alerts) {
            callbackCopy(alert);
        }
    }
}

void VitalsMonitor::TriggerAcuteEvent(VitalsAlarmType alarmType) {
    AlarmCallback callbackCopy{nullptr};
    std::vector<VitalsAlert> alerts;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        uint64_t now = common::GetUtcMilliseconds();
        m_currentVitals.timestampMs = now;

        switch (alarmType) {
            case VitalsAlarmType::CARDIAC_ARREST:
                m_currentVitals.heartRate = 0;
                m_currentVitals.systolicBP = 0;
                m_currentVitals.diastolicBP = 0;
                m_currentVitals.respirationRate = 0;
                m_currentVitals.gcsScore = 3;
                break;
            case VitalsAlarmType::LOW_SPO2:
                m_currentVitals.spO2 = 82;
                m_currentVitals.heartRate = 115;
                m_currentVitals.respirationRate = 28;
                break;
            case VitalsAlarmType::CRITICAL_BP:
                m_currentVitals.systolicBP = 70;
                m_currentVitals.diastolicBP = 40;
                m_currentVitals.heartRate = 135;
                break;
            case VitalsAlarmType::HIGH_TEMPERATURE:
                m_currentVitals.temperatureCelsius = 40.2f;
                m_currentVitals.heartRate = 120;
                break;
            case VitalsAlarmType::NONE:
            default:
                m_currentVitals.heartRate = 75;
                m_currentVitals.spO2 = 98;
                m_currentVitals.systolicBP = 120;
                m_currentVitals.diastolicBP = 80;
                m_currentVitals.temperatureCelsius = 37.0f;
                m_currentVitals.respirationRate = 16;
                m_currentVitals.gcsScore = 15;
                break;
        }

        alerts = EvaluateAlarms(m_currentVitals);
        callbackCopy = m_alarmCallback;
    }

    if (callbackCopy) {
        for (const auto& alert : alerts) {
            callbackCopy(alert);
        }
    }
}

void VitalsMonitor::RegisterAlarmCallback(AlarmCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_alarmCallback = std::move(callback);
}

void VitalsMonitor::GenerateSimulatedVitals() {
    uint64_t now = common::GetUtcMilliseconds();
    m_currentVitals.timestampMs = now;

    switch (m_simCondition) {
        case PatientCondition::NORMAL:
            m_currentVitals.heartRate = 76;
            m_currentVitals.spO2 = 98;
            m_currentVitals.systolicBP = 122;
            m_currentVitals.diastolicBP = 82;
            m_currentVitals.respirationRate = 16;
            m_currentVitals.temperatureCelsius = 37.0f;
            m_currentVitals.gcsScore = 15;
            break;
        case PatientCondition::CARDIAC_CRISIS:
            m_currentVitals.heartRate = 0;
            m_currentVitals.spO2 = 85;
            m_currentVitals.systolicBP = 50;
            m_currentVitals.diastolicBP = 30;
            m_currentVitals.respirationRate = 0;
            m_currentVitals.temperatureCelsius = 36.4f;
            m_currentVitals.gcsScore = 3;
            break;
        case PatientCondition::HYPOXIA:
            m_currentVitals.heartRate = 118;
            m_currentVitals.spO2 = 84;
            m_currentVitals.systolicBP = 110;
            m_currentVitals.diastolicBP = 75;
            m_currentVitals.respirationRate = 28;
            m_currentVitals.temperatureCelsius = 37.2f;
            m_currentVitals.gcsScore = 13;
            break;
        case PatientCondition::SEPTIC_FEVER:
            m_currentVitals.heartRate = 122;
            m_currentVitals.spO2 = 93;
            m_currentVitals.systolicBP = 96;
            m_currentVitals.diastolicBP = 58;
            m_currentVitals.respirationRate = 24;
            m_currentVitals.temperatureCelsius = 40.1f;
            m_currentVitals.gcsScore = 14;
            break;
        case PatientCondition::HYPOTENSIVE_SHOCK:
            m_currentVitals.heartRate = 132;
            m_currentVitals.spO2 = 91;
            m_currentVitals.systolicBP = 72;
            m_currentVitals.diastolicBP = 45;
            m_currentVitals.respirationRate = 22;
            m_currentVitals.temperatureCelsius = 36.0f;
            m_currentVitals.gcsScore = 12;
            break;
    }
}

void VitalsMonitor::RunSamplingLoop() {
    // 1 Hz physiological sampling loop (1000 ms period)
    while (m_isRunning.load()) {
        auto startTime = std::chrono::steady_clock::now();

        PatientVitals sampled = SampleVitals();
        auto alerts = EvaluateAlarms(sampled);

        AlarmCallback cbCopy{nullptr};
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            cbCopy = m_alarmCallback;
        }

        if (cbCopy) {
            for (const auto& alert : alerts) {
                cbCopy(alert);
            }
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        if (elapsed < 1000) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 - elapsed));
        }
    }
}

} // namespace ambulance
} // namespace edge
} // namespace aegis
