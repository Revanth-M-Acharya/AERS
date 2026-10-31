/**
 * @file HardwareConfiguration.cpp
 * @brief SMART AEGIS (AERS) Phase 20 - Physical Hardware Configuration Implementation.
 */

#include "HardwareConfiguration.h"
#include "aegis/common/json_helper.hpp"
#include <fstream>
#include <sstream>

namespace aegis {
namespace hardware {

HardwareConfiguration::HardwareConfiguration() {
    // Populate default static topology
    m_nodes = {
        {"COORDINATOR", "Central EOC Coordinator", "COORDINATOR", "192.168.1.100", 9000, "192.168.1.100", 9000, "DC:A6:32:01:00:01"},
        {"INT_A", "Brigade Road / MG Road", "TRAFFIC_NODE", "192.168.1.101", 9001, "192.168.1.100", 9000, "DC:A6:32:01:00:02"},
        {"INT_B", "MG Road / Residency Road", "TRAFFIC_NODE", "192.168.1.102", 9002, "192.168.1.100", 9000, "DC:A6:32:01:00:03"},
        {"INT_C", "Hospital Approach Avenue", "TRAFFIC_NODE", "192.168.1.103", 9003, "192.168.1.100", 9000, "DC:A6:32:01:00:04"}
    };

    m_signalPins.roadARedPin = 17;
    m_signalPins.roadAYellowPin = 27;
    m_signalPins.roadAGreenPin = 22;

    m_signalPins.roadBRedPin = 23;
    m_signalPins.roadBYellowPin = 24;
    m_signalPins.roadBGreenPin = 25;

    m_signalPins.pedWalkPin = 5;
    m_signalPins.pedDontWalkPin = 6;
    m_signalPins.buzzerPin = 18;
    m_signalPins.strobePin = 26;
    m_signalPins.manualOverridePin = 16;

    m_relays = {
        {1, 17, "ROAD_A_RED", false, true},
        {2, 22, "ROAD_A_GREEN", false, false},
        {3, 23, "ROAD_B_RED", false, true},
        {4, 25, "ROAD_B_GREEN", false, false}
    };

    m_irSensors.approachSensorPin = 12;
    m_irSensors.departureSensorPin = 13;
    m_irSensors.debounceMs = 50;
    m_irSensors.stuckTimeoutSec = 30;

    m_timings.normalGreenSec = 22.0;
    m_timings.clearanceYellowSec = 4.0;
    m_timings.allRedHoldSec = 2.0;
    m_timings.pedestrianWalkSec = 7.0;
    m_timings.preemptionMaxHoldSec = 60.0;
    m_timings.failsafeState = "FLASHING_YELLOW";
    m_timings.failsafeFlashRateHz = 1.0;
    m_relayActiveLow = false;
}

bool HardwareConfiguration::LoadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return LoadFromJson(buffer.str());
}

bool HardwareConfiguration::LoadFromJson(const std::string& jsonString) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (jsonString.empty()) {
        return false;
    }

    // Extract timings if present
    auto greenSec = common::JsonHelper::ExtractDouble(jsonString, "normalGreenSec");
    if (greenSec.has_value()) {
        m_timings.normalGreenSec = greenSec.value();
    }
    auto yellowSec = common::JsonHelper::ExtractDouble(jsonString, "clearanceYellowSec");
    if (yellowSec.has_value()) {
        m_timings.clearanceYellowSec = yellowSec.value();
    }
    auto allRedSec = common::JsonHelper::ExtractDouble(jsonString, "allRedHoldSec");
    if (allRedSec.has_value()) {
        m_timings.allRedHoldSec = allRedSec.value();
    }
    auto preemptionSec = common::JsonHelper::ExtractDouble(jsonString, "preemptionMaxHoldSec");
    if (preemptionSec.has_value()) {
        m_timings.preemptionMaxHoldSec = preemptionSec.value();
    }
    auto failsafe = common::JsonHelper::ExtractString(jsonString, "failsafeState");
    if (failsafe.has_value()) {
        m_timings.failsafeState = failsafe.value();
    }

    // Extract relay active low
    auto activeLow = common::JsonHelper::ExtractBool(jsonString, "activeLow");
    if (activeLow.has_value()) {
        m_relayActiveLow = activeLow.value();
        for (auto& r : m_relays) {
            r.activeLow = m_relayActiveLow;
        }
    }

    return true;
}

bool HardwareConfiguration::GetNodeRecord(const std::string& nodeId, WifiNodeRecord& outRecord) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& n : m_nodes) {
        if (n.nodeId == nodeId) {
            outRecord = n;
            return true;
        }
    }
    return false;
}

std::string HardwareConfiguration::ToJson() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string json = "{\n";
    json += "  \"system\": \"SMART_AEGIS_CPS\",\n";
    json += "  \"hardwarePlatform\": \"Raspberry_Pi_4_Model_B\",\n";
    json += "  \"activeLow\": " + std::string(m_relayActiveLow ? "true" : "false") + ",\n";
    json += "  \"normalGreenSec\": " + std::to_string(m_timings.normalGreenSec) + ",\n";
    json += "  \"clearanceYellowSec\": " + std::to_string(m_timings.clearanceYellowSec) + ",\n";
    json += "  \"allRedHoldSec\": " + std::to_string(m_timings.allRedHoldSec) + ",\n";
    json += "  \"preemptionMaxHoldSec\": " + std::to_string(m_timings.preemptionMaxHoldSec) + ",\n";
    json += "  \"failsafeState\": \"" + m_timings.failsafeState + "\"\n";
    json += "}\n";
    return json;
}

} // namespace hardware
} // namespace aegis
