#include "aegis/network/serializer.hpp"
#include "aegis/common/json_helper.hpp"
#include <sstream>
#include <iomanip>

namespace aegis {
namespace network {

using common::JsonHelper;

static std::vector<std::string> ExtractStringArray(std::string_view json, std::string_view key) {
    std::vector<std::string> result;
    std::string needle = "\"" + std::string(key) + "\"";
    auto pos = json.find(needle);
    if (pos == std::string_view::npos) return result;

    auto openBracket = json.find('[', pos + needle.size());
    if (openBracket == std::string_view::npos) return result;

    auto closeBracket = json.find(']', openBracket);
    if (closeBracket == std::string_view::npos) return result;

    std::string_view arrayContent = json.substr(openBracket + 1, closeBracket - openBracket - 1);
    size_t idx = 0;
    while (idx < arrayContent.size()) {
        auto q1 = arrayContent.find('"', idx);
        if (q1 == std::string_view::npos) break;
        auto q2 = arrayContent.find('"', q1 + 1);
        if (q2 == std::string_view::npos) break;
        result.emplace_back(arrayContent.substr(q1 + 1, q2 - q1 - 1));
        idx = q2 + 1;
    }
    return result;
}

std::string Serializer::Serialize(const EmergencyRequestMsg& msg) {
    std::ostringstream oss;
    oss << "{"
        << JsonHelper::FormatField("incidentId", msg.incidentId)
        << JsonHelper::FormatField("timestamp", static_cast<int64_t>(msg.timestamp))
        << JsonHelper::FormatField("callerContact", msg.callerContact)
        << JsonHelper::FormatField("latitude", msg.latitude, 6)
        << JsonHelper::FormatField("longitude", msg.longitude, 6)
        << JsonHelper::FormatField("triageCategory", msg.triageCategory)
        << JsonHelper::FormatField("incidentType", msg.incidentType)
        << "\"requiredCapabilities\":[";
    for (size_t i = 0; i < msg.requiredCapabilities.size(); ++i) {
        oss << "\"" << msg.requiredCapabilities[i] << "\"";
        if (i + 1 < msg.requiredCapabilities.size()) {
            oss << ",";
        }
    }
    oss << "]}";
    return oss.str();
}

std::string Serializer::Serialize(const GPSUpdateMsg& msg) {
    std::ostringstream oss;
    oss << "{"
        << JsonHelper::FormatField("unitId", msg.unitId)
        << JsonHelper::FormatField("timestamp", static_cast<int64_t>(msg.timestamp))
        << JsonHelper::FormatField("latitude", msg.latitude, 6)
        << JsonHelper::FormatField("longitude", msg.longitude, 6)
        << JsonHelper::FormatField("speed", static_cast<double>(msg.speed), 2)
        << JsonHelper::FormatField("bearing", static_cast<double>(msg.bearing), 2)
        << JsonHelper::FormatField("hdop", static_cast<double>(msg.hdop), 2)
        << JsonHelper::FormatField("fixQuality", static_cast<int64_t>(msg.fixQuality), true)
        << "}";
    return oss.str();
}

std::string Serializer::Serialize(const VitalsUpdateMsg& msg) {
    std::ostringstream oss;
    oss << "{"
        << JsonHelper::FormatField("unitId", msg.unitId)
        << JsonHelper::FormatField("patientId", msg.patientId)
        << JsonHelper::FormatField("timestamp", static_cast<int64_t>(msg.timestamp))
        << JsonHelper::FormatField("heartRate", static_cast<int64_t>(msg.heartRate))
        << JsonHelper::FormatField("spO2", static_cast<int64_t>(msg.spO2))
        << JsonHelper::FormatField("systolicBP", static_cast<int64_t>(msg.systolicBP))
        << JsonHelper::FormatField("diastolicBP", static_cast<int64_t>(msg.diastolicBP))
        << JsonHelper::FormatField("respirationRate", static_cast<int64_t>(msg.respirationRate))
        << JsonHelper::FormatField("gcsScore", static_cast<int64_t>(msg.gcsScore), true)
        << "}";
    return oss.str();
}

std::string Serializer::Serialize(const TrafficPreemptionRequestMsg& msg) {
    std::ostringstream oss;
    oss << "{"
        << JsonHelper::FormatField("requestId", msg.requestId)
        << JsonHelper::FormatField("intersectionId", msg.intersectionId)
        << JsonHelper::FormatField("unitId", msg.unitId)
        << JsonHelper::FormatField("corridorPriority", static_cast<int64_t>(msg.corridorPriority))
        << JsonHelper::FormatField("requestedPhase", msg.requestedPhase)
        << JsonHelper::FormatField("estimatedArrivalSec", static_cast<int64_t>(msg.estimatedArrivalSec))
        << JsonHelper::FormatField("holdDurationSec", static_cast<int64_t>(msg.holdDurationSec))
        << JsonHelper::FormatField("signature", msg.signature, true)
        << "}";
    return oss.str();
}

std::string Serializer::Serialize(const TrafficStatusMsg& msg) {
    std::ostringstream oss;
    oss << "{"
        << JsonHelper::FormatField("intersectionId", msg.intersectionId)
        << JsonHelper::FormatField("timestamp", static_cast<int64_t>(msg.timestamp))
        << JsonHelper::FormatField("currentMode", msg.currentMode)
        << JsonHelper::FormatField("northSouthPhase", msg.northSouthPhase)
        << JsonHelper::FormatField("eastWestPhase", msg.eastWestPhase)
        << JsonHelper::FormatField("activePreemptionUnit", msg.activePreemptionUnit)
        << JsonHelper::FormatField("wdtHealthy", msg.wdtHealthy, true)
        << "}";
    return oss.str();
}

std::string Serializer::Serialize(const HospitalAlertMsg& msg) {
    std::ostringstream oss;
    oss << "{"
        << JsonHelper::FormatField("alertId", msg.alertId)
        << JsonHelper::FormatField("unitId", msg.unitId)
        << JsonHelper::FormatField("patientId", msg.patientId)
        << JsonHelper::FormatField("severity", msg.severity)
        << JsonHelper::FormatField("alarmType", msg.alarmType)
        << JsonHelper::FormatField("triggeredValue", msg.triggeredValue)
        << JsonHelper::FormatField("timestamp", static_cast<int64_t>(msg.timestamp), true)
        << "}";
    return oss.str();
}

std::string Serializer::Serialize(const HospitalCommandMsg& msg) {
    std::ostringstream oss;
    oss << "{"
        << JsonHelper::FormatField("commandId", msg.commandId)
        << JsonHelper::FormatField("unitId", msg.unitId)
        << JsonHelper::FormatField("physicianName", msg.physicianName)
        << JsonHelper::FormatField("instruction", msg.instruction)
        << JsonHelper::FormatField("destinationBay", msg.destinationBay)
        << JsonHelper::FormatField("timestamp", static_cast<int64_t>(msg.timestamp), true)
        << "}";
    return oss.str();
}

std::string Serializer::Serialize(const PriorityDecisionMsg& msg) {
    std::ostringstream oss;
    oss << "{"
        << JsonHelper::FormatField("decisionId", msg.decisionId)
        << JsonHelper::FormatField("contestedResourceId", msg.contestedResourceId)
        << JsonHelper::FormatField("grantedUnitId", msg.grantedUnitId)
        << JsonHelper::FormatField("yieldingUnitId", msg.yieldingUnitId)
        << JsonHelper::FormatField("grantedPriorityScore", static_cast<double>(msg.grantedPriorityScore), 2)
        << JsonHelper::FormatField("yieldingPriorityScore", static_cast<double>(msg.yieldingPriorityScore), 2)
        << JsonHelper::FormatField("yieldAdvisorySpeedKmH", static_cast<double>(msg.yieldAdvisorySpeedKmH), 2, true)
        << "}";
    return oss.str();
}

std::string Serializer::Serialize(const HeartbeatMsg& msg) {
    std::ostringstream oss;
    oss << "{"
        << JsonHelper::FormatField("nodeId", msg.nodeId)
        << JsonHelper::FormatField("timestamp", static_cast<int64_t>(msg.timestamp))
        << JsonHelper::FormatField("cpuUsagePct", static_cast<double>(msg.cpuUsagePct), 2)
        << JsonHelper::FormatField("ramUsageMb", static_cast<double>(msg.ramUsageMb), 2)
        << JsonHelper::FormatField("processStatus", msg.processStatus)
        << JsonHelper::FormatField("sequenceNumber", static_cast<int64_t>(msg.sequenceNumber), true)
        << "}";
    return oss.str();
}

std::string Serializer::Serialize(const FaultEventMsg& msg) {
    std::ostringstream oss;
    oss << "{"
        << JsonHelper::FormatField("faultId", msg.faultId)
        << JsonHelper::FormatField("originModule", msg.originModule)
        << JsonHelper::FormatField("errorCode", msg.errorCode)
        << JsonHelper::FormatField("severity", msg.severity)
        << JsonHelper::FormatField("description", msg.description)
        << JsonHelper::FormatField("timestamp", static_cast<int64_t>(msg.timestamp), true)
        << "}";
    return oss.str();
}

// Deserializers
common::Result<EmergencyRequestMsg> Serializer::DeserializeEmergencyRequest(std::string_view json) {
    auto id = JsonHelper::ExtractString(json, "incidentId");
    auto ts = JsonHelper::ExtractInt64(json, "timestamp");
    auto lat = JsonHelper::ExtractDouble(json, "latitude");
    auto lon = JsonHelper::ExtractDouble(json, "longitude");
    auto cat = JsonHelper::ExtractString(json, "triageCategory");

    if (!id || !ts || !lat || !lon || !cat) {
        return common::Result<EmergencyRequestMsg>::Err(common::ErrorCode::ERR_INVALID_MESSAGE);
    }

    EmergencyRequestMsg msg;
    msg.incidentId = *id;
    msg.timestamp = static_cast<uint64_t>(*ts);
    msg.latitude = *lat;
    msg.longitude = *lon;
    msg.triageCategory = *cat;
    msg.callerContact = JsonHelper::ExtractString(json, "callerContact").value_or("");
    msg.incidentType = JsonHelper::ExtractString(json, "incidentType").value_or("GENERAL");
    msg.requiredCapabilities = ExtractStringArray(json, "requiredCapabilities");
    return common::Result<EmergencyRequestMsg>::Ok(msg);
}

common::Result<GPSUpdateMsg> Serializer::DeserializeGPSUpdate(std::string_view json) {
    auto unitId = JsonHelper::ExtractString(json, "unitId");
    auto ts = JsonHelper::ExtractInt64(json, "timestamp");
    auto lat = JsonHelper::ExtractDouble(json, "latitude");
    auto lon = JsonHelper::ExtractDouble(json, "longitude");

    if (!unitId || !ts || !lat || !lon) {
        return common::Result<GPSUpdateMsg>::Err(common::ErrorCode::ERR_INVALID_MESSAGE);
    }

    GPSUpdateMsg msg;
    msg.unitId = *unitId;
    msg.timestamp = static_cast<uint64_t>(*ts);
    msg.latitude = *lat;
    msg.longitude = *lon;
    msg.speed = static_cast<float>(JsonHelper::ExtractDouble(json, "speed").value_or(0.0));
    msg.bearing = static_cast<float>(JsonHelper::ExtractDouble(json, "bearing").value_or(0.0));
    msg.hdop = static_cast<float>(JsonHelper::ExtractDouble(json, "hdop").value_or(1.0));
    msg.fixQuality = static_cast<uint8_t>(JsonHelper::ExtractInt64(json, "fixQuality").value_or(1));
    return common::Result<GPSUpdateMsg>::Ok(msg);
}

common::Result<VitalsUpdateMsg> Serializer::DeserializeVitalsUpdate(std::string_view json) {
    auto unitId = JsonHelper::ExtractString(json, "unitId");
    auto patientId = JsonHelper::ExtractString(json, "patientId");
    auto ts = JsonHelper::ExtractInt64(json, "timestamp");
    auto hr = JsonHelper::ExtractInt64(json, "heartRate");
    auto spO2 = JsonHelper::ExtractInt64(json, "spO2");

    if (!unitId || !patientId || !ts || !hr || !spO2) {
        return common::Result<VitalsUpdateMsg>::Err(common::ErrorCode::ERR_INVALID_MESSAGE);
    }

    VitalsUpdateMsg msg;
    msg.unitId = *unitId;
    msg.patientId = *patientId;
    msg.timestamp = static_cast<uint64_t>(*ts);
    msg.heartRate = static_cast<uint16_t>(*hr);
    msg.spO2 = static_cast<uint8_t>(*spO2);
    msg.systolicBP = static_cast<uint16_t>(JsonHelper::ExtractInt64(json, "systolicBP").value_or(120));
    msg.diastolicBP = static_cast<uint16_t>(JsonHelper::ExtractInt64(json, "diastolicBP").value_or(80));
    msg.respirationRate = static_cast<uint8_t>(JsonHelper::ExtractInt64(json, "respirationRate").value_or(16));
    msg.gcsScore = static_cast<uint8_t>(JsonHelper::ExtractInt64(json, "gcsScore").value_or(15));
    return common::Result<VitalsUpdateMsg>::Ok(msg);
}

common::Result<TrafficPreemptionRequestMsg> Serializer::DeserializeTrafficPreemptionRequest(std::string_view json) {
    auto reqId = JsonHelper::ExtractString(json, "requestId");
    auto intId = JsonHelper::ExtractString(json, "intersectionId");
    auto unitId = JsonHelper::ExtractString(json, "unitId");
    auto phase = JsonHelper::ExtractString(json, "requestedPhase");

    if (!reqId || !intId || !unitId || !phase) {
        return common::Result<TrafficPreemptionRequestMsg>::Err(common::ErrorCode::ERR_INVALID_MESSAGE);
    }

    TrafficPreemptionRequestMsg msg;
    msg.requestId = *reqId;
    msg.intersectionId = *intId;
    msg.unitId = *unitId;
    msg.requestedPhase = *phase;
    msg.corridorPriority = static_cast<uint8_t>(JsonHelper::ExtractInt64(json, "corridorPriority").value_or(1));
    msg.estimatedArrivalSec = static_cast<uint16_t>(JsonHelper::ExtractInt64(json, "estimatedArrivalSec").value_or(0));
    msg.holdDurationSec = static_cast<uint16_t>(JsonHelper::ExtractInt64(json, "holdDurationSec").value_or(20));
    msg.signature = JsonHelper::ExtractString(json, "signature").value_or("");
    return common::Result<TrafficPreemptionRequestMsg>::Ok(msg);
}

common::Result<TrafficStatusMsg> Serializer::DeserializeTrafficStatus(std::string_view json) {
    auto intId = JsonHelper::ExtractString(json, "intersectionId");
    auto mode = JsonHelper::ExtractString(json, "currentMode");

    if (!intId || !mode) {
        return common::Result<TrafficStatusMsg>::Err(common::ErrorCode::ERR_INVALID_MESSAGE);
    }

    TrafficStatusMsg msg;
    msg.intersectionId = *intId;
    msg.currentMode = *mode;
    msg.timestamp = static_cast<uint64_t>(JsonHelper::ExtractInt64(json, "timestamp").value_or(0));
    msg.northSouthPhase = JsonHelper::ExtractString(json, "northSouthPhase").value_or("RED");
    msg.eastWestPhase = JsonHelper::ExtractString(json, "eastWestPhase").value_or("RED");
    msg.activePreemptionUnit = JsonHelper::ExtractString(json, "activePreemptionUnit").value_or("");
    msg.wdtHealthy = JsonHelper::ExtractBool(json, "wdtHealthy").value_or(true);
    return common::Result<TrafficStatusMsg>::Ok(msg);
}

common::Result<HospitalAlertMsg> Serializer::DeserializeHospitalAlert(std::string_view json) {
    auto alertId = JsonHelper::ExtractString(json, "alertId");
    auto unitId = JsonHelper::ExtractString(json, "unitId");
    auto type = JsonHelper::ExtractString(json, "alarmType");

    if (!alertId || !unitId || !type) {
        return common::Result<HospitalAlertMsg>::Err(common::ErrorCode::ERR_INVALID_MESSAGE);
    }

    HospitalAlertMsg msg;
    msg.alertId = *alertId;
    msg.unitId = *unitId;
    msg.alarmType = *type;
    msg.patientId = JsonHelper::ExtractString(json, "patientId").value_or("");
    msg.severity = JsonHelper::ExtractString(json, "severity").value_or("CRITICAL");
    msg.triggeredValue = JsonHelper::ExtractString(json, "triggeredValue").value_or("");
    msg.timestamp = static_cast<uint64_t>(JsonHelper::ExtractInt64(json, "timestamp").value_or(0));
    return common::Result<HospitalAlertMsg>::Ok(msg);
}

common::Result<HospitalCommandMsg> Serializer::DeserializeHospitalCommand(std::string_view json) {
    auto cmdId = JsonHelper::ExtractString(json, "commandId");
    auto unitId = JsonHelper::ExtractString(json, "unitId");
    auto inst = JsonHelper::ExtractString(json, "instruction");

    if (!cmdId || !unitId || !inst) {
        return common::Result<HospitalCommandMsg>::Err(common::ErrorCode::ERR_INVALID_MESSAGE);
    }

    HospitalCommandMsg msg;
    msg.commandId = *cmdId;
    msg.unitId = *unitId;
    msg.instruction = *inst;
    msg.physicianName = JsonHelper::ExtractString(json, "physicianName").value_or("");
    msg.destinationBay = JsonHelper::ExtractString(json, "destinationBay").value_or("");
    msg.timestamp = static_cast<uint64_t>(JsonHelper::ExtractInt64(json, "timestamp").value_or(0));
    return common::Result<HospitalCommandMsg>::Ok(msg);
}

common::Result<PriorityDecisionMsg> Serializer::DeserializePriorityDecision(std::string_view json) {
    auto id = JsonHelper::ExtractString(json, "decisionId");
    auto granted = JsonHelper::ExtractString(json, "grantedUnitId");
    auto yielding = JsonHelper::ExtractString(json, "yieldingUnitId");

    if (!id || !granted || !yielding) {
        return common::Result<PriorityDecisionMsg>::Err(common::ErrorCode::ERR_INVALID_MESSAGE);
    }

    PriorityDecisionMsg msg;
    msg.decisionId = *id;
    msg.grantedUnitId = *granted;
    msg.yieldingUnitId = *yielding;
    msg.contestedResourceId = JsonHelper::ExtractString(json, "contestedResourceId").value_or("");
    msg.grantedPriorityScore = static_cast<float>(JsonHelper::ExtractDouble(json, "grantedPriorityScore").value_or(0.0));
    msg.yieldingPriorityScore = static_cast<float>(JsonHelper::ExtractDouble(json, "yieldingPriorityScore").value_or(0.0));
    msg.yieldAdvisorySpeedKmH = static_cast<float>(JsonHelper::ExtractDouble(json, "yieldAdvisorySpeedKmH").value_or(0.0));
    return common::Result<PriorityDecisionMsg>::Ok(msg);
}

common::Result<HeartbeatMsg> Serializer::DeserializeHeartbeat(std::string_view json) {
    auto nodeId = JsonHelper::ExtractString(json, "nodeId");
    auto ts = JsonHelper::ExtractInt64(json, "timestamp");
    auto seq = JsonHelper::ExtractInt64(json, "sequenceNumber");

    if (!nodeId || !ts || !seq) {
        return common::Result<HeartbeatMsg>::Err(common::ErrorCode::ERR_INVALID_MESSAGE);
    }

    HeartbeatMsg msg;
    msg.nodeId = *nodeId;
    msg.timestamp = static_cast<uint64_t>(*ts);
    msg.sequenceNumber = static_cast<uint64_t>(*seq);
    msg.cpuUsagePct = static_cast<float>(JsonHelper::ExtractDouble(json, "cpuUsagePct").value_or(0.0));
    msg.ramUsageMb = static_cast<float>(JsonHelper::ExtractDouble(json, "ramUsageMb").value_or(0.0));
    msg.processStatus = JsonHelper::ExtractString(json, "processStatus").value_or("HEALTHY");
    return common::Result<HeartbeatMsg>::Ok(msg);
}

common::Result<FaultEventMsg> Serializer::DeserializeFaultEvent(std::string_view json) {
    auto faultId = JsonHelper::ExtractString(json, "faultId");
    auto mod = JsonHelper::ExtractString(json, "originModule");
    auto code = JsonHelper::ExtractString(json, "errorCode");

    if (!faultId || !mod || !code) {
        return common::Result<FaultEventMsg>::Err(common::ErrorCode::ERR_INVALID_MESSAGE);
    }

    FaultEventMsg msg;
    msg.faultId = *faultId;
    msg.originModule = *mod;
    msg.errorCode = *code;
    msg.severity = JsonHelper::ExtractString(json, "severity").value_or("ERROR");
    msg.description = JsonHelper::ExtractString(json, "description").value_or("");
    msg.timestamp = static_cast<uint64_t>(JsonHelper::ExtractInt64(json, "timestamp").value_or(0));
    return common::Result<FaultEventMsg>::Ok(msg);
}

std::string Serializer::Serialize(const IntersectionStatusMsg& msg) {
    std::ostringstream oss;
    oss << "{"
        << JsonHelper::FormatField("intersection_id", msg.intersectionId)
        << JsonHelper::FormatField("current_state", msg.currentState)
        << JsonHelper::FormatField("emergency_active", msg.emergencyActive)
        << JsonHelper::FormatField("recovery_active", msg.recoveryActive)
        << JsonHelper::FormatField("heartbeat_counter", static_cast<int64_t>(msg.heartbeatCounter))
        << JsonHelper::FormatField("uptime_ms", static_cast<int64_t>(msg.uptimeMs))
        << JsonHelper::FormatField("timestamp", static_cast<int64_t>(msg.timestamp))
        << JsonHelper::FormatField("health", msg.health, true)
        << "}";
    return oss.str();
}

std::string Serializer::Serialize(const IntersectionCommandMsg& msg) {
    std::ostringstream oss;
    oss << "{"
        << JsonHelper::FormatField("command", msg.command)
        << JsonHelper::FormatField("intersection_id", msg.intersectionId)
        << JsonHelper::FormatField("timestamp", static_cast<int64_t>(msg.timestamp), true)
        << "}";
    return oss.str();
}

common::Result<IntersectionStatusMsg> Serializer::DeserializeIntersectionStatus(std::string_view json) {
    auto id = JsonHelper::ExtractString(json, "intersection_id");
    if (!id) id = JsonHelper::ExtractString(json, "intersectionId");

    auto state = JsonHelper::ExtractString(json, "current_state");
    if (!state) state = JsonHelper::ExtractString(json, "currentState");

    if (!id || !state) {
        return common::Result<IntersectionStatusMsg>::Err(common::ErrorCode::ERR_INVALID_MESSAGE);
    }

    IntersectionStatusMsg msg;
    msg.intersectionId = *id;
    msg.currentState = *state;

    auto em = JsonHelper::ExtractBool(json, "emergency_active");
    if (!em) em = JsonHelper::ExtractBool(json, "emergencyActive");
    msg.emergencyActive = em.value_or(false);

    auto rec = JsonHelper::ExtractBool(json, "recovery_active");
    if (!rec) rec = JsonHelper::ExtractBool(json, "recoveryActive");
    msg.recoveryActive = rec.value_or(false);

    auto hb = JsonHelper::ExtractInt64(json, "heartbeat_counter");
    if (!hb) hb = JsonHelper::ExtractInt64(json, "heartbeatCounter");
    msg.heartbeatCounter = static_cast<uint64_t>(hb.value_or(0));

    auto up = JsonHelper::ExtractInt64(json, "uptime_ms");
    if (!up) up = JsonHelper::ExtractInt64(json, "uptimeMs");
    msg.uptimeMs = static_cast<uint64_t>(up.value_or(0));

    msg.timestamp = static_cast<uint64_t>(JsonHelper::ExtractInt64(json, "timestamp").value_or(0));
    msg.health = JsonHelper::ExtractString(json, "health").value_or("HEALTHY");

    return common::Result<IntersectionStatusMsg>::Ok(msg);
}

common::Result<IntersectionCommandMsg> Serializer::DeserializeIntersectionCommand(std::string_view json) {
    auto cmd = JsonHelper::ExtractString(json, "command");
    if (!cmd) {
        return common::Result<IntersectionCommandMsg>::Err(common::ErrorCode::ERR_INVALID_MESSAGE);
    }

    IntersectionCommandMsg msg;
    msg.command = *cmd;

    auto id = JsonHelper::ExtractString(json, "intersection_id");
    if (!id) id = JsonHelper::ExtractString(json, "intersectionId");
    msg.intersectionId = id.value_or("");

    msg.timestamp = static_cast<uint64_t>(JsonHelper::ExtractInt64(json, "timestamp").value_or(0));
    return common::Result<IntersectionCommandMsg>::Ok(msg);
}

} // namespace network
} // namespace aegis

