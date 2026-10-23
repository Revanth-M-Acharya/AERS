/**
 * @file serializer.hpp
 * @brief Formal schemas and JSON serializer/deserializer for all 10 ICD message types.
 */

#ifndef AEGIS_NETWORK_SERIALIZER_HPP
#define AEGIS_NETWORK_SERIALIZER_HPP

#include <string>
#include <vector>
#include <cstdint>
#include "aegis/common/error_codes.hpp"
#include "aegis/common/result.hpp"

namespace aegis {
namespace network {

// 7.1 EmergencyRequest
struct EmergencyRequestMsg {
    std::string incidentId;
    uint64_t timestamp{0};
    std::string callerContact;
    double latitude{0.0};
    double longitude{0.0};
    std::string triageCategory; // CODE_RED, CODE_AMBER, CODE_GREEN
    std::string incidentType;   // CARDIAC, TRAUMA, etc.
    std::vector<std::string> requiredCapabilities;
};

// 7.2 GPSUpdate
struct GPSUpdateMsg {
    std::string unitId;
    uint64_t timestamp{0};
    double latitude{0.0};
    double longitude{0.0};
    float speed{0.0f};
    float bearing{0.0f};
    float hdop{1.0f};
    uint8_t fixQuality{1};
};

// 7.3 VitalsUpdate
struct VitalsUpdateMsg {
    std::string unitId;
    std::string patientId;
    uint64_t timestamp{0};
    uint16_t heartRate{0};
    uint8_t spO2{0};
    uint16_t systolicBP{0};
    uint16_t diastolicBP{0};
    uint8_t respirationRate{0};
    uint8_t gcsScore{15};
};

// 7.4 TrafficPreemptionRequest
struct TrafficPreemptionRequestMsg {
    std::string requestId;
    std::string intersectionId;
    std::string unitId;
    uint8_t corridorPriority{1};
    std::string requestedPhase;
    uint16_t estimatedArrivalSec{0};
    uint16_t holdDurationSec{20};
    std::string signature;
};

// 7.5 TrafficStatus
struct TrafficStatusMsg {
    std::string intersectionId;
    uint64_t timestamp{0};
    std::string currentMode;
    std::string northSouthPhase;
    std::string eastWestPhase;
    std::string activePreemptionUnit;
    bool wdtHealthy{true};
};

// 7.6 HospitalAlert
struct HospitalAlertMsg {
    std::string alertId;
    std::string unitId;
    std::string patientId;
    std::string severity;
    std::string alarmType;
    std::string triggeredValue;
    uint64_t timestamp{0};
};

// 7.7 HospitalCommand
struct HospitalCommandMsg {
    std::string commandId;
    std::string unitId;
    std::string physicianName;
    std::string instruction;
    std::string destinationBay;
    uint64_t timestamp{0};
};

// 7.8 PriorityDecision
struct PriorityDecisionMsg {
    std::string decisionId;
    std::string contestedResourceId;
    std::string grantedUnitId;
    std::string yieldingUnitId;
    float grantedPriorityScore{0.0f};
    float yieldingPriorityScore{0.0f};
    float yieldAdvisorySpeedKmH{0.0f};
};

// 7.9 Heartbeat
struct HeartbeatMsg {
    std::string nodeId;
    uint64_t timestamp{0};
    float cpuUsagePct{0.0f};
    float ramUsageMb{0.0f};
    std::string processStatus;
    uint64_t sequenceNumber{0};
};

// 7.10 FaultEvent
struct FaultEventMsg {
    std::string faultId;
    std::string originModule;
    std::string errorCode;
    std::string severity;
    std::string description;
    uint64_t timestamp{0};
};

class Serializer {
public:
    // Serializers
    static std::string Serialize(const EmergencyRequestMsg& msg);
    static std::string Serialize(const GPSUpdateMsg& msg);
    static std::string Serialize(const VitalsUpdateMsg& msg);
    static std::string Serialize(const TrafficPreemptionRequestMsg& msg);
    static std::string Serialize(const TrafficStatusMsg& msg);
    static std::string Serialize(const HospitalAlertMsg& msg);
    static std::string Serialize(const HospitalCommandMsg& msg);
    static std::string Serialize(const PriorityDecisionMsg& msg);
    static std::string Serialize(const HeartbeatMsg& msg);
    static std::string Serialize(const FaultEventMsg& msg);

    // Deserializers
    static common::Result<EmergencyRequestMsg> DeserializeEmergencyRequest(std::string_view json);
    static common::Result<GPSUpdateMsg> DeserializeGPSUpdate(std::string_view json);
    static common::Result<VitalsUpdateMsg> DeserializeVitalsUpdate(std::string_view json);
    static common::Result<TrafficPreemptionRequestMsg> DeserializeTrafficPreemptionRequest(std::string_view json);
    static common::Result<TrafficStatusMsg> DeserializeTrafficStatus(std::string_view json);
    static common::Result<HospitalAlertMsg> DeserializeHospitalAlert(std::string_view json);
    static common::Result<HospitalCommandMsg> DeserializeHospitalCommand(std::string_view json);
    static common::Result<PriorityDecisionMsg> DeserializePriorityDecision(std::string_view json);
    static common::Result<HeartbeatMsg> DeserializeHeartbeat(std::string_view json);
    static common::Result<FaultEventMsg> DeserializeFaultEvent(std::string_view json);
};

} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_SERIALIZER_HPP
