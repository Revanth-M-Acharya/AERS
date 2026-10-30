/**
 * @file RuntimeEventFlow.h
 * @brief SMART AEGIS (AERS) Phase 16C - Distributed Runtime Event Flow Data Models.
 *
 * Defines input and output telemetry models for end-to-end emergency mission execution
 * across all integrated municipal subsystems.
 */

#ifndef AEGIS_RUNTIME_EVENT_FLOW_H
#define AEGIS_RUNTIME_EVENT_FLOW_H

#include <string>
#include <vector>
#include <cstdint>
#include <sstream>

namespace aegis {
namespace runtime {

/**
 * @brief Telemetry input describing an incoming emergency incident.
 */
struct EmergencyPipelineInput {
    std::string callId{""};
    std::string callerName{"Anonymous"};
    std::string callerContact{"+91-9999999999"};
    double incidentLatitude{12.9716};
    double incidentLongitude{77.5946};
    std::string emergencyDescription{"Severe acute cardiac arrest with unconsciousness"};
    std::string conditionHint{"CARDIAC_ARREST"};
    std::string patientName{"John Doe"};
    uint16_t patientAge{55};
    std::string patientGender{"Male"};
    uint16_t heartRate{140};
    uint16_t systolicBP{80};
    uint16_t diastolicBP{50};
    uint16_t spO2{88};
    uint16_t respirationRate{28};
    bool isConscious{false};
    double targetSpeedKmh{60.0};
};

/**
 * @brief Complete diagnostic outcome of an integrated emergency pipeline run.
 */
struct EmergencyPipelineResult {
    bool success{false};
    std::string incidentId{""};
    std::string missionId{""};
    std::string patientId{""};
    double clinicalPriorityScore{0.0};
    std::string priorityLevel{"UNKNOWN"};
    std::string assignedAmbulanceId{""};
    std::string assignedHospitalId{""};
    bool bedReserved{false};
    bool icuBedReserved{false};
    std::vector<std::string> routeWaypoints{};
    std::vector<std::string> preemptionIntersections{};
    double estimatedTransitTimeSeconds{0.0};
    std::string greenWavePlanId{""};
    bool consensusApproved{false};
    uint32_t consensusVotes{0};
    uint32_t commandsDispatched{0};
    uint32_t acksReceived{0};
    std::string missionStatus{"NEW"};
    std::string missionStage{"CALL_RECEIVED"};
    uint64_t executionLatencyUs{0};
    std::string failureReason{""};
    std::vector<std::string> operationalLogs{};

    std::string ToString() const {
        std::ostringstream oss;
        oss << "[EMERGENCY PIPELINE RESULT]\n"
            << "  Success           : " << (success ? "YES" : "NO") << "\n"
            << "  Incident ID       : " << incidentId << "\n"
            << "  Mission ID        : " << missionId << "\n"
            << "  Priority Score    : " << clinicalPriorityScore << " (" << priorityLevel << ")\n"
            << "  Ambulance Unit    : " << assignedAmbulanceId << "\n"
            << "  Receiving Hospital: " << assignedHospitalId << " (Bed: " << (bedReserved ? "YES" : "NO")
            << ", ICU: " << (icuBedReserved ? "YES" : "NO") << ")\n"
            << "  Route Path        : ";
        for (size_t i = 0; i < routeWaypoints.size(); ++i) {
            oss << routeWaypoints[i] << (i + 1 < routeWaypoints.size() ? " -> " : "");
        }
        oss << "\n  Consensus Status  : " << (consensusApproved ? "APPROVED" : "PENDING")
            << " (" << consensusVotes << " votes)\n"
            << "  Network Commands  : " << commandsDispatched << " dispatched, " << acksReceived << " ACKs\n"
            << "  Mission Lifecycle : " << missionStatus << " / " << missionStage << "\n"
            << "  Execution Latency : " << (static_cast<double>(executionLatencyUs) / 1000.0) << " ms\n";
        if (!success) {
            oss << "  Failure Reason    : " << failureReason << "\n";
        }
        return oss.str();
    }
};

} // namespace runtime
} // namespace aegis

#endif // AEGIS_RUNTIME_EVENT_FLOW_H
