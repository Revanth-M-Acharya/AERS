/**
 * @file PatientProfile.cpp
 * @brief Implementation of PatientProfile methods and serialization.
 */

#include "medical/PatientProfile.h"
#include <iomanip>
#include <sstream>

namespace aegis {
namespace medical {

PatientProfile::PatientProfile()
    : m_patientId("")
    , m_patientName("")
    , m_age(0)
    , m_gender(Gender::UNKNOWN)
    , m_emergencyType(EmergencyType::UNKNOWN)
    , m_vitals()
    , m_remarks("") {}

PatientProfile::PatientProfile(std::string patientId,
                               std::string patientName,
                               uint16_t age,
                               Gender gender,
                               EmergencyType emergencyType,
                               PatientVitals vitals,
                               std::string remarks)
    : m_patientId(std::move(patientId))
    , m_patientName(std::move(patientName))
    , m_age(age)
    , m_gender(gender)
    , m_emergencyType(emergencyType)
    , m_vitals(vitals)
    , m_remarks(std::move(remarks)) {}

bool PatientProfile::IsValid() const noexcept {
    if (m_patientId.empty() || m_patientName.empty()) {
        return false;
    }
    if (m_emergencyType == EmergencyType::UNKNOWN) {
        return false;
    }
    if (m_vitals.spO2 > 100) {
        return false;
    }
    if (m_vitals.bloodLossPercentage < 0.0 || m_vitals.bloodLossPercentage > 100.0) {
        return false;
    }
    if (m_vitals.painLevel > 10) {
        return false;
    }
    if (m_vitals.heartRate > 300 || m_vitals.respirationRate > 100) {
        return false;
    }
    return true;
}

std::string PatientProfile::ToString() const {
    std::ostringstream oss;
    oss << "PatientProfile [" << m_patientId << "] " << m_patientName
        << " (Age: " << m_age << ", Gender: " << GetGenderString() << ")\n"
        << "  Disease/Type   : " << GetDiseaseString() << "\n"
        << "  Heart Rate     : " << m_vitals.heartRate << " bpm\n"
        << "  Blood Pressure : " << m_vitals.bloodPressure.ToString() << "\n"
        << "  SpO2           : " << m_vitals.spO2 << " %\n"
        << "  Respiration    : " << m_vitals.respirationRate << " bpm\n"
        << "  Blood Loss     : " << std::fixed << std::setprecision(1) << m_vitals.bloodLossPercentage << " %\n"
        << "  Pain Level     : " << static_cast<int>(m_vitals.painLevel) << "/10\n"
        << "  Consciousness  : " << (m_vitals.isConscious ? "Conscious" : "Unconscious") << "\n"
        << "  Time Elapsed   : " << m_vitals.estimatedTimeSinceIncidentMinutes << " mins\n"
        << "  Remarks        : " << (m_remarks.empty() ? "None" : m_remarks);
    return oss.str();
}

std::string PatientProfile::ToJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"patient_id\":\"" << m_patientId << "\","
        << "\"name\":\"" << m_patientName << "\","
        << "\"age\":" << m_age << ","
        << "\"gender\":\"" << GetGenderString() << "\","
        << "\"disease\":\"" << GetDiseaseString() << "\","
        << "\"emergency_type\":\"" << GetEmergencyTypeString() << "\","
        << "\"remarks\":\"" << m_remarks << "\","
        << "\"vitals\":{"
        << "\"heart_rate\":" << m_vitals.heartRate << ","
        << "\"systolic\":" << m_vitals.bloodPressure.systolic << ","
        << "\"diastolic\":" << m_vitals.bloodPressure.diastolic << ","
        << "\"spo2\":" << m_vitals.spO2 << ","
        << "\"respiration_rate\":" << m_vitals.respirationRate << ","
        << "\"blood_loss_percent\":" << std::fixed << std::setprecision(1) << m_vitals.bloodLossPercentage << ","
        << "\"pain_level\":" << static_cast<int>(m_vitals.painLevel) << ","
        << "\"conscious\":" << (m_vitals.isConscious ? "true" : "false") << ","
        << "\"elapsed_minutes\":" << m_vitals.estimatedTimeSinceIncidentMinutes
        << "}"
        << "}";
    return oss.str();
}

} // namespace medical
} // namespace aegis
