/**
 * @file IntersectionRegistry.h
 * @brief Thread-safe registry for Raspberry Pi roadside intersection nodes.
 *
 * Tracks intersection network locations, online/offline status, and
 * liveness heartbeat telemetry in the Central Coordinator.
 */

#ifndef AEGIS_COORDINATOR_INTERSECTION_REGISTRY_H
#define AEGIS_COORDINATOR_INTERSECTION_REGISTRY_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <cstdint>
#include <chrono>

namespace aegis {
namespace coordinator {

/**
 * @brief Metadata record representing an active or provisioned roadside intersection.
 */
struct IntersectionEntry {
    std::string intersectionId{""};          ///< Unique identifier (e.g., "INT_101", "INT_NORTH_01")
    std::string ipAddress{""};               ///< Remote IPv4 address of the Raspberry Pi node
    uint16_t port{0};                        ///< Remote port number
    bool isOnline{false};                    ///< Current connectivity state
    uint64_t lastHeartbeatTimestamp{0};      ///< Epoch timestamp (ms) of the most recent heartbeat
    int clientId{-1};                        ///< Connected communication socket client ID
    std::string metadata{""};                ///< Optional metadata (e.g. latitude/longitude, phase count)

    IntersectionEntry() = default;

    IntersectionEntry(std::string id,
                      std::string ip,
                      uint16_t pt = 0,
                      bool online = true,
                      uint64_t lastHb = 0,
                      int cId = -1,
                      std::string meta = "")
        : intersectionId(std::move(id))
        , ipAddress(std::move(ip))
        , port(pt)
        , isOnline(online)
        , lastHeartbeatTimestamp(lastHb)
        , clientId(cId)
        , metadata(std::move(meta)) {}
};

/**
 * @brief Thread-safe registry for intersection management.
 */
class IntersectionRegistry {
public:
    /**
     * @brief Construct an empty IntersectionRegistry.
     */
    IntersectionRegistry() = default;

    /**
     * @brief Destructor.
     */
    ~IntersectionRegistry() = default;

    // Non-copyable, movable
    IntersectionRegistry(const IntersectionRegistry&) = delete;
    IntersectionRegistry& operator=(const IntersectionRegistry&) = delete;
    IntersectionRegistry(IntersectionRegistry&&) noexcept;
    IntersectionRegistry& operator=(IntersectionRegistry&&) noexcept;

    // =========================================================================
    // Registration Management
    // =========================================================================

    /**
     * @brief Register a new intersection or update an existing intersection record.
     * @param intersectionId Unique intersection identifier.
     * @param ipAddress IPv4 address of the node.
     * @param port Remote port number.
     * @param clientId Connected socket client ID.
     * @return true if registered successfully.
     */
    bool RegisterIntersection(const std::string& intersectionId,
                              const std::string& ipAddress,
                              uint16_t port = 0,
                              int clientId = -1);

    /**
     * @brief Register an intersection using a pre-populated record.
     * @param entry Full record details.
     * @return true if added/updated; false if entry ID is empty.
     */
    bool RegisterIntersection(const IntersectionEntry& entry);

    /**
     * @brief Remove an intersection from the registry.
     * @param intersectionId Target intersection ID to unregister.
     * @return true if intersection existed and was removed; false if not found.
     */
    bool RemoveIntersection(const std::string& intersectionId);

    /**
     * @brief Update the heartbeat timestamp and set intersection status to online.
     * @param intersectionId Target intersection identifier.
     * @param timestamp Unix epoch in milliseconds (0 = use current system clock).
     * @return true if intersection was found and updated; false otherwise.
     */
    bool UpdateHeartbeat(const std::string& intersectionId, uint64_t timestamp = 0);

    /**
     * @brief Retrieve record for a specific intersection by ID.
     * @param intersectionId Target intersection ID.
     * @param outEntry Populated with entry details on success.
     * @return true if found; false otherwise.
     */
    bool GetIntersection(const std::string& intersectionId, IntersectionEntry& outEntry) const;

    /**
     * @brief Retrieve record for a specific intersection as optional.
     * @param intersectionId Target intersection ID.
     * @return std::optional with entry if found, std::nullopt otherwise.
     */
    std::optional<IntersectionEntry> GetIntersection(const std::string& intersectionId) const;

    /**
     * @brief Retrieve snapshot list of all registered intersections.
     * @return Vector containing all intersection entries.
     */
    std::vector<IntersectionEntry> GetAllIntersections() const;

    // =========================================================================
    // Status & Telemetry Queries
    // =========================================================================

    /**
     * @brief Check if an intersection is registered.
     */
    bool HasIntersection(const std::string& intersectionId) const;

    /**
     * @brief Set the online/offline status of an intersection.
     * @param intersectionId Target intersection ID.
     * @param isOnline Connectivity status.
     * @return true if updated; false if not found.
     */
    bool SetOnlineStatus(const std::string& intersectionId, bool isOnline);

    /**
     * @brief Check all intersections for stale heartbeats exceeding timeout interval.
     *
     * Transitions any intersection whose heartbeat is older than @p timeoutIntervalMs
     * to isOnline = false, and returns list of timed-out intersection IDs.
     *
     * @param timeoutIntervalMs Maximum allowed time since last heartbeat in milliseconds.
     * @return Vector of intersection IDs whose heartbeat timed out.
     */
    std::vector<std::string> CheckHeartbeatTimeouts(uint64_t timeoutIntervalMs);

    /**
     * @brief Associate an open communication socket client ID with an intersection.
     */
    bool AssociateClientId(const std::string& intersectionId, int clientId);

    /**
     * @brief Find intersection ID mapped to a specific socket client ID.
     * @param clientId Target communication client ID.
     * @param outIntersectionId Populated with intersection ID if found.
     * @return true if mapping exists; false otherwise.
     */
    bool FindByClientId(int clientId, std::string& outIntersectionId) const;

    /**
     * @brief Total count of registered intersections.
     */
    size_t GetTotalCount() const;

    /**
     * @brief Total count of currently online intersections.
     */
    size_t GetOnlineCount() const;

    /**
     * @brief Clear all registered intersections.
     */
    void Clear();

    /**
     * @brief Helper to get current epoch milliseconds.
     */
    static uint64_t CurrentTimestampMs() noexcept;

private:
    mutable std::mutex m_mutex{};
    std::unordered_map<std::string, IntersectionEntry> m_intersections{};
    std::unordered_map<int, std::string> m_clientIdToIntersectionId{};
};

} // namespace coordinator
} // namespace aegis

#endif // AEGIS_COORDINATOR_INTERSECTION_REGISTRY_H
