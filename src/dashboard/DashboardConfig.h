/**
 * @file DashboardConfig.h
 * @brief SMART AEGIS (AERS) Phase 17 - Control Centre Dashboard Configuration Model.
 */

#ifndef AEGIS_DASHBOARD_CONFIG_H
#define AEGIS_DASHBOARD_CONFIG_H

#include <string>
#include <cstdint>

namespace aegis {
namespace dashboard {

/**
 * @brief Geographic bounding box for city map projection.
 */
struct MapBoundingBox {
    double minLatitude{12.9650};
    double maxLatitude{12.9800};
    double minLongitude{77.5880};
    double maxLongitude{77.6050};
};

/**
 * @brief Operational simulation speed settings.
 */
enum class SimulationSpeed : uint8_t {
    SPEED_1X = 1,
    SPEED_2X = 2,
    SPEED_5X = 5,
    SPEED_10X = 10
};

/**
 * @brief Comprehensive configuration parameters for the Control Centre Dashboard.
 */
class DashboardConfig {
public:
    DashboardConfig();

    static DashboardConfig DefaultCoordinatorConfig();

    bool LoadFromJsonFile(const std::string& filePath);
    bool SaveToJsonFile(const std::string& filePath) const;
    std::string ToJsonString() const;

    static bool LoadFromFile(const std::string& filePath, DashboardConfig& cfg) {
        return cfg.LoadFromJsonFile(filePath);
    }

    // Window dimensions & display
    int windowWidth{1600};
    int windowHeight{960};
    bool startFullscreen{false};
    bool fullscreen{false};
    bool highDpiEnabled{true};

    // Refresh & animation intervals
    uint32_t uiRefreshIntervalMs{50};         // 20 Hz UI refresh
    uint32_t targetRefreshRateHz{20};         // 20 Hz
    double simulationSpeed{1.0};              // 1.0x
    uint32_t mapAnimationIntervalMs{33};      // ~30 FPS map animation
    uint32_t heartbeatTimeoutMs{3000};        // 3 sec node eviction threshold
    uint32_t logRetentionLimit{500};          // Max event log rows

    // Networking
    std::string coordinatorHost{"127.0.0.1"};
    uint16_t coordinatorPort{22060};

    // Geographic viewport
    MapBoundingBox mapBounds;

    // Simulation playback
    SimulationSpeed simSpeed{SimulationSpeed::SPEED_1X};
    bool isPaused{false};

    // Operational features
    bool audioAlertsEnabled{true};
    bool autoScrollEventLog{true};
    bool animatedPipelinesEnabled{true};
};

} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_CONFIG_H
