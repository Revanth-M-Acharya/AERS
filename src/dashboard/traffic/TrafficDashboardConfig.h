/**
 * @file TrafficDashboardConfig.h
 * @brief SMART AEGIS (AERS) Phase 18 - Traffic Intersection Dashboard Configuration Model.
 */

#ifndef AEGIS_DASHBOARD_TRAFFIC_CONFIG_H
#define AEGIS_DASHBOARD_TRAFFIC_CONFIG_H

#include <string>
#include <cstdint>

namespace aegis {
namespace dashboard {
namespace traffic {

/**
 * @brief Bounding box for the intersection vector map.
 */
struct TrafficMapBounds {
    double minLat{12.9650};
    double maxLat{12.9800};
    double minLon{77.5880};
    double maxLon{77.6050};
    double defaultZoom{1.0};
};

/**
 * @brief Operational alert thresholds for municipal intersections.
 */
struct TrafficThresholds {
    uint32_t queueWarningVehicles{10};
    uint32_t queueCriticalVehicles{20};
    double latencyWarningMs{50.0};
    double cpuWarningPct{80.0};
    double tempWarningCelsius{70.0};
};

/**
 * @brief Visibility flags for individual modular panels.
 */
struct PanelVisibility {
    bool showMap{true};
    bool showSignal{true};
    bool showQueue{true};
    bool showIrSensor{true};
    bool showNodeHealth{true};
    bool showGreenCorridor{true};
    bool showEventLog{true};
    bool showManualControl{true};
};

/**
 * @brief Configuration model for the Traffic Operations Dashboard.
 */
class TrafficDashboardConfig {
public:
    TrafficDashboardConfig();

    static TrafficDashboardConfig DefaultConfig();

    bool LoadFromJsonFile(const std::string& filePath);
    bool SaveToJsonFile(const std::string& filePath) const;
    std::string ToJsonString() const;

    static bool LoadFromFile(const std::string& filePath, TrafficDashboardConfig& cfg) {
        return cfg.LoadFromJsonFile(filePath);
    }

    // Window dimensions
    int windowWidth{1600};
    int windowHeight{980};
    bool fullscreen{false};
    std::string windowTitle{"SMART AEGIS — Traffic Intersection Operations Console (QNX RTOS / RPi4)"};

    // Timing & Intervals
    uint32_t refreshIntervalMs{50};         // 20 Hz
    uint32_t animationIntervalMs{33};       // 30 FPS
    uint32_t heartbeatTimeoutMs{3000};      // 3s
    uint32_t logRetentionLimit{500};

    // Styling & Theme
    std::string themeScheme{"DARK_OBSIDIAN"};
    std::string colorPrimary{"#10B981"};
    std::string colorAccent{"#06B6D4"};
    std::string colorDanger{"#EF4444"};
    std::string colorWarning{"#F59E0B"};
    std::string colorBg{"#0B0F19"};
    std::string colorCard{"#111827"};

    // Panels & Subsystems
    PanelVisibility panels;
    TrafficMapBounds mapBounds;
    TrafficThresholds thresholds;
};

} // namespace traffic
} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_TRAFFIC_CONFIG_H

