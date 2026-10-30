/**
 * @file GreenWaveCoordinator.h
 * @brief Dynamic Green Wave Coordination Engine for SMART AEGIS (Phase 7B).
 *
 * Ingests an EmergencyRoute (from Phase 7A EmergencyCorridorPlanner) and
 * automatically generates the time-synchronized traffic signal schedule
 * (GreenWavePlan) creating a continuous green corridor for emergency vehicles.
 */

#ifndef AEGIS_GREEN_WAVE_COORDINATOR_H
#define AEGIS_GREEN_WAVE_COORDINATOR_H

#include <string>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <memory>

#include "green_corridor/EmergencyCorridorPlanner.h"

namespace aegis {
namespace green_wave {

// Convenience type aliases from Phase 7A
using EmergencyRoute = aegis::green_corridor::EmergencyRoute;
using IntersectionEta = aegis::green_corridor::IntersectionEta;

/**
 * @brief Scheduled green signal activation window for an intersection.
 */
struct GreenWindow {
    std::string intersectionId{""};
    double startTimeSeconds{0.0};
    double endTimeSeconds{0.0};
    std::string command{"GREEN"};
    double safetyBufferSeconds{3.0};
    double clearanceSeconds{5.0};
    double etaSeconds{0.0};

    GreenWindow() = default;
    GreenWindow(std::string id, double start, double end, std::string cmd = "GREEN",
                double buffer = 3.0, double clearance = 5.0, double eta = 0.0)
        : intersectionId(std::move(id))
        , startTimeSeconds(start)
        , endTimeSeconds(end)
        , command(std::move(cmd))
        , safetyBufferSeconds(buffer)
        , clearanceSeconds(clearance)
        , etaSeconds(eta) {}

    bool operator==(const GreenWindow& other) const noexcept {
        return intersectionId == other.intersectionId &&
               startTimeSeconds == other.startTimeSeconds &&
               endTimeSeconds == other.endTimeSeconds &&
               command == other.command;
    }
};

/**
 * @brief Complete time-coordinated green wave corridor schedule.
 */
struct GreenWavePlan {
    std::vector<GreenWindow> windows{};
    std::string routeId{""};
    std::string ambulanceId{"AMBULANCE"};
    std::string hospitalId{"HOSPITAL"};
    double totalDurationSeconds{0.0};
    bool isValid{false};
};

/**
 * @class GreenWaveCoordinator
 * @brief Thread-safe schedule generation engine creating green wave corridors.
 */
class GreenWaveCoordinator {
public:
    GreenWaveCoordinator();
    GreenWaveCoordinator(double safetyBufferSeconds, double clearanceSeconds);
    ~GreenWaveCoordinator() = default;

    // Non-copyable for robust concurrent ownership
    GreenWaveCoordinator(const GreenWaveCoordinator&) = delete;
    GreenWaveCoordinator& operator=(const GreenWaveCoordinator&) = delete;
    GreenWaveCoordinator(GreenWaveCoordinator&&) noexcept;
    GreenWaveCoordinator& operator=(GreenWaveCoordinator&&) noexcept;

    // =========================================================================
    // Configuration
    // =========================================================================
    void SetSafetyBufferSeconds(double buffer);
    double GetSafetyBufferSeconds() const;

    void SetClearanceSeconds(double clearance);
    double GetClearanceSeconds() const;

    void SetCommand(const std::string& command);
    std::string GetCommand() const;

    void AddNonSignalNode(const std::string& nodeId);
    void RemoveNonSignalNode(const std::string& nodeId);
    void ClearNonSignalNodes();
    bool IsSignalized(const std::string& nodeId, const std::string& hospitalId = "") const;

    // =========================================================================
    // Public APIs: Estimation & Generation
    // =========================================================================
    GreenWindow EstimateGreenWindow(const std::string& intersectionId,
                                    double etaSeconds,
                                    double safetyBufferSeconds,
                                    double clearanceSeconds,
                                    const std::string& command = "GREEN") const;

    bool EstimateGreenWindow(double etaSeconds,
                             double safetyBufferSeconds,
                             double clearanceSeconds,
                             double& outStart,
                             double& outEnd) const;

    bool GenerateGreenWaveSchedule(const EmergencyRoute& route, GreenWavePlan& outPlan);
    GreenWavePlan GenerateGreenWaveSchedule(const EmergencyRoute& route);

    // =========================================================================
    // Plan Accessors & Lifecycle
    // =========================================================================
    GreenWavePlan GetCurrentPlan() const;
    void ClearPlan();

    // =========================================================================
    // Formatting & Structured Logging
    // =========================================================================
    std::string FormatPlan(const GreenWavePlan& plan) const;
    std::string FormatPlan() const;

    static std::string FormatLog(const GreenWavePlan& plan);
    std::string GetLastLog() const;
    void PrintLog(const GreenWavePlan& plan) const;
    void EnableAutoLogging(bool enable);

private:
    mutable std::mutex m_mutex;
    double m_safetyBufferSeconds{3.0};
    double m_clearanceSeconds{5.0};
    std::string m_command{"GREEN"};
    bool m_autoLogging{true};

    std::unordered_set<std::string> m_nonSignalNodes;

    GreenWavePlan m_currentPlan{};
    std::string m_lastLog{""};
};

} // namespace green_wave
} // namespace aegis

#endif // AEGIS_GREEN_WAVE_COORDINATOR_H
