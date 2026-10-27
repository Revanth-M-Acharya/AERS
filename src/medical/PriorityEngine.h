/**
 * @file PriorityEngine.h
 * @brief Clinical Emergency Scoring and Priority Assessment Engine for SMART AEGIS.
 */

#ifndef AEGIS_MEDICAL_PRIORITY_ENGINE_H
#define AEGIS_MEDICAL_PRIORITY_ENGINE_H

#include "medical/PatientProfile.h"
#include <string>
#include <vector>

namespace aegis {
namespace medical {

/**
 * @brief Categorical priority levels for emergency green corridor dispatch.
 */
enum class PriorityLevel {
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
};

/**
 * @brief Converts PriorityLevel enum to string representation.
 */
inline std::string PriorityLevelToString(PriorityLevel level) {
    switch (level) {
        case PriorityLevel::LOW:      return "LOW";
        case PriorityLevel::MEDIUM:   return "MEDIUM";
        case PriorityLevel::HIGH:     return "HIGH";
        case PriorityLevel::CRITICAL: return "CRITICAL";
        default:                      return "UNKNOWN";
    }
}

/**
 * @brief Parses string representation into PriorityLevel enum.
 */
inline PriorityLevel StringToPriorityLevel(const std::string& str) {
    if (str == "LOW" || str == "low") return PriorityLevel::LOW;
    if (str == "MEDIUM" || str == "medium") return PriorityLevel::MEDIUM;
    if (str == "HIGH" || str == "high") return PriorityLevel::HIGH;
    if (str == "CRITICAL" || str == "critical") return PriorityLevel::CRITICAL;
    return PriorityLevel::LOW;
}

/**
 * @brief Detailed breakdown of emergency score components.
 */
struct EmergencyScoreBreakdown {
    double diseaseScore{0.0};
    double vitalsScore{0.0};
    double ageScore{0.0};
    double bloodLossScore{0.0};
    double consciousnessScore{0.0};
    double timeFactorScore{0.0};
    double painScore{0.0};
    double totalScore{0.0};
    PriorityLevel priority{PriorityLevel::LOW};
    double survivalRiskPercentage{0.0};
    std::string estimatedSurvivalRisk{""};
    uint32_t recommendedMaxETAMinutes{30};
    std::vector<std::string> contributingFactors{};
};

/**
 * @brief Clinical Decision Support Engine that dynamically evaluates patient urgency.
 */
class PriorityEngine {
public:
    PriorityEngine() = default;
    ~PriorityEngine() = default;

    // Threshold constants for triage categorization
    static constexpr double THRESHOLD_CRITICAL = 75.0;
    static constexpr double THRESHOLD_HIGH     = 55.0;
    static constexpr double THRESHOLD_MEDIUM   = 35.0;

    /**
     * @brief Computes categorical priority (LOW, MEDIUM, HIGH, CRITICAL) for a patient.
     * @param profile The patient profile to triage.
     * @return Triage PriorityLevel.
     */
    static PriorityLevel CalculatePriority(const PatientProfile& profile);

    /**
     * @brief Computes normalized continuous numerical emergency score (0.0 to 100.0).
     * @param profile The patient profile to score.
     * @return Numerical emergency score.
     */
    static double CalculateEmergencyScore(const PatientProfile& profile);

    /**
     * @brief Generates detailed clinical justification text explaining triage level.
     * @param profile The patient profile to explain.
     * @return Human-readable clinical justification.
     */
    static std::string GeneratePriorityReason(const PatientProfile& profile);

    /**
     * @brief Estimates clinical mortality/morbidity survival risk.
     * @param profile The patient profile to assess.
     * @return Human-readable survival risk description.
     */
    static std::string EstimateSurvivalRisk(const PatientProfile& profile);

    /**
     * @brief Computes survival risk percentage (0.0 to 100.0%).
     * @param profile The patient profile to assess.
     * @return Mortality/morbidity risk percentage.
     */
    static double CalculateSurvivalRiskPercentage(const PatientProfile& profile);

    /**
     * @brief Computes recommended maximum transit time before irreversible decompensation.
     * @param profile The patient profile to assess.
     * @return Maximum recommended ETA in minutes.
     */
    static uint32_t CalculateRecommendedMaxETAMinutes(const PatientProfile& profile);

    /**
     * @brief Returns recommended maximum transit time as formatted string.
     * @param profile The patient profile to assess.
     * @return Formatted recommended ETA string.
     */
    static std::string GetRecommendedMaxETAString(const PatientProfile& profile);

    /**
     * @brief Computes granular factor breakdown for telemetry and diagnostic auditing.
     * @param profile The patient profile to inspect.
     * @return Breakdown of each contributing medical factor.
     */
    static EmergencyScoreBreakdown EvaluateBreakdown(const PatientProfile& profile);

private:
    static double CalculateDiseaseScore(EmergencyType type, std::vector<std::string>& reasons);
    static double CalculateVitalsScore(const PatientVitals& vitals, std::vector<std::string>& reasons);
    static double CalculateAgeScore(uint16_t age, std::vector<std::string>& reasons);
    static double CalculateBloodLossScore(double bloodLossPercent, std::vector<std::string>& reasons);
    static double CalculateConsciousnessScore(bool isConscious, std::vector<std::string>& reasons);
    static double CalculateTimeFactorScore(uint32_t minutesElapsed, std::vector<std::string>& reasons);
    static double CalculatePainScore(uint8_t painLevel, std::vector<std::string>& reasons);
};

} // namespace medical
} // namespace aegis

#endif // AEGIS_MEDICAL_PRIORITY_ENGINE_H
