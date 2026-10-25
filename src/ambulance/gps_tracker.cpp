/**
 * @file gps_tracker.cpp
 * @brief Implementation of GpsTracker serial NMEA parser, simulation engine, and dead-reckoning.
 */

#include "aegis/edge/gps_tracker.hpp"
#include "aegis/common/geo_utils.hpp"
#include "aegis/common/time_utils.hpp"
#include "aegis/common/constants.hpp"
#include "aegis/logger/logger.hpp"

#include <cmath>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace aegis {
namespace edge {
namespace ambulance {

GpsTracker::GpsTracker() {
    m_latestKinematics.position = {12.971598, 77.594562, 0.0};
    m_latestKinematics.speedKmH = 0.0f;
    m_latestKinematics.bearingDeg = 0.0f;
    m_latestKinematics.hdop = 0.9f;
    m_latestKinematics.fixQuality = 1;
    m_latestKinematics.timestampMs = common::GetUtcMilliseconds();
}

GpsTracker::~GpsTracker() {
    Stop();
}

bool GpsTracker::Initialize(const std::string& serialPortPath, uint32_t baudRate) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_serialPortPath = serialPortPath;
    m_baudRate = baudRate;
    m_lastFixTimestampMs = common::GetUtcMilliseconds();
    m_hasValidFix = true;
    m_latestKinematics.timestampMs = m_lastFixTimestampMs;

    // In a simulation or hardware environment without physical TTY, initialize succeeds
    return true;
}

bool GpsTracker::Start() {
    if (m_isRunning.load()) {
        return true;
    }
    m_isRunning.store(true);
    m_workerThread = std::thread(&GpsTracker::RunLoop, this);
    return true;
}

void GpsTracker::Stop() {
    if (m_isRunning.load()) {
        m_isRunning.store(false);
        if (m_workerThread.joinable()) {
            m_workerThread.join();
        }
    }
}

bool GpsTracker::IsRunning() const noexcept {
    return m_isRunning.load();
}

VehicleKinematics GpsTracker::GetLatestKinematics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_latestKinematics;
}

uint8_t GpsTracker::CalculateNmeaChecksum(std::string_view sentence) {
    size_t start = sentence.find('$');
    if (start == std::string_view::npos) {
        start = sentence.find('!');
    }
    start = (start == std::string_view::npos) ? 0 : start + 1;

    size_t end = sentence.find('*', start);
    if (end == std::string_view::npos) {
        end = sentence.length();
    }

    uint8_t checksum = 0;
    for (size_t i = start; i < end; ++i) {
        checksum ^= static_cast<uint8_t>(sentence[i]);
    }
    return checksum;
}

bool GpsTracker::ValidateNmeaChecksum(std::string_view sentence) {
    size_t starPos = sentence.find('*');
    if (starPos == std::string_view::npos || starPos + 2 >= sentence.length()) {
        return false;
    }

    uint8_t calculated = CalculateNmeaChecksum(sentence);

    std::string hexStr(sentence.substr(starPos + 1, 2));
    char* endPtr = nullptr;
    unsigned long parsed = std::strtoul(hexStr.c_str(), &endPtr, 16);
    if (endPtr == hexStr.c_str() || parsed > 0xFF) {
        return false;
    }

    return calculated == static_cast<uint8_t>(parsed);
}

double GpsTracker::ConvertNmeaCoordinate(std::string_view coordStr, char direction) {
    if (coordStr.empty()) return 0.0;

    size_t dotPos = coordStr.find('.');
    size_t degDigits = 2; // Default for latitude (ddmm.mmmm)
    if (dotPos != std::string_view::npos && dotPos > 2) {
        degDigits = dotPos - 2;
    }

    std::string degPart(coordStr.substr(0, degDigits));
    std::string minPart(coordStr.substr(degDigits));

    double degrees = std::stod(degPart);
    double minutes = std::stod(minPart);
    double result = degrees + (minutes / 60.0);

    if (direction == 'S' || direction == 's' || direction == 'W' || direction == 'w') {
        result = -result;
    }
    return result;
}

bool GpsTracker::ParseNmeaSentence(std::string_view sentence) {
    if (sentence.empty()) return false;

    // Checksum check
    if (sentence.find('*') != std::string_view::npos) {
        if (!ValidateNmeaChecksum(sentence)) {
            return false;
        }
    }

    // Strip $, !, \r, \n, and checksum
    size_t start = (sentence.front() == '$' || sentence.front() == '!') ? 1 : 0;
    size_t end = sentence.find('*');
    if (end == std::string_view::npos) {
        end = sentence.find_first_of("\r\n");
    }
    if (end == std::string_view::npos) {
        end = sentence.length();
    }

    std::string body(sentence.substr(start, end - start));
    std::vector<std::string> tokens;
    std::stringstream ss(body);
    std::string token;
    while (std::getline(ss, token, ',')) {
        tokens.push_back(token);
    }

    if (tokens.empty()) return false;
    const std::string& type = tokens[0];

    std::lock_guard<std::mutex> lock(m_mutex);
    uint64_t now = common::GetUtcMilliseconds();

    // Handle $GPRMC / $GNRMC
    if (type == "GPRMC" || type == "GNRMC") {
        if (tokens.size() < 9) return false;
        char status = tokens[2].empty() ? 'V' : tokens[2][0];
        if (status != 'A') {
            m_hasValidFix = false;
            return false; // Status V = Warning/Invalid
        }

        double lat = ConvertNmeaCoordinate(tokens[3], tokens[4].empty() ? 'N' : tokens[4][0]);
        double lon = ConvertNmeaCoordinate(tokens[5], tokens[6].empty() ? 'E' : tokens[6][0]);
        float speedKnots = tokens[7].empty() ? 0.0f : std::stof(tokens[7]);
        float bearing = tokens[8].empty() ? 0.0f : std::stof(tokens[8]);

        m_latestKinematics.position.latitude = lat;
        m_latestKinematics.position.longitude = lon;
        m_latestKinematics.speedKmH = speedKnots * 1.852f;
        m_latestKinematics.bearingDeg = bearing;
        m_latestKinematics.fixQuality = 1;
        m_latestKinematics.timestampMs = now;
        m_lastFixTimestampMs = now;
        m_hasValidFix = true;

        if (m_callback) {
            m_callback(m_latestKinematics);
        }
        return true;
    }

    // Handle $GPGGA / $GNGGA
    if (type == "GPGGA" || type == "GNGGA") {
        if (tokens.size() < 10) return false;

        uint8_t fix = tokens[6].empty() ? 0 : static_cast<uint8_t>(std::stoi(tokens[6]));
        if (fix == 0) {
            m_hasValidFix = false;
            return false;
        }

        double lat = ConvertNmeaCoordinate(tokens[2], tokens[3].empty() ? 'N' : tokens[3][0]);
        double lon = ConvertNmeaCoordinate(tokens[4], tokens[5].empty() ? 'E' : tokens[5][0]);
        float hdop = tokens[8].empty() ? 1.0f : std::stof(tokens[8]);
        double alt = tokens[9].empty() ? 0.0 : std::stod(tokens[9]);

        m_latestKinematics.position.latitude = lat;
        m_latestKinematics.position.longitude = lon;
        m_latestKinematics.position.altitude = alt;
        m_latestKinematics.fixQuality = fix;
        m_latestKinematics.hdop = hdop;
        m_latestKinematics.timestampMs = now;
        m_lastFixTimestampMs = now;
        m_hasValidFix = true;

        if (m_callback) {
            m_callback(m_latestKinematics);
        }
        return true;
    }

    return false;
}

void GpsTracker::InjectPosition(double latitude, double longitude, float speedKmH, float bearingDeg, double altitude) {
    std::lock_guard<std::mutex> lock(m_mutex);
    uint64_t now = common::GetUtcMilliseconds();
    m_latestKinematics.position.latitude = latitude;
    m_latestKinematics.position.longitude = longitude;
    m_latestKinematics.position.altitude = altitude;
    m_latestKinematics.speedKmH = speedKmH;
    m_latestKinematics.bearingDeg = bearingDeg;
    m_latestKinematics.fixQuality = 1;
    m_latestKinematics.hdop = 0.8f;
    m_latestKinematics.timestampMs = now;
    m_lastFixTimestampMs = now;
    m_hasValidFix = true;

    if (m_callback) {
        m_callback(m_latestKinematics);
    }
}

bool GpsTracker::InjectNmeaSentence(std::string_view nmeaSentence) {
    return ParseNmeaSentence(nmeaSentence);
}

void GpsTracker::SetSimulatedRoute(const std::vector<common::GeoCoordinate>& waypoints, float speedKmH) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_simWaypoints = waypoints;
    m_currentWaypointIndex = 0;
    m_simSpeedKmH = speedKmH;
    if (!m_simWaypoints.empty()) {
        m_latestKinematics.position = m_simWaypoints[0];
        m_latestKinematics.speedKmH = speedKmH;
        m_latestKinematics.timestampMs = common::GetUtcMilliseconds();
        m_lastFixTimestampMs = m_latestKinematics.timestampMs;
        m_hasValidFix = true;
    }
}

bool GpsTracker::UpdateSimulationStep(float deltaSeconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_simWaypoints.empty() || m_currentWaypointIndex >= m_simWaypoints.size() - 1) {
        return false;
    }

    const auto& currentTarget = m_simWaypoints[m_currentWaypointIndex + 1];
    double distMeters = common::CalculateHaversineDistance(m_latestKinematics.position, currentTarget);

    double travelMeters = (static_cast<double>(m_simSpeedKmH) / 3.6) * static_cast<double>(deltaSeconds);

    if (distMeters <= travelMeters || distMeters < 1.0) {
        // Reached waypoint
        m_latestKinematics.position = currentTarget;
        m_currentWaypointIndex++;
        if (m_currentWaypointIndex < m_simWaypoints.size() - 1) {
            m_latestKinematics.bearingDeg = static_cast<float>(
                common::CalculateBearing(m_latestKinematics.position, m_simWaypoints[m_currentWaypointIndex + 1]));
        }
    } else {
        // Interpolate position along bearing
        double bearingDeg = common::CalculateBearing(m_latestKinematics.position, currentTarget);
        m_latestKinematics.bearingDeg = static_cast<float>(bearingDeg);

        double bearingRad = bearingDeg * (M_PI / 180.0);
        double earthRadiusM = 6371000.0;
        double deltaLat = (travelMeters * std::cos(bearingRad)) / earthRadiusM * (180.0 / M_PI);
        double latRad = m_latestKinematics.position.latitude * (M_PI / 180.0);
        double deltaLon = (travelMeters * std::sin(bearingRad)) / (earthRadiusM * std::cos(latRad)) * (180.0 / M_PI);

        m_latestKinematics.position.latitude += deltaLat;
        m_latestKinematics.position.longitude += deltaLon;
    }

    m_latestKinematics.speedKmH = m_simSpeedKmH;
    m_latestKinematics.fixQuality = 1;
    m_latestKinematics.timestampMs = common::GetUtcMilliseconds();
    m_lastFixTimestampMs = m_latestKinematics.timestampMs;
    m_hasValidFix = true;

    if (m_callback) {
        m_callback(m_latestKinematics);
    }
    return true;
}

void GpsTracker::UpdateDeadReckoning(uint64_t currentTimestampMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_lastFixTimestampMs == 0) {
        m_lastFixTimestampMs = currentTimestampMs;
        return;
    }

    uint64_t elapsedMs = (currentTimestampMs > m_lastFixTimestampMs) ? (currentTimestampMs - m_lastFixTimestampMs) : 0;
    if (elapsedMs < 100) return; // At least 100ms for step

    double deltaSec = static_cast<double>(elapsedMs) / 1000.0;
    if (m_latestKinematics.speedKmH > 0.1f) {
        double travelMeters = (static_cast<double>(m_latestKinematics.speedKmH) / 3.6) * deltaSec;
        double bearingRad = static_cast<double>(m_latestKinematics.bearingDeg) * (M_PI / 180.0);
        double earthRadiusM = 6371000.0;

        double deltaLat = (travelMeters * std::cos(bearingRad)) / earthRadiusM * (180.0 / M_PI);
        double latRad = m_latestKinematics.position.latitude * (M_PI / 180.0);
        double deltaLon = (travelMeters * std::sin(bearingRad)) / (earthRadiusM * std::cos(latRad)) * (180.0 / M_PI);

        m_latestKinematics.position.latitude += deltaLat;
        m_latestKinematics.position.longitude += deltaLon;
    }

    // Mark as Dead-Reckoning (fixQuality 6 in NMEA) and increase HDOP
    m_latestKinematics.fixQuality = 6;
    m_latestKinematics.hdop = std::min(10.0f, m_latestKinematics.hdop + static_cast<float>(0.05 * deltaSec));
    m_latestKinematics.timestampMs = currentTimestampMs;
    m_lastFixTimestampMs = currentTimestampMs;

    if (m_callback) {
        m_callback(m_latestKinematics);
    }
}

void GpsTracker::RegisterKinematicsCallback(KinematicsCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callback = std::move(callback);
}

void GpsTracker::RunLoop() {
    // 10 Hz telemetry loop (100 ms period)
    while (m_isRunning.load()) {
        auto startTime = std::chrono::steady_clock::now();

        {
            uint64_t now = common::GetUtcMilliseconds();
            bool needsDeadReckoning = false;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (!m_simWaypoints.empty()) {
                    // Update simulation step if waypoints loaded
                    UpdateSimulationStep(0.1f);
                } else if (!m_hasValidFix || (now > m_lastFixTimestampMs + 1000)) {
                    needsDeadReckoning = true;
                }
            }

            if (needsDeadReckoning) {
                UpdateDeadReckoning(now);
            }
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        if (elapsed < 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100 - elapsed));
        }
    }
}

} // namespace ambulance
} // namespace edge
} // namespace aegis
