/**
 * @file MedicalDatabase.cpp
 * @brief Implementation of sample patient generator and clinical database.
 */

#include "medical/MedicalDatabase.h"
#include <fstream>
#include <sstream>

namespace aegis {
namespace medical {

MedicalDatabase::MedicalDatabase() {
    ResetToDefaults();
}

std::vector<PatientProfile> MedicalDatabase::GenerateSamplePatients() {
    std::vector<PatientProfile> list;
    list.reserve(16);

    // 1. Acute STEMI Heart Attack
    {
        PatientVitals v{118, BloodPressure{168, 98}, 93, 22, 0.0, true, 35, 9};
        list.emplace_back("PT-SAMP-001", "Robert Sterling", 68, Gender::MALE,
                          EmergencyType::HEART_ATTACK, v,
                          "Acute anterior STEMI with severe crushing retrosternal pain radiating to left jaw");
    }

    // 2. Out-of-Hospital Cardiac Arrest
    {
        PatientVitals v{0, BloodPressure{0, 0}, 62, 0, 0.0, false, 7, 0};
        list.emplace_back("PT-SAMP-002", "Marcus Holloway", 54, Gender::MALE,
                          EmergencyType::CARDIAC_ARREST, v,
                          "Out-of-hospital cardiac arrest witnessed in train terminal, CPR in progress");
    }

    // 3. Acute Ischemic Stroke
    {
        PatientVitals v{92, BloodPressure{198, 112}, 95, 18, 0.0, true, 45, 4};
        list.emplace_back("PT-SAMP-003", "Catherine Bell", 73, Gender::FEMALE,
                          EmergencyType::STROKE, v,
                          "Acute right MCA ischemic stroke, dense left hemiplegia, onset 45m ago");
    }

    // 4. Multi-Vehicle Major Highway Accident
    {
        PatientVitals v{138, BloodPressure{82, 52}, 91, 28, 38.0, true, 20, 10};
        list.emplace_back("PT-SAMP-004", "David Zhang", 29, Gender::MALE,
                          EmergencyType::MAJOR_ACCIDENT, v,
                          "High-speed rollover MVC, flail chest, pelvic fracture, severe hemorrhagic shock");
    }

    // 5. Severe Pre-eclampsia / Obstetric Crisis
    {
        PatientVitals v{110, BloodPressure{178, 116}, 96, 20, 5.0, true, 15, 8};
        list.emplace_back("PT-SAMP-005", "Priya Sharma", 28, Gender::FEMALE,
                          EmergencyType::PREGNANCY, v,
                          "37 weeks pregnant, severe pre-eclampsia with visual disturbances and clonus");
    }

    // 6. Extensive Thermal Burn Injury
    {
        PatientVitals v{135, BloodPressure{98, 65}, 88, 26, 0.0, true, 30, 10};
        list.emplace_back("PT-SAMP-006", "Ethan Gallagher", 42, Gender::MALE,
                          EmergencyType::BURN_INJURY, v,
                          "Flash fire in chemical lab, 45% TBSA partial and full thickness burns with inhalation injury");
    }

    // 7. Internal Bleeding / Ruptured Spleen
    {
        PatientVitals v{144, BloodPressure{76, 44}, 89, 30, 44.0, false, 25, 8};
        list.emplace_back("PT-SAMP-007", "Amara Okafor", 24, Gender::FEMALE,
                          EmergencyType::INTERNAL_BLEEDING, v,
                          "Blunt splenic rupture and hemoperitoneum following scaffolding fall, decompensated shock");
    }

    // 8. Acute Severe Respiratory Failure
    {
        PatientVitals v{126, BloodPressure{155, 92}, 78, 34, 0.0, true, 50, 6};
        list.emplace_back("PT-SAMP-008", "Arthur Henderson", 69, Gender::MALE,
                          EmergencyType::RESPIRATORY_FAILURE, v,
                          "Severe COPD exacerbation, refractory hypoxemic respiratory failure");
    }

    // 9. Acute Cholinergic Toxin Poisoning
    {
        PatientVitals v{38, BloodPressure{84, 50}, 92, 10, 0.0, false, 18, 2};
        list.emplace_back("PT-SAMP-009", "Lucas Silva", 36, Gender::MALE,
                          EmergencyType::POISONING, v,
                          "Organophosphate insecticide toxicity, acute cholinergic crisis with severe bradycardia");
    }

    // 10. Pediatric Status Asthmaticus
    {
        PatientVitals v{172, BloodPressure{88, 54}, 87, 48, 0.0, true, 22, 7};
        list.emplace_back("PT-SAMP-010", "Sophia Miller", 4, Gender::FEMALE,
                          EmergencyType::PEDIATRIC_EMERGENCY, v,
                          "Pediatric status asthmaticus with severe intercostal retractions and lethargy");
    }

    // 11. Donor Heart Organ Transport
    {
        PatientVitals v{0, BloodPressure{0, 0}, 100, 0, 0.0, false, 40, 0};
        list.emplace_back("PT-SAMP-011", "Donor Heart TX-9", 22, Gender::MALE,
                          EmergencyType::ORGAN_TRANSPORT, v,
                          "Donor cardiac allograft in hypothermic machine perfusion, 4-hour cold ischemia limit");
    }

    // 12. Severe Orthopedic / Crushing Trauma
    {
        PatientVitals v{122, BloodPressure{105, 70}, 96, 22, 22.0, true, 15, 9};
        list.emplace_back("PT-SAMP-012", "Vikram Patel", 38, Gender::MALE,
                          EmergencyType::TRAUMA, v,
                          "Industrial machinery crushing injury to lower limb with open tibial compound fracture");
    }

    // 13. Road Accident / Collision
    {
        PatientVitals v{105, BloodPressure{122, 80}, 97, 18, 8.0, true, 35, 6};
        list.emplace_back("PT-SAMP-013", "Elena Rostova", 31, Gender::FEMALE,
                          EmergencyType::ROAD_ACCIDENT, v,
                          "Intersection T-bone collision, clavicle fracture, stable hemodynamics");
    }

    // 14. Geriatric NSTEMI with Acute Heart Failure
    {
        PatientVitals v{98, BloodPressure{138, 82}, 91, 22, 0.0, true, 60, 7};
        list.emplace_back("PT-SAMP-014", "George Washington", 81, Gender::MALE,
                          EmergencyType::HEART_ATTACK, v,
                          "Geriatric NSTEMI with acute heart failure decompensation, bibasilar rales");
    }

    // 15. Routine Active Obstetric Labor
    {
        PatientVitals v{82, BloodPressure{118, 76}, 99, 16, 0.0, true, 90, 5};
        list.emplace_back("PT-SAMP-015", "Chloe Martin", 27, Gender::FEMALE,
                          EmergencyType::PREGNANCY, v,
                          "Normal active first-stage labor, regular contractions every 4 minutes, intact membranes");
    }

    // 16. Pediatric Airway Foreign Body Aspiration
    {
        PatientVitals v{148, BloodPressure{96, 60}, 89, 36, 0.0, true, 15, 8};
        list.emplace_back("PT-SAMP-016", "Oliver Twist", 8, Gender::MALE,
                          EmergencyType::PEDIATRIC_EMERGENCY, v,
                          "Foreign body airway aspiration (toy piece), acute stridor and suprasternal tugging");
    }

    return list;
}

void MedicalDatabase::AddPatient(PatientProfile patient) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_patients[patient.GetPatientId()] = std::move(patient);
}

bool MedicalDatabase::RemovePatient(const std::string& patientId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_patients.erase(patientId) > 0;
}

std::optional<PatientProfile> MedicalDatabase::GetPatientById(const std::string& patientId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_patients.find(patientId);
    if (it != m_patients.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<PatientProfile> MedicalDatabase::GetAllPatients() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<PatientProfile> results;
    results.reserve(m_patients.size());
    for (const auto& [id, patient] : m_patients) {
        results.push_back(patient);
    }
    return results;
}

std::vector<PatientProfile> MedicalDatabase::GetPatientsByDisease(EmergencyType disease) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<PatientProfile> results;
    for (const auto& [id, patient] : m_patients) {
        if (patient.GetDisease() == disease) {
            results.push_back(patient);
        }
    }
    return results;
}

std::vector<PatientProfile> MedicalDatabase::GetPatientsByPriority(PriorityLevel priority) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<PatientProfile> results;
    for (const auto& [id, patient] : m_patients) {
        if (PriorityEngine::CalculatePriority(patient) == priority) {
            results.push_back(patient);
        }
    }
    return results;
}

size_t MedicalDatabase::GetTotalPatientCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_patients.size();
}

size_t MedicalDatabase::GetCountByPriority(PriorityLevel priority) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    for (const auto& [id, patient] : m_patients) {
        if (PriorityEngine::CalculatePriority(patient) == priority) {
            ++count;
        }
    }
    return count;
}

std::string MedicalDatabase::ExportDatabaseJson() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::ostringstream oss;
    oss << "[";
    size_t idx = 0;
    for (const auto& [id, patient] : m_patients) {
        oss << patient.ToJson();
        if (++idx < m_patients.size()) oss << ",";
    }
    oss << "]";
    return oss.str();
}

bool MedicalDatabase::ExportToFile(const std::string& filePath) const {
    std::string jsonStr = ExportDatabaseJson();
    std::ofstream outFile(filePath, std::ios::out | std::ios::trunc);
    if (!outFile.is_open()) {
        return false;
    }
    outFile << jsonStr;
    outFile.close();
    return true;
}

void MedicalDatabase::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_patients.clear();
}

void MedicalDatabase::ResetToDefaults() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_patients.clear();
    auto samples = GenerateSamplePatients();
    for (auto& patient : samples) {
        m_patients[patient.GetPatientId()] = std::move(patient);
    }
}

} // namespace medical
} // namespace aegis
