/**
 * @file PatientProfile.h
 * @brief Comprehensive Patient Profile and Vitals Data Model for SMART AEGIS.
 */

#ifndef AEGIS_MEDICAL_PATIENT_PROFILE_H
#define AEGIS_MEDICAL_PATIENT_PROFILE_H

#include <string>
#include <cstdint>
#include <vector>
#include <optional>
#include <sstream>

namespace aegis {
namespace medical {

/**
 * @brief Biological sex / gender classification.
 */
enum class Gender {
    MALE,
    FEMALE,
    OTHER,
    UNKNOWN
};

inline std::string GenderToString(Gender gender) {
    switch (gender) {
        case Gender::MALE:    return "Male";
        case Gender::FEMALE:  return "Female";
        case Gender::OTHER:   return "Other";
        default:              return "Unknown";
    }
}

inline Gender StringToGender(const std::string& str) {
    if (str == "Male" || str == "MALE" || str == "m") return Gender::MALE;
    if (str == "Female" || str == "FEMALE" || str == "f") return Gender::FEMALE;
    if (str == "Other" || str == "OTHER") return Gender::OTHER;
    return Gender::UNKNOWN;
}

/**
 * @brief Emergency incident clinical category / disease classification.
 */
enum class EmergencyType {
    HEART_ATTACK,
    CARDIAC_ARREST,
    STROKE,
    MAJOR_ACCIDENT,
    ROAD_ACCIDENT,
    PREGNANCY,
    BURN_INJURY,
    TRAUMA,
    INTERNAL_BLEEDING,
    RESPIRATORY_FAILURE,
    POISONING,
    PEDIATRIC_EMERGENCY,
    ORGAN_TRANSPORT,
    UNKNOWN
};

inline std::string EmergencyTypeToString(EmergencyType type) {
    switch (type) {
        case EmergencyType::HEART_ATTACK:        return "Heart Attack";
        case EmergencyType::CARDIAC_ARREST:      return "Cardiac Arrest";
        case EmergencyType::STROKE:              return "Stroke";
        case EmergencyType::MAJOR_ACCIDENT:      return "Major Accident";
        case EmergencyType::ROAD_ACCIDENT:       return "Road Accident";
        case EmergencyType::PREGNANCY:           return "Pregnancy";
        case EmergencyType::BURN_INJURY:         return "Burn Injury";
        case EmergencyType::TRAUMA:              return "Trauma";
        case EmergencyType::INTERNAL_BLEEDING:   return "Internal Bleeding";
        case EmergencyType::RESPIRATORY_FAILURE: return "Respiratory Failure";
        case EmergencyType::POISONING:           return "Poisoning";
        case EmergencyType::PEDIATRIC_EMERGENCY: return "Pediatric Emergency";
        case EmergencyType::ORGAN_TRANSPORT:     return "Organ Transport";
        default:                                 return "Unknown";
    }
}

inline EmergencyType StringToEmergencyType(const std::string& str) {
    if (str == "Heart Attack" || str == "HEART_ATTACK") return EmergencyType::HEART_ATTACK;
    if (str == "Cardiac Arrest" || str == "CARDIAC_ARREST") return EmergencyType::CARDIAC_ARREST;
    if (str == "Stroke" || str == "STROKE") return EmergencyType::STROKE;
    if (str == "Major Accident" || str == "MAJOR_ACCIDENT") return EmergencyType::MAJOR_ACCIDENT;
    if (str == "Road Accident" || str == "ROAD_ACCIDENT") return EmergencyType::ROAD_ACCIDENT;
    if (str == "Pregnancy" || str == "PREGNANCY") return EmergencyType::PREGNANCY;
    if (str == "Burn Injury" || str == "BURN_INJURY") return EmergencyType::BURN_INJURY;
    if (str == "Trauma" || str == "TRAUMA") return EmergencyType::TRAUMA;
    if (str == "Internal Bleeding" || str == "INTERNAL_BLEEDING") return EmergencyType::INTERNAL_BLEEDING;
    if (str == "Respiratory Failure" || str == "RESPIRATORY_FAILURE") return EmergencyType::RESPIRATORY_FAILURE;
    if (str == "Poisoning" || str == "POISONING") return EmergencyType::POISONING;
    if (str == "Pediatric Emergency" || str == "PEDIATRIC_EMERGENCY") return EmergencyType::PEDIATRIC_EMERGENCY;
    if (str == "Organ Transport" || str == "ORGAN_TRANSPORT") return EmergencyType::ORGAN_TRANSPORT;
    return EmergencyType::UNKNOWN;
}

/**
 * @brief Arterial blood pressure measurements in mmHg.
 */
struct BloodPressure {
    uint16_t systolic{120};   ///< Systolic blood pressure (mmHg)
    uint16_t diastolic{80};   ///< Diastolic blood pressure (mmHg)

    BloodPressure() = default;
    BloodPressure(uint16_t sys, uint16_t dia) : systolic(sys), diastolic(dia) {}

    std::string ToString() const {
        return std::to_string(systolic) + "/" + std::to_string(diastolic) + " mmHg";
    }
};

/**
 * @brief Real-time physiological vitals telemetry.
 */
struct PatientVitals {
    uint16_t heartRate{75};                       ///< Beats per minute (bpm)
    BloodPressure bloodPressure{120, 80};         ///< Arterial blood pressure
    uint16_t spO2{98};                            ///< Oxygen saturation (percentage 0-100)
    uint16_t respirationRate{16};                 ///< Breaths per minute (bpm)
    double bloodLossPercentage{0.0};              ///< Estimated blood loss (0.0 to 100.0%)
    uint8_t painLevel{0};                         ///< Numeric pain rating scale (0 to 10)
    bool isConscious{true};                       ///< Consciousness status (true = conscious, false = unconscious)
    uint32_t estimatedTimeSinceIncidentMinutes{0};///< Time elapsed since acute onset (minutes)

    PatientVitals() = default;

    PatientVitals(uint16_t hr,
                  BloodPressure bp,
                  uint16_t o2,
                  uint16_t rr,
                  double bloodLoss,
                  bool conscious,
                  uint32_t timeSinceIncident,
                  uint8_t pain = 0)
        : heartRate(hr)
        , bloodPressure(bp)
        , spO2(o2)
        , respirationRate(rr)
        , bloodLossPercentage(bloodLoss)
        , painLevel(pain)
        , isConscious(conscious)
        , estimatedTimeSinceIncidentMinutes(timeSinceIncident) {}
};

/**
 * @brief Medical record encapsulating patient demographics, disease/emergency classification, vitals, and remarks.
 */
class PatientProfile {
public:
    PatientProfile();

    PatientProfile(std::string patientId,
                   std::string patientName,
                   uint16_t age,
                   Gender gender,
                   EmergencyType emergencyType,
                   PatientVitals vitals = PatientVitals{},
                   std::string remarks = "");

    ~PatientProfile() = default;

    // Demographic Accessors
    const std::string& GetPatientId() const noexcept { return m_patientId; }
    void SetPatientId(std::string id) { m_patientId = std::move(id); }

    const std::string& GetPatientName() const noexcept { return m_patientName; }
    void SetPatientName(std::string name) { m_patientName = std::move(name); }

    uint16_t GetAge() const noexcept { return m_age; }
    void SetAge(uint16_t age) noexcept { m_age = age; }

    Gender GetGender() const noexcept { return m_gender; }
    void SetGender(Gender gender) noexcept { m_gender = gender; }
    std::string GetGenderString() const { return GenderToString(m_gender); }

    // Emergency Classification / Disease Accessors
    EmergencyType GetEmergencyType() const noexcept { return m_emergencyType; }
    void SetEmergencyType(EmergencyType type) noexcept { m_emergencyType = type; }
    std::string GetEmergencyTypeString() const { return EmergencyTypeToString(m_emergencyType); }

    // Disease alias methods
    EmergencyType GetDisease() const noexcept { return m_emergencyType; }
    void SetDisease(EmergencyType disease) noexcept { m_emergencyType = disease; }
    std::string GetDiseaseString() const { return EmergencyTypeToString(m_emergencyType); }

    // Remarks Accessors
    const std::string& GetRemarks() const noexcept { return m_remarks; }
    void SetRemarks(std::string remarks) { m_remarks = std::move(remarks); }

    // Vitals Accessors
    const PatientVitals& GetVitals() const noexcept { return m_vitals; }
    PatientVitals& GetVitals() noexcept { return m_vitals; }
    void SetVitals(PatientVitals vitals) noexcept { m_vitals = vitals; }

    // Convenient Direct Vitals Setters & Getters
    uint16_t GetHeartRate() const noexcept { return m_vitals.heartRate; }
    void SetHeartRate(uint16_t hr) noexcept { m_vitals.heartRate = hr; }

    const BloodPressure& GetBloodPressure() const noexcept { return m_vitals.bloodPressure; }
    void SetBloodPressure(uint16_t sys, uint16_t dia) noexcept { m_vitals.bloodPressure = BloodPressure{sys, dia}; }

    uint16_t GetSpO2() const noexcept { return m_vitals.spO2; }
    void SetSpO2(uint16_t o2) noexcept { m_vitals.spO2 = o2; }

    uint16_t GetRespirationRate() const noexcept { return m_vitals.respirationRate; }
    void SetRespirationRate(uint16_t rr) noexcept { m_vitals.respirationRate = rr; }

    double GetBloodLossPercentage() const noexcept { return m_vitals.bloodLossPercentage; }
    void SetBloodLossPercentage(double loss) noexcept { m_vitals.bloodLossPercentage = loss; }

    uint8_t GetPainLevel() const noexcept { return m_vitals.painLevel; }
    void SetPainLevel(uint8_t pain) noexcept { m_vitals.painLevel = pain; }

    bool IsConscious() const noexcept { return m_vitals.isConscious; }
    void SetConscious(bool conscious) noexcept { m_vitals.isConscious = conscious; }

    uint32_t GetEstimatedTimeSinceIncident() const noexcept { return m_vitals.estimatedTimeSinceIncidentMinutes; }
    void SetEstimatedTimeSinceIncident(uint32_t mins) noexcept { m_vitals.estimatedTimeSinceIncidentMinutes = mins; }

    // Inspection & Validation
    bool IsValid() const noexcept;
    std::string ToString() const;
    std::string ToJson() const;

private:
    std::string m_patientId{""};
    std::string m_patientName{""};
    uint16_t m_age{0};
    Gender m_gender{Gender::UNKNOWN};
    EmergencyType m_emergencyType{EmergencyType::UNKNOWN};
    PatientVitals m_vitals{};
    std::string m_remarks{""};
};

} // namespace medical
} // namespace aegis

#endif // AEGIS_MEDICAL_PATIENT_PROFILE_H
