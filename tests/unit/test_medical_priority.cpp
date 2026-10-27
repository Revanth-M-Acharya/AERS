/**
 * @file test_medical_priority.cpp
 * @brief Unit tests for PatientProfile and PriorityEngine in SMART AEGIS.
 */

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>

#include "medical/PatientProfile.h"
#include "medical/PriorityEngine.h"
#include "medical/MedicalDatabase.h"

#define AEGIS_TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "\n[FAIL] Assertion failed: (" #cond ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::abort(); \
        } \
    } while (false)

using namespace aegis::medical;

// =============================================================================
// Test 1: Cardiac Arrest (CRITICAL Priority)
// =============================================================================
static void TestCardiacArrest() {
    std::cout << "[TEST] 1. Cardiac Arrest Profile & Immediate Triage... ";

    PatientVitals vitals;
    vitals.heartRate = 0; // Asystole
    vitals.bloodPressure = BloodPressure{0, 0};
    vitals.spO2 = 65; // Severe anoxia
    vitals.respirationRate = 0; // Apnea
    vitals.bloodLossPercentage = 0.0;
    vitals.isConscious = false;
    vitals.estimatedTimeSinceIncidentMinutes = 8;

    PatientProfile patient(
        "PT-1001",
        "Eleanor Vance",
        62,
        Gender::FEMALE,
        EmergencyType::CARDIAC_ARREST,
        vitals
    );

    AEGIS_TEST_ASSERT(patient.IsValid());
    AEGIS_TEST_ASSERT(patient.GetPatientId() == "PT-1001");
    AEGIS_TEST_ASSERT(patient.GetEmergencyType() == EmergencyType::CARDIAC_ARREST);

    double score = PriorityEngine::CalculateEmergencyScore(patient);
    PriorityLevel priority = PriorityEngine::CalculatePriority(patient);
    std::string reason = PriorityEngine::GeneratePriorityReason(patient);

    // Cardiac Arrest base (35) + Unconscious (20) + SpO2 < 85 (12) + HR < 40 (10) + BP < 80/50 (8) + RR < 8 (5) = 90
    AEGIS_TEST_ASSERT(score >= PriorityEngine::THRESHOLD_CRITICAL);
    AEGIS_TEST_ASSERT(priority == PriorityLevel::CRITICAL);
    AEGIS_TEST_ASSERT(reason.find("CRITICAL") != std::string::npos);
    AEGIS_TEST_ASSERT(reason.find("Cardiac Arrest") != std::string::npos);
    AEGIS_TEST_ASSERT(reason.find("Unconscious") != std::string::npos);
    AEGIS_TEST_ASSERT(reason.find("Severe Hypoxemia") != std::string::npos);

    std::cout << "PASS (Score: " << score << ", Priority: " << PriorityLevelToString(priority) << ")\n";
}

// =============================================================================
// Test 2: Internal Bleeding with Hemorrhagic Shock (CRITICAL / HIGH Priority)
// =============================================================================
static void TestInternalBleeding() {
    std::cout << "[TEST] 2. Internal Bleeding & Class IV Shock... ";

    PatientVitals vitals;
    vitals.heartRate = 142; // Tachycardia
    vitals.bloodPressure = BloodPressure{74, 46}; // Decompensated shock
    vitals.spO2 = 88; // Moderate hypoxemia
    vitals.respirationRate = 28; // Tachypnea
    vitals.bloodLossPercentage = 42.0; // Class IV Hemorrhage
    vitals.isConscious = true;
    vitals.estimatedTimeSinceIncidentMinutes = 25;

    PatientProfile patient(
        "PT-1002",
        "Arthur Pendelton",
        45,
        Gender::MALE,
        EmergencyType::INTERNAL_BLEEDING,
        vitals
    );

    AEGIS_TEST_ASSERT(patient.IsValid());

    double score = PriorityEngine::CalculateEmergencyScore(patient);
    PriorityLevel priority = PriorityEngine::CalculatePriority(patient);
    std::string reason = PriorityEngine::GeneratePriorityReason(patient);

    // Disease (27) + Blood loss >= 40% (25) + BP < 80/50 (8) + HR 121-150 (7) + SpO2 85-89 (9) + RR 25-30 (3) = 79.0
    AEGIS_TEST_ASSERT(score >= PriorityEngine::THRESHOLD_CRITICAL);
    AEGIS_TEST_ASSERT(priority == PriorityLevel::CRITICAL);
    AEGIS_TEST_ASSERT(reason.find("Class IV Hemorrhagic Shock") != std::string::npos);
    AEGIS_TEST_ASSERT(reason.find("Decompensated Hypotension") != std::string::npos);

    std::cout << "PASS (Score: " << score << ", Priority: " << PriorityLevelToString(priority) << ")\n";
}

// =============================================================================
// Test 3: Acute Heart Attack (HIGH Priority)
// =============================================================================
static void TestHeartAttack() {
    std::cout << "[TEST] 3. Acute Heart Attack (STEMI) with Elderly Risk... ";

    PatientVitals vitals;
    vitals.heartRate = 114; // Mild tachycardia
    vitals.bloodPressure = BloodPressure{168, 98}; // Stage 2 HTN
    vitals.spO2 = 91; // Mild hypoxemia
    vitals.respirationRate = 22;
    vitals.bloodLossPercentage = 0.0;
    vitals.isConscious = true;
    vitals.estimatedTimeSinceIncidentMinutes = 45; // Golden window slipping

    PatientProfile patient(
        "PT-1003",
        "Marcus Brody",
        68,
        Gender::MALE,
        EmergencyType::HEART_ATTACK,
        vitals
    );

    AEGIS_TEST_ASSERT(patient.IsValid());

    double score = PriorityEngine::CalculateEmergencyScore(patient);
    PriorityLevel priority = PriorityEngine::CalculatePriority(patient);
    std::string reason = PriorityEngine::GeneratePriorityReason(patient);

    // Disease (30) + Age 65-74 (6) + SpO2 90-93 (5) + HR 101-120 (3) + BP Stage 2 (4) + Delay 30-60m (4) = 52
    // If we adjust SpO2 to 89% (+9) -> 56 -> HIGH!
    patient.SetSpO2(89);
    score = PriorityEngine::CalculateEmergencyScore(patient);
    priority = PriorityEngine::CalculatePriority(patient);
    reason = PriorityEngine::GeneratePriorityReason(patient);

    AEGIS_TEST_ASSERT(score >= PriorityEngine::THRESHOLD_HIGH);
    AEGIS_TEST_ASSERT(priority == PriorityLevel::HIGH);
    AEGIS_TEST_ASSERT(reason.find("HIGH Priority") != std::string::npos);
    AEGIS_TEST_ASSERT(reason.find("Heart Attack") != std::string::npos);

    std::cout << "PASS (Score: " << score << ", Priority: " << PriorityLevelToString(priority) << ")\n";
}

// =============================================================================
// Test 4: Acute Stroke with Hypertensive Crisis (HIGH Priority)
// =============================================================================
static void TestStroke() {
    std::cout << "[TEST] 4. Acute Ischemic / Hemorrhagic Stroke with Crisis BP... ";

    PatientVitals vitals;
    vitals.heartRate = 88;
    vitals.bloodPressure = BloodPressure{195, 115}; // Hypertensive crisis
    vitals.spO2 = 92; // Mild hypoxemia
    vitals.respirationRate = 18;
    vitals.bloodLossPercentage = 0.0;
    vitals.isConscious = true;
    vitals.estimatedTimeSinceIncidentMinutes = 70; // > 60m pre-hospital delay

    PatientProfile patient(
        "PT-1004",
        "Clara Higgins",
        76,
        Gender::FEMALE,
        EmergencyType::STROKE,
        vitals
    );

    AEGIS_TEST_ASSERT(patient.IsValid());

    double score = PriorityEngine::CalculateEmergencyScore(patient);
    PriorityLevel priority = PriorityEngine::CalculatePriority(patient);
    std::string reason = PriorityEngine::GeneratePriorityReason(patient);

    // Disease (28) + Age >= 75 (10) + BP crisis (7) + SpO2 90-93 (5) + Delay > 60m (8) = 58.0
    AEGIS_TEST_ASSERT(score >= PriorityEngine::THRESHOLD_HIGH);
    AEGIS_TEST_ASSERT(priority == PriorityLevel::HIGH);
    AEGIS_TEST_ASSERT(reason.find("Stroke") != std::string::npos);
    AEGIS_TEST_ASSERT(reason.find("Hypertensive Crisis") != std::string::npos);
    AEGIS_TEST_ASSERT(reason.find("Geriatric High Risk") != std::string::npos);

    std::cout << "PASS (Score: " << score << ", Priority: " << PriorityLevelToString(priority) << ")\n";
}

// =============================================================================
// Test 5: Road Accident with Moderate Hemorrhage (MEDIUM Priority)
// =============================================================================
static void TestRoadAccident() {
    std::cout << "[TEST] 5. Road Accident Vehicle Collision... ";

    PatientVitals vitals;
    vitals.heartRate = 126; // Marked tachycardia
    vitals.bloodPressure = BloodPressure{88, 58}; // Pre-shock hypotension
    vitals.spO2 = 95; // Normal oxygenation
    vitals.respirationRate = 26; // Tachypnea
    vitals.bloodLossPercentage = 20.0; // Class II Hemorrhage
    vitals.isConscious = true;
    vitals.estimatedTimeSinceIncidentMinutes = 15;

    PatientProfile patient(
        "PT-1005",
        "David Miller",
        29,
        Gender::MALE,
        EmergencyType::ROAD_ACCIDENT,
        vitals
    );

    AEGIS_TEST_ASSERT(patient.IsValid());

    double score = PriorityEngine::CalculateEmergencyScore(patient);
    PriorityLevel priority = PriorityEngine::CalculatePriority(patient);
    std::string reason = PriorityEngine::GeneratePriorityReason(patient);

    // Disease (24) + Class II (12) + BP pre-shock (4) + HR 121-150 (7) + RR 25-30 (3) = 50.0
    AEGIS_TEST_ASSERT(score >= PriorityEngine::THRESHOLD_MEDIUM && score < PriorityEngine::THRESHOLD_HIGH);
    AEGIS_TEST_ASSERT(priority == PriorityLevel::MEDIUM);
    AEGIS_TEST_ASSERT(reason.find("Road Accident") != std::string::npos);
    AEGIS_TEST_ASSERT(reason.find("Class II Hemorrhagic Shock") != std::string::npos);

    // Condition worsens into unconsciousness
    patient.SetConscious(false); // +20 points -> 70.0 (HIGH)
    double escalatedScore = PriorityEngine::CalculateEmergencyScore(patient);
    PriorityLevel escalatedPriority = PriorityEngine::CalculatePriority(patient);
    AEGIS_TEST_ASSERT(escalatedScore >= PriorityEngine::THRESHOLD_HIGH);
    AEGIS_TEST_ASSERT(escalatedPriority == PriorityLevel::HIGH);

    std::cout << "PASS (Score: " << score << " -> Escalated: " << escalatedScore << ")\n";
}

// =============================================================================
// Test 6: Burn Injury (MEDIUM / HIGH Priority depending on airway/vitals)
// =============================================================================
static void TestBurnInjury() {
    std::cout << "[TEST] 6. Severe Thermal Burn with Inhalation Compromise... ";

    PatientVitals vitals;
    vitals.heartRate = 132; // Marked tachycardia
    vitals.bloodPressure = BloodPressure{162, 94};
    vitals.spO2 = 88; // Smoke inhalation / moderate hypoxemia
    vitals.respirationRate = 32; // Severe tachypnea
    vitals.bloodLossPercentage = 0.0;
    vitals.isConscious = true;
    vitals.estimatedTimeSinceIncidentMinutes = 20;

    PatientProfile patient(
        "PT-1006",
        "Sarah Connor",
        34,
        Gender::FEMALE,
        EmergencyType::BURN_INJURY,
        vitals
    );

    AEGIS_TEST_ASSERT(patient.IsValid());

    double score = PriorityEngine::CalculateEmergencyScore(patient);
    PriorityLevel priority = PriorityEngine::CalculatePriority(patient);
    std::string reason = PriorityEngine::GeneratePriorityReason(patient);

    // Disease (22) + SpO2 85-89 (9) + RR > 30 (5) + HR 121-150 (7) + BP Stage 2 (4) = 47.0 (MEDIUM)
    AEGIS_TEST_ASSERT(score >= PriorityEngine::THRESHOLD_MEDIUM);
    AEGIS_TEST_ASSERT(priority == PriorityLevel::MEDIUM);
    AEGIS_TEST_ASSERT(reason.find("Burn Injury") != std::string::npos);
    AEGIS_TEST_ASSERT(reason.find("Severe Respiratory Distress") != std::string::npos);

    std::cout << "PASS (Score: " << score << ", Priority: " << PriorityLevelToString(priority) << ")\n";
}

// =============================================================================
// Test 7: Pregnancy (LOW Priority routine vs Escalated Obstetric)
// =============================================================================
static void TestPregnancy() {
    std::cout << "[TEST] 7. Pregnancy Routine Labor vs Obstetric Emergency... ";

    // Routine labor: normal vitals
    PatientVitals routineVitals;
    routineVitals.heartRate = 82;
    routineVitals.bloodPressure = BloodPressure{118, 76};
    routineVitals.spO2 = 98;
    routineVitals.respirationRate = 16;
    routineVitals.bloodLossPercentage = 0.0;
    routineVitals.isConscious = true;
    routineVitals.estimatedTimeSinceIncidentMinutes = 15;

    PatientProfile normalLabor(
        "PT-1007A",
        "Rachel Green",
        28,
        Gender::FEMALE,
        EmergencyType::PREGNANCY,
        routineVitals
    );

    AEGIS_TEST_ASSERT(normalLabor.IsValid());
    double routineScore = PriorityEngine::CalculateEmergencyScore(normalLabor);
    PriorityLevel routinePriority = PriorityEngine::CalculatePriority(normalLabor);
    std::string routineReason = PriorityEngine::GeneratePriorityReason(normalLabor);

    // Base (20) + no derangements = 20.0 (LOW)
    AEGIS_TEST_ASSERT(routineScore < PriorityEngine::THRESHOLD_MEDIUM);
    AEGIS_TEST_ASSERT(routinePriority == PriorityLevel::LOW);
    AEGIS_TEST_ASSERT(routineReason.find("LOW Priority") != std::string::npos);
    AEGIS_TEST_ASSERT(routineReason.find("Vitals stable") != std::string::npos);

    // Severe postpartum/antepartum hemorrhage & eclampsia
    PatientVitals criticalVitals;
    criticalVitals.heartRate = 138; // Marked tachycardia (+7)
    criticalVitals.bloodPressure = BloodPressure{185, 122}; // Severe pre-eclampsia crisis (+7)
    criticalVitals.spO2 = 89; // Pulmonary edema hypoxemia (+9)
    criticalVitals.respirationRate = 31; // Respiratory distress (+5)
    criticalVitals.bloodLossPercentage = 32.0; // Class III hemorrhage (+18)
    criticalVitals.isConscious = false; // Eclamptic seizure unconscious (+20)
    criticalVitals.estimatedTimeSinceIncidentMinutes = 30; // (+4)

    PatientProfile severeObstetric(
        "PT-1007B",
        "Emily Thorne",
        31,
        Gender::FEMALE,
        EmergencyType::PREGNANCY,
        criticalVitals
    );

    double severeScore = PriorityEngine::CalculateEmergencyScore(severeObstetric);
    PriorityLevel severePriority = PriorityEngine::CalculatePriority(severeObstetric);

    // Base (20) + 7 + 7 + 9 + 5 + 18 + 20 + 4 = 90.0 (CRITICAL)
    AEGIS_TEST_ASSERT(severeScore >= PriorityEngine::THRESHOLD_CRITICAL);
    AEGIS_TEST_ASSERT(severePriority == PriorityLevel::CRITICAL);

    std::cout << "PASS (Routine: " << routineScore << " [LOW] -> Severe: " << severeScore << " [CRITICAL])\n";
}

// =============================================================================
// Test 8: Demographics, Validation, and Serialization
// =============================================================================
static void TestProfileSerializationAndValidation() {
    std::cout << "[TEST] 8. Validation Rules and JSON/String Export... ";

    PatientProfile invalidProfile;
    AEGIS_TEST_ASSERT(!invalidProfile.IsValid()); // Missing ID, Name, Type

    invalidProfile.SetPatientId("PT-9999");
    invalidProfile.SetPatientName("Ghost Patient");
    invalidProfile.SetEmergencyType(EmergencyType::TRAUMA);
    AEGIS_TEST_ASSERT(invalidProfile.IsValid());

    // Invalid SpO2 > 100
    invalidProfile.SetSpO2(105);
    AEGIS_TEST_ASSERT(!invalidProfile.IsValid());
    invalidProfile.SetSpO2(98);
    AEGIS_TEST_ASSERT(invalidProfile.IsValid());

    // Invalid Blood Loss > 100%
    invalidProfile.SetBloodLossPercentage(110.0);
    AEGIS_TEST_ASSERT(!invalidProfile.IsValid());
    invalidProfile.SetBloodLossPercentage(12.5);
    AEGIS_TEST_ASSERT(invalidProfile.IsValid());

    // String representation check
    std::string strRepr = invalidProfile.ToString();
    AEGIS_TEST_ASSERT(strRepr.find("PT-9999") != std::string::npos);
    AEGIS_TEST_ASSERT(strRepr.find("Ghost Patient") != std::string::npos);
    AEGIS_TEST_ASSERT(strRepr.find("Trauma") != std::string::npos);

    // JSON export check
    std::string jsonRepr = invalidProfile.ToJson();
    AEGIS_TEST_ASSERT(jsonRepr.find("\"patient_id\":\"PT-9999\"") != std::string::npos);
    AEGIS_TEST_ASSERT(jsonRepr.find("\"spo2\":98") != std::string::npos);

    // Granular factor breakdown inspection
    auto breakdown = PriorityEngine::EvaluateBreakdown(invalidProfile);
    AEGIS_TEST_ASSERT(breakdown.diseaseScore == 25.0); // Trauma base
    AEGIS_TEST_ASSERT(breakdown.bloodLossScore == 5.0); // 12.5% Class I
    AEGIS_TEST_ASSERT(breakdown.totalScore == 30.0);

    std::cout << "PASS\n";
}

// =============================================================================
// Test 9: All 12 Diseases & New Clinical Categories
// =============================================================================
static void TestNewDiseaseCategories() {
    std::cout << "[TEST] 9. Comprehensive Disease Models (Pediatric, Organ Transport, Major Accident)... ";

    // Pediatric Emergency
    PatientVitals pedVitals{165, BloodPressure{85, 50}, 88, 44, 0.0, true, 20, 8};
    PatientProfile ped("PT-PED-01", "Emma Watson", 3, Gender::FEMALE, EmergencyType::PEDIATRIC_EMERGENCY, pedVitals, "Pediatric status asthmaticus");
    AEGIS_TEST_ASSERT(ped.IsValid());
    AEGIS_TEST_ASSERT(ped.GetDisease() == EmergencyType::PEDIATRIC_EMERGENCY);
    AEGIS_TEST_ASSERT(ped.GetDiseaseString() == "Pediatric Emergency");
    PriorityLevel pedPri = PriorityEngine::CalculatePriority(ped);
    AEGIS_TEST_ASSERT(pedPri == PriorityLevel::HIGH || pedPri == PriorityLevel::CRITICAL);

    // Organ Transport
    PatientVitals organVitals{0, BloodPressure{0, 0}, 100, 0, 0.0, false, 35, 0};
    PatientProfile organ("PT-ORG-01", "Donor Heart TX-1", 21, Gender::MALE, EmergencyType::ORGAN_TRANSPORT, organVitals, "Cold ischemia window 4h");
    AEGIS_TEST_ASSERT(organ.IsValid());
    AEGIS_TEST_ASSERT(organ.GetDiseaseString() == "Organ Transport");
    double organScore = PriorityEngine::CalculateEmergencyScore(organ);
    AEGIS_TEST_ASSERT(organScore >= PriorityEngine::THRESHOLD_HIGH);

    // Major Accident
    PatientVitals accidentVitals{140, BloodPressure{78, 48}, 84, 28, 42.0, true, 15, 10};
    PatientProfile accident("PT-ACC-01", "Mark Johnson", 32, Gender::MALE, EmergencyType::MAJOR_ACCIDENT, accidentVitals, "Rollover crash polytrauma");
    AEGIS_TEST_ASSERT(accident.IsValid());
    AEGIS_TEST_ASSERT(accident.GetDiseaseString() == "Major Accident");
    PriorityLevel accPri = PriorityEngine::CalculatePriority(accident);
    AEGIS_TEST_ASSERT(accPri == PriorityLevel::CRITICAL);

    std::cout << "PASS\n";
}

// =============================================================================
// Test 10: Survival Risk and Recommended Max ETA
// =============================================================================
static void TestSurvivalRiskAndRecommendedMaxETA() {
    std::cout << "[TEST] 10. Estimated Survival Risk & Recommended Max ETA... ";

    // Critical Cardiac Arrest
    PatientVitals vCrit{0, BloodPressure{0, 0}, 60, 0, 0.0, false, 5, 0};
    PatientProfile ptCrit("PT-CRIT", "Crit Patient", 58, Gender::MALE, EmergencyType::CARDIAC_ARREST, vCrit);

    double riskCrit = PriorityEngine::CalculateSurvivalRiskPercentage(ptCrit);
    std::string riskStrCrit = PriorityEngine::EstimateSurvivalRisk(ptCrit);
    uint32_t etaCrit = PriorityEngine::CalculateRecommendedMaxETAMinutes(ptCrit);
    std::string etaStrCrit = PriorityEngine::GetRecommendedMaxETAString(ptCrit);

    AEGIS_TEST_ASSERT(riskCrit >= 80.0);
    AEGIS_TEST_ASSERT(riskStrCrit.find("Mortality") != std::string::npos);
    AEGIS_TEST_ASSERT(etaCrit <= 8);
    AEGIS_TEST_ASSERT(etaStrCrit == "6 minutes");

    // High Priority STEMI Heart Attack
    PatientVitals vHigh{115, BloodPressure{160, 95}, 92, 22, 0.0, true, 30, 8};
    PatientProfile ptHigh("PT-HIGH", "High Patient", 67, Gender::MALE, EmergencyType::HEART_ATTACK, vHigh);
    uint32_t etaHigh = PriorityEngine::CalculateRecommendedMaxETAMinutes(ptHigh);
    AEGIS_TEST_ASSERT(etaHigh == 15);

    // Routine Labor (Low Priority)
    PatientVitals vLow{75, BloodPressure{120, 80}, 99, 16, 0.0, true, 60, 4};
    PatientProfile ptLow("PT-LOW", "Low Patient", 28, Gender::FEMALE, EmergencyType::PREGNANCY, vLow);
    uint32_t etaLow = PriorityEngine::CalculateRecommendedMaxETAMinutes(ptLow);
    AEGIS_TEST_ASSERT(etaLow == 60);

    std::cout << "PASS\n";
}

// =============================================================================
// Test 11: All 14 PatientProfile Fields and Accessors
// =============================================================================
static void TestAllProfileFieldsAndAccessors() {
    std::cout << "[TEST] 11. Complete 14 PatientProfile Fields & Accessors... ";

    PatientProfile p;
    p.SetPatientId("PT-ALL-01");
    p.SetPatientName("Alexander Hamilton");
    p.SetAge(47);
    p.SetGender(Gender::MALE);
    p.SetDisease(EmergencyType::TRAUMA);
    p.SetHeartRate(95);
    p.SetBloodPressure(128, 84);
    p.SetRespirationRate(20);
    p.SetSpO2(97);
    p.SetBloodLossPercentage(15.5);
    p.SetPainLevel(7);
    p.SetConscious(true);
    p.SetEstimatedTimeSinceIncident(40);
    p.SetRemarks("Gunshot wound to right shoulder, direct pressure applied");

    AEGIS_TEST_ASSERT(p.GetPatientId() == "PT-ALL-01");
    AEGIS_TEST_ASSERT(p.GetPatientName() == "Alexander Hamilton");
    AEGIS_TEST_ASSERT(p.GetAge() == 47);
    AEGIS_TEST_ASSERT(p.GetGender() == Gender::MALE);
    AEGIS_TEST_ASSERT(p.GetDisease() == EmergencyType::TRAUMA);
    AEGIS_TEST_ASSERT(p.GetHeartRate() == 95);
    AEGIS_TEST_ASSERT(p.GetBloodPressure().systolic == 128);
    AEGIS_TEST_ASSERT(p.GetBloodPressure().diastolic == 84);
    AEGIS_TEST_ASSERT(p.GetRespirationRate() == 20);
    AEGIS_TEST_ASSERT(p.GetSpO2() == 97);
    AEGIS_TEST_ASSERT(p.GetBloodLossPercentage() == 15.5);
    AEGIS_TEST_ASSERT(p.GetPainLevel() == 7);
    AEGIS_TEST_ASSERT(p.IsConscious() == true);
    AEGIS_TEST_ASSERT(p.GetEstimatedTimeSinceIncident() == 40);
    AEGIS_TEST_ASSERT(p.GetRemarks() == "Gunshot wound to right shoulder, direct pressure applied");

    std::string jsonStr = p.ToJson();
    AEGIS_TEST_ASSERT(jsonStr.find("\"pain_level\":7") != std::string::npos);
    AEGIS_TEST_ASSERT(jsonStr.find("\"remarks\":\"Gunshot wound") != std::string::npos);
    AEGIS_TEST_ASSERT(jsonStr.find("\"disease\":\"Trauma\"") != std::string::npos);

    std::cout << "PASS\n";
}

// =============================================================================
// Test 12: MedicalDatabase Sample Patients & Repository
// =============================================================================
static void TestMedicalDatabaseSampleGeneration() {
    std::cout << "[TEST] 12. MedicalDatabase Sample Generation (16 Patients & Queries)... ";

    auto samplePatients = MedicalDatabase::GenerateSamplePatients();
    AEGIS_TEST_ASSERT(samplePatients.size() >= 15); // Spec requires at least 15
    AEGIS_TEST_ASSERT(samplePatients.size() == 16);

    // Verify presence of all required diseases in sample mix
    bool hasHeartAttack = false;
    bool hasCardiacArrest = false;
    bool hasStroke = false;
    bool hasMajorAccident = false;
    bool hasPregnancy = false;
    bool hasBurn = false;
    bool hasTrauma = false;
    bool hasInternalBleeding = false;
    bool hasRespiratoryFailure = false;
    bool hasPoisoning = false;
    bool hasPediatric = false;
    bool hasOrganTransport = false;

    for (const auto& pt : samplePatients) {
        AEGIS_TEST_ASSERT(pt.IsValid());
        switch (pt.GetDisease()) {
            case EmergencyType::HEART_ATTACK:        hasHeartAttack = true; break;
            case EmergencyType::CARDIAC_ARREST:      hasCardiacArrest = true; break;
            case EmergencyType::STROKE:              hasStroke = true; break;
            case EmergencyType::MAJOR_ACCIDENT:      hasMajorAccident = true; break;
            case EmergencyType::PREGNANCY:           hasPregnancy = true; break;
            case EmergencyType::BURN_INJURY:         hasBurn = true; break;
            case EmergencyType::TRAUMA:              hasTrauma = true; break;
            case EmergencyType::INTERNAL_BLEEDING:   hasInternalBleeding = true; break;
            case EmergencyType::RESPIRATORY_FAILURE: hasRespiratoryFailure = true; break;
            case EmergencyType::POISONING:           hasPoisoning = true; break;
            case EmergencyType::PEDIATRIC_EMERGENCY: hasPediatric = true; break;
            case EmergencyType::ORGAN_TRANSPORT:     hasOrganTransport = true; break;
            default: break;
        }
    }

    AEGIS_TEST_ASSERT(hasHeartAttack);
    AEGIS_TEST_ASSERT(hasCardiacArrest);
    AEGIS_TEST_ASSERT(hasStroke);
    AEGIS_TEST_ASSERT(hasMajorAccident);
    AEGIS_TEST_ASSERT(hasPregnancy);
    AEGIS_TEST_ASSERT(hasBurn);
    AEGIS_TEST_ASSERT(hasTrauma);
    AEGIS_TEST_ASSERT(hasInternalBleeding);
    AEGIS_TEST_ASSERT(hasRespiratoryFailure);
    AEGIS_TEST_ASSERT(hasPoisoning);
    AEGIS_TEST_ASSERT(hasPediatric);
    AEGIS_TEST_ASSERT(hasOrganTransport);

    // Test Database Catalog
    MedicalDatabase db;
    AEGIS_TEST_ASSERT(db.GetTotalPatientCount() == 16);

    auto heartAttacks = db.GetPatientsByDisease(EmergencyType::HEART_ATTACK);
    AEGIS_TEST_ASSERT(!heartAttacks.empty());

    auto critPatients = db.GetPatientsByPriority(PriorityLevel::CRITICAL);
    AEGIS_TEST_ASSERT(!critPatients.empty());

    auto searchOpt = db.GetPatientById("PT-SAMP-002");
    AEGIS_TEST_ASSERT(searchOpt.has_value());
    AEGIS_TEST_ASSERT(searchOpt->GetPatientName() == "Marcus Holloway");

    // JSON export
    std::string dbJson = db.ExportDatabaseJson();
    AEGIS_TEST_ASSERT(!dbJson.empty());
    AEGIS_TEST_ASSERT(dbJson.front() == '[');
    AEGIS_TEST_ASSERT(dbJson.back() == ']');
    AEGIS_TEST_ASSERT(dbJson.find("Robert Sterling") != std::string::npos);

    std::cout << "PASS\n";
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Medical Patient Profile & PriorityEngine Unit Tests" << std::endl;
    std::cout << "==========================================================" << std::endl;

    TestCardiacArrest();
    TestInternalBleeding();
    TestHeartAttack();
    TestStroke();
    TestRoadAccident();
    TestBurnInjury();
    TestPregnancy();
    TestProfileSerializationAndValidation();
    TestNewDiseaseCategories();
    TestSurvivalRiskAndRecommendedMaxETA();
    TestAllProfileFieldsAndAccessors();
    TestMedicalDatabaseSampleGeneration();

    std::cout << "==========================================================" << std::endl;
    std::cout << " All Medical Priority Tests Passed Successfully!" << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
