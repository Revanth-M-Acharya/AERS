/**
 * @file PriorityEngine.cpp
 * @brief Implementation of PriorityEngine clinical decision scoring.
 */

#include "medical/PriorityEngine.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace aegis {
namespace medical {

PriorityLevel PriorityEngine::CalculatePriority(const PatientProfile& profile) {
    const double score = CalculateEmergencyScore(profile);
    if (score >= THRESHOLD_CRITICAL) {
        return PriorityLevel::CRITICAL;
    }
    if (score >= THRESHOLD_HIGH) {
        return PriorityLevel::HIGH;
    }
    if (score >= THRESHOLD_MEDIUM) {
        return PriorityLevel::MEDIUM;
    }
    return PriorityLevel::LOW;
}

double PriorityEngine::CalculateEmergencyScore(const PatientProfile& profile) {
    EmergencyScoreBreakdown breakdown = EvaluateBreakdown(profile);
    return breakdown.totalScore;
}

std::string PriorityEngine::GeneratePriorityReason(const PatientProfile& profile) {
    EmergencyScoreBreakdown breakdown = EvaluateBreakdown(profile);

    std::ostringstream oss;
    oss << PriorityLevelToString(breakdown.priority) << " Priority (Score: "
        << std::fixed << std::setprecision(1) << breakdown.totalScore << "/100) - "
        << profile.GetEmergencyTypeString();

    if (breakdown.contributingFactors.empty()) {
        oss << "; Vitals stable within normal physiological limits.";
    } else {
        for (const auto& factor : breakdown.contributingFactors) {
            oss << "; " << factor;
        }
    }

    return oss.str();
}

std::string PriorityEngine::EstimateSurvivalRisk(const PatientProfile& profile) {
    EmergencyScoreBreakdown breakdown = EvaluateBreakdown(profile);
    return breakdown.estimatedSurvivalRisk;
}

double PriorityEngine::CalculateSurvivalRiskPercentage(const PatientProfile& profile) {
    EmergencyScoreBreakdown breakdown = EvaluateBreakdown(profile);
    return breakdown.survivalRiskPercentage;
}

uint32_t PriorityEngine::CalculateRecommendedMaxETAMinutes(const PatientProfile& profile) {
    EmergencyScoreBreakdown breakdown = EvaluateBreakdown(profile);
    return breakdown.recommendedMaxETAMinutes;
}

std::string PriorityEngine::GetRecommendedMaxETAString(const PatientProfile& profile) {
    uint32_t mins = CalculateRecommendedMaxETAMinutes(profile);
    return std::to_string(mins) + " minutes";
}

EmergencyScoreBreakdown PriorityEngine::EvaluateBreakdown(const PatientProfile& profile) {
    EmergencyScoreBreakdown breakdown{};
    std::vector<std::string>& factors = breakdown.contributingFactors;

    breakdown.diseaseScore = CalculateDiseaseScore(profile.GetEmergencyType(), factors);
    breakdown.vitalsScore = CalculateVitalsScore(profile.GetVitals(), factors);
    breakdown.ageScore = CalculateAgeScore(profile.GetAge(), factors);
    breakdown.bloodLossScore = CalculateBloodLossScore(profile.GetVitals().bloodLossPercentage, factors);
    breakdown.consciousnessScore = CalculateConsciousnessScore(profile.GetVitals().isConscious, factors);
    breakdown.timeFactorScore = CalculateTimeFactorScore(profile.GetVitals().estimatedTimeSinceIncidentMinutes, factors);
    breakdown.painScore = CalculatePainScore(profile.GetVitals().painLevel, factors);

    double sum = breakdown.diseaseScore +
                 breakdown.vitalsScore +
                 breakdown.ageScore +
                 breakdown.bloodLossScore +
                 breakdown.consciousnessScore +
                 breakdown.timeFactorScore +
                 breakdown.painScore;

    breakdown.totalScore = std::min(100.0, std::max(0.0, sum));

    // Priority Category
    if (breakdown.totalScore >= THRESHOLD_CRITICAL) {
        breakdown.priority = PriorityLevel::CRITICAL;
    } else if (breakdown.totalScore >= THRESHOLD_HIGH) {
        breakdown.priority = PriorityLevel::HIGH;
    } else if (breakdown.totalScore >= THRESHOLD_MEDIUM) {
        breakdown.priority = PriorityLevel::MEDIUM;
    } else {
        breakdown.priority = PriorityLevel::LOW;
    }

    // Estimated Survival Risk
    if (breakdown.totalScore >= 85.0) {
        breakdown.survivalRiskPercentage = 90.0;
        breakdown.estimatedSurvivalRisk = "Extreme / Imminent Mortality Risk (85-95%)";
    } else if (breakdown.totalScore >= THRESHOLD_CRITICAL) {
        breakdown.survivalRiskPercentage = 70.0;
        breakdown.estimatedSurvivalRisk = "High Critical Mortality Risk (60-80%)";
    } else if (breakdown.totalScore >= THRESHOLD_HIGH) {
        breakdown.survivalRiskPercentage = 40.0;
        breakdown.estimatedSurvivalRisk = "Moderate-to-High Morbidity Risk (30-50%)";
    } else if (breakdown.totalScore >= THRESHOLD_MEDIUM) {
        breakdown.survivalRiskPercentage = 18.0;
        breakdown.estimatedSurvivalRisk = "Moderate Decompensation Risk (10-25%)";
    } else {
        breakdown.survivalRiskPercentage = 3.0;
        breakdown.estimatedSurvivalRisk = "Low Clinical Survival Risk (< 5%)";
    }

    // Recommended Max ETA
    if (breakdown.priority == PriorityLevel::CRITICAL) {
        if (profile.GetEmergencyType() == EmergencyType::CARDIAC_ARREST) {
            breakdown.recommendedMaxETAMinutes = 6;
        } else if (profile.GetEmergencyType() == EmergencyType::ORGAN_TRANSPORT) {
            breakdown.recommendedMaxETAMinutes = 10;
        } else {
            breakdown.recommendedMaxETAMinutes = 8;
        }
    } else if (breakdown.priority == PriorityLevel::HIGH) {
        breakdown.recommendedMaxETAMinutes = 15;
    } else if (breakdown.priority == PriorityLevel::MEDIUM) {
        breakdown.recommendedMaxETAMinutes = 30;
    } else {
        breakdown.recommendedMaxETAMinutes = 60;
    }

    return breakdown;
}

double PriorityEngine::CalculateDiseaseScore(EmergencyType type, std::vector<std::string>& /*reasons*/) {
    switch (type) {
        case EmergencyType::CARDIAC_ARREST:       return 35.0;
        case EmergencyType::RESPIRATORY_FAILURE:  return 33.0;
        case EmergencyType::ORGAN_TRANSPORT:      return 32.0;
        case EmergencyType::HEART_ATTACK:        return 30.0;
        case EmergencyType::STROKE:              return 28.0;
        case EmergencyType::PEDIATRIC_EMERGENCY: return 28.0;
        case EmergencyType::INTERNAL_BLEEDING:    return 27.0;
        case EmergencyType::MAJOR_ACCIDENT:      return 26.0;
        case EmergencyType::TRAUMA:               return 25.0;
        case EmergencyType::ROAD_ACCIDENT:        return 24.0;
        case EmergencyType::BURN_INJURY:          return 22.0;
        case EmergencyType::POISONING:            return 22.0;
        case EmergencyType::PREGNANCY:            return 20.0;
        default:                                 return 10.0;
    }
}

double PriorityEngine::CalculateVitalsScore(const PatientVitals& vitals, std::vector<std::string>& reasons) {
    double score = 0.0;

    // SpO2 Assessment
    if (vitals.spO2 < 85) {
        score += 12.0;
        reasons.push_back("Severe Hypoxemia (SpO2 " + std::to_string(vitals.spO2) + "%)");
    } else if (vitals.spO2 < 90) {
        score += 9.0;
        reasons.push_back("Moderate Hypoxemia (SpO2 " + std::to_string(vitals.spO2) + "%)");
    } else if (vitals.spO2 < 94) {
        score += 5.0;
        reasons.push_back("Mild Hypoxemia (SpO2 " + std::to_string(vitals.spO2) + "%)");
    }

    // Heart Rate Assessment
    if (vitals.heartRate < 40) {
        score += 10.0;
        reasons.push_back("Critical Bradycardia (HR " + std::to_string(vitals.heartRate) + " bpm)");
    } else if (vitals.heartRate > 150) {
        score += 10.0;
        reasons.push_back("Severe Tachycardia (HR " + std::to_string(vitals.heartRate) + " bpm)");
    } else if (vitals.heartRate < 50) {
        score += 7.0;
        reasons.push_back("Marked Bradycardia (HR " + std::to_string(vitals.heartRate) + " bpm)");
    } else if (vitals.heartRate > 120) {
        score += 7.0;
        reasons.push_back("Marked Tachycardia (HR " + std::to_string(vitals.heartRate) + " bpm)");
    } else if (vitals.heartRate < 60) {
        score += 3.0;
        reasons.push_back("Mild Bradycardia (HR " + std::to_string(vitals.heartRate) + " bpm)");
    } else if (vitals.heartRate > 100) {
        score += 3.0;
        reasons.push_back("Mild Tachycardia (HR " + std::to_string(vitals.heartRate) + " bpm)");
    }

    // Blood Pressure Assessment
    if (vitals.bloodPressure.systolic < 80 || vitals.bloodPressure.diastolic < 50) {
        score += 8.0;
        reasons.push_back("Decompensated Hypotension / Shock (" + vitals.bloodPressure.ToString() + ")");
    } else if (vitals.bloodPressure.systolic >= 180 || vitals.bloodPressure.diastolic >= 120) {
        score += 7.0;
        reasons.push_back("Hypertensive Crisis (" + vitals.bloodPressure.ToString() + ")");
    } else if (vitals.bloodPressure.systolic < 90) {
        score += 4.0;
        reasons.push_back("Pre-shock Hypotension (" + vitals.bloodPressure.ToString() + ")");
    } else if (vitals.bloodPressure.systolic >= 160) {
        score += 4.0;
        reasons.push_back("Stage 2 Hypertension (" + vitals.bloodPressure.ToString() + ")");
    }

    // Respiration Rate Assessment
    if (vitals.respirationRate < 8 || vitals.respirationRate > 30) {
        score += 5.0;
        reasons.push_back("Severe Respiratory Distress (RR " + std::to_string(vitals.respirationRate) + " bpm)");
    } else if (vitals.respirationRate < 12 || vitals.respirationRate > 24) {
        score += 3.0;
        reasons.push_back("Abnormal Respiration (RR " + std::to_string(vitals.respirationRate) + " bpm)");
    }

    return score;
}

double PriorityEngine::CalculateAgeScore(uint16_t age, std::vector<std::string>& reasons) {
    if (age >= 75) {
        reasons.push_back("Geriatric High Risk (Age " + std::to_string(age) + ")");
        return 10.0;
    }
    if (age >= 65) {
        reasons.push_back("Elderly Patient Risk (Age " + std::to_string(age) + ")");
        return 6.0;
    }
    if (age <= 5 && age > 0) {
        reasons.push_back("Pediatric / Infant High Vulnerability (Age " + std::to_string(age) + ")");
        return 8.0;
    }
    if (age <= 12 && age > 5) {
        reasons.push_back("Pediatric Vulnerability (Age " + std::to_string(age) + ")");
        return 4.0;
    }
    return 0.0;
}

double PriorityEngine::CalculateBloodLossScore(double bloodLossPercent, std::vector<std::string>& reasons) {
    if (bloodLossPercent >= 40.0) {
        std::ostringstream oss;
        oss << "Class IV Hemorrhagic Shock (" << std::fixed << std::setprecision(1) << bloodLossPercent << "% blood loss)";
        reasons.push_back(oss.str());
        return 25.0;
    }
    if (bloodLossPercent >= 30.0) {
        std::ostringstream oss;
        oss << "Class III Hemorrhagic Shock (" << std::fixed << std::setprecision(1) << bloodLossPercent << "% blood loss)";
        reasons.push_back(oss.str());
        return 18.0;
    }
    if (bloodLossPercent >= 15.0) {
        std::ostringstream oss;
        oss << "Class II Hemorrhagic Shock (" << std::fixed << std::setprecision(1) << bloodLossPercent << "% blood loss)";
        reasons.push_back(oss.str());
        return 12.0;
    }
    if (bloodLossPercent > 0.0) {
        std::ostringstream oss;
        oss << "Class I Hemorrhage (" << std::fixed << std::setprecision(1) << bloodLossPercent << "% blood loss)";
        reasons.push_back(oss.str());
        return 5.0;
    }
    return 0.0;
}

double PriorityEngine::CalculateConsciousnessScore(bool isConscious, std::vector<std::string>& reasons) {
    if (!isConscious) {
        reasons.push_back("Unconscious / Altered Mental Status");
        return 20.0;
    }
    return 0.0;
}

double PriorityEngine::CalculateTimeFactorScore(uint32_t minutesElapsed, std::vector<std::string>& reasons) {
    if (minutesElapsed >= 60) {
        reasons.push_back("Prolonged Pre-hospital Transit Delay (" + std::to_string(minutesElapsed) + " mins)");
        return 8.0;
    }
    if (minutesElapsed >= 30) {
        reasons.push_back("Pre-hospital Transit Delay (" + std::to_string(minutesElapsed) + " mins)");
        return 4.0;
    }
    return 0.0;
}

double PriorityEngine::CalculatePainScore(uint8_t painLevel, std::vector<std::string>& reasons) {
    if (painLevel >= 8) {
        reasons.push_back("Severe Intractable Pain (Rating " + std::to_string(painLevel) + "/10)");
        return 3.0;
    }
    if (painLevel >= 5) {
        reasons.push_back("Moderate Pain (Rating " + std::to_string(painLevel) + "/10)");
        return 1.5;
    }
    return 0.0;
}

} // namespace medical
} // namespace aegis
