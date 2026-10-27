/**
 * @file TrafficCommandManager.h
 * @brief Factory for generating deterministic traffic control commands.
 *
 * Responsible ONLY for generating structured communication::Message packets.
 * Does NOT perform network transmission directly.
 */

#ifndef AEGIS_COORDINATOR_TRAFFIC_COMMAND_MANAGER_H
#define AEGIS_COORDINATOR_TRAFFIC_COMMAND_MANAGER_H

#include "communication/Message.h"
#include <string>

namespace aegis {
namespace coordinator {

/**
 * @brief Command string constants for intersection preemption and phase control.
 */
namespace TrafficCommands {
    inline constexpr const char* SET_GREEN          = "SET_GREEN";
    inline constexpr const char* SET_RED            = "SET_RED";
    inline constexpr const char* SET_NORMAL         = "SET_NORMAL";
    inline constexpr const char* EMERGENCY_MODE_ON  = "EMERGENCY_MODE_ON";
    inline constexpr const char* EMERGENCY_MODE_OFF = "EMERGENCY_MODE_OFF";
}

/**
 * @brief Factory class for creating intersection command messages.
 */
class TrafficCommandManager {
public:
    /**
     * @brief Construct a TrafficCommandManager with a specific sender ID.
     * @param coordinatorId Sender identifier populated in generated messages (default: "COORDINATOR").
     */
    explicit TrafficCommandManager(std::string coordinatorId = "COORDINATOR");

    ~TrafficCommandManager() = default;

    // Default copy and move
    TrafficCommandManager(const TrafficCommandManager&) = default;
    TrafficCommandManager& operator=(const TrafficCommandManager&) = default;
    TrafficCommandManager(TrafficCommandManager&&) noexcept = default;
    TrafficCommandManager& operator=(TrafficCommandManager&&) noexcept = default;

    // =========================================================================
    // Command Message Factories
    // =========================================================================

    /**
     * @brief Generate a SET_GREEN command message for target intersection.
     * @param intersectionId Target roadside intersection ID.
     * @param durationSec Optional green hold duration in seconds (default: 30).
     * @return Formatted Message of type COMMAND.
     */
    communication::Message CreateGreenCommand(const std::string& intersectionId, uint32_t durationSec = 30) const;

    /**
     * @brief Generate a SET_RED command message for target intersection.
     * @param intersectionId Target roadside intersection ID.
     * @return Formatted Message of type COMMAND.
     */
    communication::Message CreateRedCommand(const std::string& intersectionId) const;

    /**
     * @brief Generate a SET_NORMAL command message restoring default cyclical phasing.
     * @param intersectionId Target roadside intersection ID.
     * @return Formatted Message of type COMMAND.
     */
    communication::Message CreateNormalCommand(const std::string& intersectionId) const;

    /**
     * @brief Generate an EMERGENCY_MODE_ON command message triggering corridor preemption.
     * @param intersectionId Target roadside intersection ID.
     * @param corridorId Optional corridor mission identifier.
     * @return Formatted Message of type COMMAND.
     */
    communication::Message CreateEmergencyOn(const std::string& intersectionId, const std::string& corridorId = "") const;

    /**
     * @brief Generate an EMERGENCY_MODE_OFF command message clearing emergency preemption.
     * @param intersectionId Target roadside intersection ID.
     * @param corridorId Optional corridor mission identifier.
     * @return Formatted Message of type COMMAND.
     */
    communication::Message CreateEmergencyOff(const std::string& intersectionId, const std::string& corridorId = "") const;

    // =========================================================================
    // Parser & Diagnostic Utilities
    // =========================================================================

    /**
     * @brief Parse a command message payload extracting the command action and target intersection.
     * @param message Input message.
     * @param outCommand Populated with command verb (e.g. SET_GREEN).
     * @param outIntersectionId Populated with target intersection ID.
     * @return true if successfully extracted; false on malformed payload.
     */
    static bool ParseCommand(const communication::Message& message,
                             std::string& outCommand,
                             std::string& outIntersectionId);

    const std::string& GetCoordinatorId() const noexcept { return m_coordinatorId; }
    void SetCoordinatorId(std::string coordinatorId) { m_coordinatorId = std::move(coordinatorId); }

private:
    std::string m_coordinatorId{"COORDINATOR"};
};

} // namespace coordinator
} // namespace aegis

#endif // AEGIS_COORDINATOR_TRAFFIC_COMMAND_MANAGER_H
