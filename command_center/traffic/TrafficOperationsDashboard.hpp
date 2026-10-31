/**
 * @file TrafficOperationsDashboard.hpp
 * @brief Real-time traffic signal operation view monitoring intersection states, modes, queues, and preemption.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <cstdint>

namespace aegis::command_center {

enum class IntersectionControlMode {
    EMERGENCY_CORRIDOR,
    NORMAL,
    ALL_RED,
    FLASHING_YELLOW,
    PRIORITY_LOCK
};

inline std::string IntersectionControlModeToString(IntersectionControlMode mode) {
    switch (mode) {
        case IntersectionControlMode::EMERGENCY_CORRIDOR: return "Emergency Corridor";
        case IntersectionControlMode::NORMAL:             return "Normal";
        case IntersectionControlMode::ALL_RED:            return "All Red";
        case IntersectionControlMode::FLASHING_YELLOW:    return "Flashing Yellow";
        case IntersectionControlMode::PRIORITY_LOCK:      return "Priority Lock";
        default:                                          return "Unknown";
    }
}

struct IntersectionTelemetryView {
    std::string intersectionId;
    std::string intersectionName;
    std::string currentSignal{"ROAD_A_GREEN"}; // "ROAD_A_GREEN", "ROAD_B_GREEN", "YELLOW", "ALL_RED"
    IntersectionControlMode currentMode{IntersectionControlMode::NORMAL};
    std::string currentAmbulanceId{"None"};
    uint32_t vehicleCount{0};
    uint32_t irSensorCount{0};
    uint32_t currentQueue{0};
    uint64_t lastTransitionTimestampMonoMs{0};
};

class TrafficOperationsDashboard {
public:
    TrafficOperationsDashboard();
    ~TrafficOperationsDashboard() = default;

    // Prevent copying
    TrafficOperationsDashboard(const TrafficOperationsDashboard&) = delete;
    TrafficOperationsDashboard& operator=(const TrafficOperationsDashboard&) = delete;

    /**
     * @brief Populate default roadside intersection layout (Int 1, Int 2, Int 3).
     */
    void InitializeDefaultIntersections();

    /**
     * @brief Update telemetry state for an intersection.
     */
    bool UpdateIntersection(const IntersectionTelemetryView& view);

    /**
     * @brief Update signal lamp, mode, and active corridor ambulance.
     */
    bool UpdateSignalState(const std::string& intersectionId,
                           const std::string& signal,
                           IntersectionControlMode mode,
                           const std::string& ambulanceId = "None",
                           uint64_t timestampMs = 0);

    /**
     * @brief Record vehicle detection event from IR sensor.
     */
    bool RecordVehicleDetection(const std::string& intersectionId, uint32_t queueEstimate = 0);

    /**
     * @brief Query single intersection view.
     */
    std::optional<IntersectionTelemetryView> GetIntersection(const std::string& intersectionId) const;

    /**
     * @brief Query all intersections.
     */
    std::vector<IntersectionTelemetryView> GetAllIntersections() const;

    /**
     * @brief Clear all intersections.
     */
    void Clear();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, IntersectionTelemetryView> intersections_;
    std::vector<std::string> displayOrder_;
};

} // namespace aegis::command_center
