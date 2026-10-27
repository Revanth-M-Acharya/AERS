/**
 * @file ConflictResolutionManager.h
 * @brief Dispatcher and command generator for multi-ambulance conflict resolution in SMART AEGIS.
 */

#ifndef AEGIS_SCHEDULER_CONFLICT_RESOLUTION_MANAGER_H
#define AEGIS_SCHEDULER_CONFLICT_RESOLUTION_MANAGER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <cstdint>

#include "scheduler/IntelligentPriorityScheduler.h"
#include "coordinator/CoordinatorManager.h"
#include "communication/Message.h"

namespace aegis {
namespace scheduler {

/**
 * @brief Discrete resolution actions dispatched to approaching emergency vehicles and junctions.
 */
enum class ResolutionAction {
    PROCEED,
    HOLD,
    DELAY,
    RESUME,
    ALTERNATE_ROUTE
};

inline std::string ResolutionActionToString(ResolutionAction action) {
    switch (action) {
        case ResolutionAction::PROCEED:         return "PROCEED";
        case ResolutionAction::HOLD:            return "HOLD";
        case ResolutionAction::DELAY:           return "DELAY";
        case ResolutionAction::RESUME:          return "RESUME";
        case ResolutionAction::ALTERNATE_ROUTE: return "ALTERNATE_ROUTE";
        default:                                return "UNKNOWN";
    }
}

inline ResolutionAction StringToResolutionAction(const std::string& str) {
    if (str == "PROCEED" || str == "proceed") return ResolutionAction::PROCEED;
    if (str == "HOLD" || str == "hold") return ResolutionAction::HOLD;
    if (str == "DELAY" || str == "delay") return ResolutionAction::DELAY;
    if (str == "RESUME" || str == "resume") return ResolutionAction::RESUME;
    if (str == "ALTERNATE_ROUTE" || str == "alternate_route") return ResolutionAction::ALTERNATE_ROUTE;
    return ResolutionAction::PROCEED;
}

/**
 * @brief Command payload directed to an ambulance or intersection node.
 */
struct ResolutionCommand {
    std::string commandId{""};
    std::string ambulanceId{""};
    std::string intersectionId{""};
    ResolutionAction action{ResolutionAction::PROCEED};
    uint32_t delaySeconds{0};
    std::vector<std::string> alternateRoute{};
    std::string reason{""};
    uint64_t timestampEpochMs{0};

    std::string ToString() const;
    std::string ToJson() const;
};

/**
 * @brief Manager generating and dispatching PROCEED, HOLD, DELAY, RESUME, and ALTERNATE_ROUTE commands,
 * with full coordinator integration.
 */
class ConflictResolutionManager {
public:
    static constexpr uint32_t DEFAULT_ALTERNATE_ROUTE_THRESHOLD_SECONDS = 45;

    explicit ConflictResolutionManager(std::shared_ptr<coordinator::CoordinatorManager> coordinator = nullptr);
    ~ConflictResolutionManager() = default;

    // Coordinator Binding
    void SetCoordinator(std::shared_ptr<coordinator::CoordinatorManager> coordinator) noexcept;
    std::shared_ptr<coordinator::CoordinatorManager> GetCoordinator() const noexcept;
    bool HasCoordinator() const noexcept;

    // Threshold Configuration
    uint32_t GetAlternateRouteThresholdSeconds() const noexcept;
    void SetAlternateRouteThresholdSeconds(uint32_t seconds) noexcept;

    // Command Generation from Scheduler Decision
    std::vector<ResolutionCommand> GenerateResolutionCommands(
        const IntelligentScheduleDecision& decision,
        const std::unordered_map<std::string, std::vector<std::string>>& availableAlternateRoutes = {});

    // Explicit Action Creators
    ResolutionCommand GenerateProceedCommand(const std::string& ambulanceId,
                                            const std::string& intersectionId,
                                            const std::string& reason = "Granted green corridor right-of-way");

    ResolutionCommand GenerateHoldCommand(const std::string& ambulanceId,
                                         const std::string& intersectionId,
                                         const std::string& reason = "Hold at junction stop line for higher priority unit");

    ResolutionCommand GenerateDelayCommand(const std::string& ambulanceId,
                                          const std::string& intersectionId,
                                          uint32_t delaySec,
                                          const std::string& reason = "Decelerate or hold to allow safe junction clearance");

    ResolutionCommand GenerateResumeCommand(const std::string& ambulanceId,
                                           const std::string& intersectionId,
                                           const std::string& reason = "Corridor cleared, proceed through junction");

    ResolutionCommand GenerateAlternateRouteCommand(const std::string& ambulanceId,
                                                   const std::string& intersectionId,
                                                   const std::vector<std::string>& alternateRoute,
                                                   const std::string& reason = "Diversion recommended due to junction contention delay");

    // Inspection & Queries
    std::vector<ResolutionCommand> GetCommandHistory() const;
    std::vector<ResolutionCommand> GetCommandsForAmbulance(const std::string& ambulanceId) const;
    std::vector<ResolutionCommand> GetCommandsForIntersection(const std::string& intersectionId) const;
    size_t GetTotalCommandCount() const;

    void Clear();

private:
    static uint64_t CurrentTimeMillis();
    std::string GenerateCommandId(const std::string& prefix);

    mutable std::mutex m_mutex;
    std::shared_ptr<coordinator::CoordinatorManager> m_coordinator{nullptr};
    uint32_t m_alternateRouteThresholdSeconds{DEFAULT_ALTERNATE_ROUTE_THRESHOLD_SECONDS};
    std::vector<ResolutionCommand> m_commandHistory{};
    uint64_t m_commandSequence{0};
};

} // namespace scheduler
} // namespace aegis

#endif // AEGIS_SCHEDULER_CONFLICT_RESOLUTION_MANAGER_H
