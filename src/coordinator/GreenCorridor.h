/**
 * @file GreenCorridor.h
 * @brief Represents an active emergency response green wave corridor.
 *
 * Tracks the ordered route of intersections, the actively preempted intersection,
 * corridor progression, and lifecycle status.
 */

#ifndef AEGIS_COORDINATOR_GREEN_CORRIDOR_H
#define AEGIS_COORDINATOR_GREEN_CORRIDOR_H

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>
#include <cstddef>

namespace aegis {
namespace coordinator {

/**
 * @brief Lifecycle status of an emergency green corridor.
 */
enum class CorridorStatus : uint8_t {
    CREATED   = 0, ///< Route constructed but preemption not yet active
    ACTIVE    = 1, ///< Preemption active and progressing along intersections
    COMPLETED = 2, ///< Ambulance passed destination; preemption cleared
    CANCELLED = 3  ///< Preemptively aborted or re-routed
};

/**
 * @brief Convert CorridorStatus enum to human-readable string.
 */
const char* CorridorStatusToString(CorridorStatus status) noexcept;

/**
 * @brief Convert string name to CorridorStatus enum.
 */
CorridorStatus StringToCorridorStatus(const std::string& str) noexcept;

/**
 * @brief Thread-safe emergency corridor representation.
 */
class GreenCorridor {
public:
    /**
     * @brief Construct an empty GreenCorridor.
     */
    GreenCorridor();

    /**
     * @brief Construct and initialize a GreenCorridor with an ID and route.
     * @param corridorId Unique identifier for this corridor mission.
     * @param intersectionIds Ordered sequence of intersection IDs on the route.
     */
    GreenCorridor(std::string corridorId, std::vector<std::string> intersectionIds);

    ~GreenCorridor() = default;

    // Non-copyable, movable
    GreenCorridor(const GreenCorridor&) = delete;
    GreenCorridor& operator=(const GreenCorridor&) = delete;
    GreenCorridor(GreenCorridor&&) noexcept;
    GreenCorridor& operator=(GreenCorridor&&) noexcept;

    // =========================================================================
    // Corridor Lifecycle Operations
    // =========================================================================

    /**
     * @brief Initialize or reconfigure corridor route and reset to CREATED state.
     * @param corridorId Identifier of the corridor.
     * @param intersectionIds Ordered list of intersection IDs.
     * @return true if valid (non-empty ID and intersections); false otherwise.
     */
    bool CreateCorridor(const std::string& corridorId, const std::vector<std::string>& intersectionIds);

    /**
     * @brief Activate preemption for the corridor, starting at the first intersection.
     * @return true if transitioned from CREATED to ACTIVE; false otherwise.
     */
    bool Activate();

    /**
     * @brief Advance active preemption focus to the subsequent intersection on the route.
     *
     * If the current intersection is the final element, advancing transitions
     * the corridor status to COMPLETED and returns false.
     *
     * @return true if advanced to a valid next intersection; false if end of route reached or not ACTIVE.
     */
    bool AdvanceToNextIntersection();

    /**
     * @brief Complete the corridor upon mission conclusion.
     * @return true if transitioned from ACTIVE to COMPLETED; false otherwise.
     */
    bool Complete();

    /**
     * @brief Abort or cancel the corridor.
     * @return true if transitioned to CANCELLED; false if already COMPLETED.
     */
    bool Cancel();

    // =========================================================================
    // Inspection & Queries
    // =========================================================================

    /**
     * @brief Get ID of the currently active intersection.
     * @return Intersection ID or empty string if corridor has no intersections.
     */
    std::string CurrentIntersection() const;

    /**
     * @brief Get ID of the upcoming intersection following current.
     * @return Next intersection ID or empty string if at final intersection or inactive.
     */
    std::string NextIntersection() const;

    /**
     * @brief Get corridor unique ID.
     */
    std::string GetCorridorId() const;

    /**
     * @brief Get current lifecycle status.
     */
    CorridorStatus GetStatus() const;

    /**
     * @brief Retrieve complete ordered list of intersections along the corridor.
     */
    std::vector<std::string> GetIntersections() const;

    /**
     * @brief Get zero-based index of the currently active intersection.
     */
    size_t GetCurrentIndex() const;

    /**
     * @brief Total number of intersections along the corridor route.
     */
    size_t GetTotalIntersections() const;

    /**
     * @brief Check if corridor is currently ACTIVE.
     */
    bool IsActive() const;

    /**
     * @brief Check if corridor is COMPLETED.
     */
    bool IsCompleted() const;

    /**
     * @brief Check if corridor is CANCELLED.
     */
    bool IsCancelled() const;

    /**
     * @brief Check if corridor is CREATED (pending activation).
     */
    bool IsCreated() const;

    /**
     * @brief Reset corridor to empty initial state.
     */
    void Reset();

private:
    mutable std::mutex m_mutex{};
    std::string m_corridorId{""};
    std::vector<std::string> m_intersections{};
    size_t m_currentIndex{0};
    CorridorStatus m_status{CorridorStatus::CREATED};
};

} // namespace coordinator
} // namespace aegis

#endif // AEGIS_COORDINATOR_GREEN_CORRIDOR_H
