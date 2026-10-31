/**
 * @file HardwareConfiguration.h
 * @brief SMART AEGIS (AERS) Phase 20 - Physical Hardware Configuration & Pin/Relay Schema.
 */

#ifndef AEGIS_HARDWARE_CONFIGURATION_H
#define AEGIS_HARDWARE_CONFIGURATION_H

#include <string>
#include <vector>
#include <cstdint>
#include <mutex>

namespace aegis {
namespace hardware {

/**
 * @brief Wireless node configuration record for Wi-Fi static IP networking.
 */
struct WifiNodeRecord {
    std::string nodeId;
    std::string name;
    std::string role; // "COORDINATOR", "TRAFFIC_NODE"
    std::string staticIp{"192.168.1.100"};
    uint16_t port{9000};
    std::string coordinatorIp{"192.168.1.100"};
    uint16_t coordinatorPort{9000};
    std::string macAddress;
};

/**
 * @brief GPIO pin assignments for dual traffic signal modules.
 */
struct DualSignalPins {
    uint32_t roadARedPin{17};
    uint32_t roadAYellowPin{27};
    uint32_t roadAGreenPin{22};

    uint32_t roadBRedPin{23};
    uint32_t roadBYellowPin{24};
    uint32_t roadBGreenPin{25};

    uint32_t pedWalkPin{5};
    uint32_t pedDontWalkPin{6};

    uint32_t buzzerPin{18};
    uint32_t strobePin{26};
    uint32_t manualOverridePin{16};

    std::vector<uint32_t> GetAllPins() const {
        return {roadARedPin, roadAYellowPin, roadAGreenPin,
                roadBRedPin, roadBYellowPin, roadBGreenPin,
                pedWalkPin, pedDontWalkPin, buzzerPin, strobePin, manualOverridePin};
    }
};

/**
 * @brief Definition of an individual relay channel.
 */
struct RelayChannelDef {
    uint8_t channel{1};
    uint32_t gpioPin{17};
    std::string assignedFunction{"ROAD_A_RED"};
    bool activeLow{true};
    bool defaultEnergized{false};
};

/**
 * @brief Optical IR vehicle sensor parameters.
 */
struct IrSensorParameters {
    uint32_t approachSensorPin{12};
    uint32_t departureSensorPin{13};
    uint32_t debounceMs{50};
    uint32_t stuckTimeoutSec{30};
};

/**
 * @brief Signal phase and fail-safe timing parameters.
 */
struct TimingParameters {
    double normalGreenSec{22.0};
    double clearanceYellowSec{4.0};
    double allRedHoldSec{2.0};
    double pedestrianWalkSec{7.0};
    double preemptionMaxHoldSec{60.0};
    std::string failsafeState{"FLASHING_YELLOW"};
    double failsafeFlashRateHz{1.0};
};

/**
 * @brief Hardware configuration container loaded from config/hardware.json.
 */
class HardwareConfiguration {
public:
    HardwareConfiguration();
    ~HardwareConfiguration() = default;

    bool LoadFromFile(const std::string& path = "config/hardware.json");
    bool LoadFromJson(const std::string& jsonString);
    std::string ToJson() const;

    // Getters
    const std::vector<WifiNodeRecord>& GetNodes() const { return m_nodes; }
    bool GetNodeRecord(const std::string& nodeId, WifiNodeRecord& outRecord) const;
    const DualSignalPins& GetSignalPins() const { return m_signalPins; }
    const std::vector<RelayChannelDef>& GetRelays() const { return m_relays; }
    const IrSensorParameters& GetIrSensors() const { return m_irSensors; }
    const TimingParameters& GetTimings() const { return m_timings; }
    bool IsRelayActiveLow() const { return m_relayActiveLow; }

    // Setters
    void SetSignalPins(const DualSignalPins& pins) { m_signalPins = pins; }
    void SetTimings(const TimingParameters& timings) { m_timings = timings; }

private:
    mutable std::mutex m_mutex;
    std::vector<WifiNodeRecord> m_nodes;
    DualSignalPins m_signalPins;
    std::vector<RelayChannelDef> m_relays;
    IrSensorParameters m_irSensors;
    TimingParameters m_timings;
    bool m_relayActiveLow{false};
};

} // namespace hardware
} // namespace aegis

#endif // AEGIS_HARDWARE_CONFIGURATION_H

