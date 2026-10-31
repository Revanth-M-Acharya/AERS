/**
 * @file RelayController.h
 * @brief SMART AEGIS (AERS) Phase 20 - 4-Channel/8-Channel Optocoupled Relay Controller.
 */

#ifndef AEGIS_HARDWARE_RELAY_CONTROLLER_H
#define AEGIS_HARDWARE_RELAY_CONTROLLER_H

#include "HardwareConfiguration.h"
#include "bsp/RaspberryPi4BSP.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <cstdint>

namespace aegis {
namespace hardware {

/**
 * @brief State and metrics for a single relay output channel.
 */
struct RelayChannelState {
    uint8_t channel{1};
    uint32_t gpioPin{17};
    std::string assignedFunction{"ROAD_A_RED"};
    bool activeLow{false};
    bool isEnergized{false};
    uint64_t switchCount{0};
    bool hasFault{false};
    std::string faultDescription;
};

/**
 * @brief Channel-level result from Relay Power-On Self Test (POST).
 */
struct RelayPostResult {
    uint8_t channel{1};
    uint32_t gpioPin{17};
    std::string assignedFunction{"ROAD_A_RED"};
    bool onTestPassed{false};
    bool offTestPassed{false};
    bool overallPassed{false};
    uint64_t switchTimeUs{0};
    std::string statusText{"PASS"}; // "Relay1 PASS" or "Relay1 FAIL"
    std::string details;
};

/**
 * @brief Complete report from the Relay Power-On Self Test (POST).
 */
struct RelayPostReport {
    std::vector<RelayPostResult> channelResults;
    bool interlockVerified{true};
    bool allPassed{false};
    uint64_t totalDurationUs{0};
    std::string summary;

    std::string GenerateReport() const;
};

/**
 * @brief High-precision relay actuator driving optocoupled relay boards via BCM2711 GPIO.
 */
class RelayController {
public:
    explicit RelayController(std::shared_ptr<bsp::RaspberryPi4BSP> bsp = nullptr);
    ~RelayController();

    // Prevent copy/move
    RelayController(const RelayController&) = delete;
    RelayController& operator=(const RelayController&) = delete;

    // Lifecycle
    bool Initialize(const std::vector<RelayChannelDef>& channels, bool activeLow = false);
    void Shutdown();
    bool IsInitialized() const;

    // Relay Actuation
    bool SetRelay(uint8_t channel, bool energized);
    bool IsRelayEnergized(uint8_t channel) const;
    bool ToggleRelay(uint8_t channel);
    void DeenergizeAll();

    // Diagnostics & Self-Test
    bool RunSelfTest();
    RelayPostReport ExecutePowerOnSelfTest();
    RelayPostReport GetLastPostReport() const;
    uint64_t GetSwitchCycleCount(uint8_t channel) const;
    bool HasFault(uint8_t channel) const;
    std::vector<RelayChannelState> GetAllChannelStates() const;

    // Hardware Readback & Health
    bool VerifyReadback(uint8_t channel);

private:
    mutable std::mutex m_mutex;
    std::shared_ptr<bsp::RaspberryPi4BSP> m_bsp;
    bool m_initialized{false};
    bool m_defaultActiveLow{false};
    std::unordered_map<uint8_t, RelayChannelState> m_channels;
    RelayPostReport m_lastPostReport;
};

} // namespace hardware
} // namespace aegis

#endif // AEGIS_HARDWARE_RELAY_CONTROLLER_H

