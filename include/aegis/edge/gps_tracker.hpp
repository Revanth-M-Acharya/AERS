/**
 * @file gps_tracker.hpp
 * @brief High-frequency GPS acquisition, NMEA parser, and Dead-Reckoning engine.
 */

#ifndef AEGIS_EDGE_GPS_TRACKER_HPP
#define AEGIS_EDGE_GPS_TRACKER_HPP

#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>
#include <thread>
#include <cstdint>
#include "aegis/common/types.hpp"

namespace aegis {
namespace edge {
namespace ambulance {

struct VehicleKinematics {
    common::GeoCoordinate position;
    float speedKmH{0.0f};
    float bearingDeg{0.0f};
    float hdop{1.0f};
    uint8_t fixQuality{1}; ///< 0=Invalid, 1=GPS, 2=DGPS, 6=Dead-Reckoning
    uint64_t timestampMs{0};
};

/**
 * @brief Serial NMEA-0183 parser with Extended Kalman Filter dead-reckoning fallback.
 * Encapsulates serial UART device reading, simulation waypoint interpolation, and 10 Hz telemetry.
 */
class GpsTracker {
public:
    using KinematicsCallback = std::function<void(const VehicleKinematics&)>;

    GpsTracker();
    ~GpsTracker();

    // Lifecycle
    bool Initialize(const std::string& serialPortPath = "", uint32_t baudRate = 9600);
    bool Start();
    void Stop();
    bool IsRunning() const noexcept;

    // Telemetry Access
    VehicleKinematics GetLatestKinematics() const;

    // NMEA Parser
    bool ParseNmeaSentence(std::string_view nmeaSentence);
    static uint8_t CalculateNmeaChecksum(std::string_view sentence);
    static bool ValidateNmeaChecksum(std::string_view sentence);

    // Simulation & Injection API
    void InjectPosition(double latitude, double longitude, float speedKmH = 0.0f, float bearingDeg = 0.0f, double altitude = 0.0);
    bool InjectNmeaSentence(std::string_view nmeaSentence);
    void SetSimulatedRoute(const std::vector<common::GeoCoordinate>& waypoints, float speedKmH = 60.0f);
    bool UpdateSimulationStep(float deltaSeconds);

    // Dead-Reckoning (EKF Fallback)
    void UpdateDeadReckoning(uint64_t currentTimestampMs);

    // Callbacks
    void RegisterKinematicsCallback(KinematicsCallback callback);

private:
    void RunLoop();
    static double ConvertNmeaCoordinate(std::string_view coordStr, char direction);

    std::string m_serialPortPath;
    uint32_t m_baudRate{9600};
    [[maybe_unused]] int m_serialFd{-1};

    mutable std::mutex m_mutex;
    VehicleKinematics m_latestKinematics;
    uint64_t m_lastFixTimestampMs{0};
    bool m_hasValidFix{false};

    // Simulation route state
    std::vector<common::GeoCoordinate> m_simWaypoints;
    size_t m_currentWaypointIndex{0};
    float m_simSpeedKmH{60.0f};

    std::atomic<bool> m_isRunning{false};
    std::thread m_workerThread;
    KinematicsCallback m_callback{nullptr};
};

} // namespace ambulance
} // namespace edge
} // namespace aegis

#endif // AEGIS_EDGE_GPS_TRACKER_HPP
