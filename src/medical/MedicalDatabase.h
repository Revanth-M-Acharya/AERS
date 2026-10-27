/**
 * @file MedicalDatabase.h
 * @brief Realistic Emergency Clinical Database & Sample Patient Generator for SMART AEGIS.
 */

#ifndef AEGIS_MEDICAL_DATABASE_H
#define AEGIS_MEDICAL_DATABASE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <memory>

#include "medical/PatientProfile.h"
#include "medical/PriorityEngine.h"

namespace aegis {
namespace medical {

/**
 * @brief Repository and generator of realistic emergency patient profiles.
 */
class MedicalDatabase {
public:
    MedicalDatabase();
    ~MedicalDatabase() = default;

    /**
     * @brief Generates at least 15 realistic predefined emergency patients.
     * Covers Heart Attack, Cardiac Arrest, Stroke, Major Accident, Pregnancy,
     * Burn Injury, Trauma, Internal Bleeding, Respiratory Failure, Poisoning,
     * Pediatric Emergency, and Organ Transport.
     */
    static std::vector<PatientProfile> GenerateSamplePatients();

    // Registry / Catalog Operations
    void AddPatient(PatientProfile patient);
    bool RemovePatient(const std::string& patientId);
    std::optional<PatientProfile> GetPatientById(const std::string& patientId) const;

    std::vector<PatientProfile> GetAllPatients() const;
    std::vector<PatientProfile> GetPatientsByDisease(EmergencyType disease) const;
    std::vector<PatientProfile> GetPatientsByPriority(PriorityLevel priority) const;

    size_t GetTotalPatientCount() const;
    size_t GetCountByPriority(PriorityLevel priority) const;

    // JSON Export
    std::string ExportDatabaseJson() const;
    bool ExportToFile(const std::string& filePath) const;

    void Clear();
    void ResetToDefaults();

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, PatientProfile> m_patients{};
};

} // namespace medical
} // namespace aegis

#endif // AEGIS_MEDICAL_DATABASE_H
