/**
 * @file DashboardConfig.cpp
 * @brief SMART AEGIS (AERS) Phase 17 - Control Centre Dashboard Configuration Implementation.
 */

#include "DashboardConfig.h"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace aegis {
namespace dashboard {

DashboardConfig::DashboardConfig() = default;

DashboardConfig DashboardConfig::DefaultCoordinatorConfig() {
    DashboardConfig cfg;
    cfg.windowWidth = 1600;
    cfg.windowHeight = 960;
    cfg.startFullscreen = false;
    cfg.uiRefreshIntervalMs = 50;
    cfg.mapAnimationIntervalMs = 33;
    cfg.coordinatorHost = "127.0.0.1";
    cfg.coordinatorPort = 22060;
    cfg.mapBounds.minLatitude = 12.9650;
    cfg.mapBounds.maxLatitude = 12.9800;
    cfg.mapBounds.minLongitude = 77.5880;
    cfg.mapBounds.maxLongitude = 77.6050;
    cfg.simSpeed = SimulationSpeed::SPEED_1X;
    cfg.isPaused = false;
    cfg.audioAlertsEnabled = true;
    cfg.autoScrollEventLog = true;
    cfg.animatedPipelinesEnabled = true;
    return cfg;
}

std::string DashboardConfig::ToJsonString() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"windowWidth\": " << windowWidth << ",\n";
    oss << "  \"windowHeight\": " << windowHeight << ",\n";
    oss << "  \"startFullscreen\": " << (startFullscreen ? "true" : "false") << ",\n";
    oss << "  \"highDpiEnabled\": " << (highDpiEnabled ? "true" : "false") << ",\n";
    oss << "  \"uiRefreshIntervalMs\": " << uiRefreshIntervalMs << ",\n";
    oss << "  \"mapAnimationIntervalMs\": " << mapAnimationIntervalMs << ",\n";
    oss << "  \"heartbeatTimeoutMs\": " << heartbeatTimeoutMs << ",\n";
    oss << "  \"logRetentionLimit\": " << logRetentionLimit << ",\n";
    oss << "  \"coordinatorHost\": \"" << coordinatorHost << "\",\n";
    oss << "  \"coordinatorPort\": " << coordinatorPort << ",\n";
    oss << "  \"mapBounds\": {\n";
    oss << "    \"minLatitude\": " << std::fixed << std::setprecision(6) << mapBounds.minLatitude << ",\n";
    oss << "    \"maxLatitude\": " << std::fixed << std::setprecision(6) << mapBounds.maxLatitude << ",\n";
    oss << "    \"minLongitude\": " << std::fixed << std::setprecision(6) << mapBounds.minLongitude << ",\n";
    oss << "    \"maxLongitude\": " << std::fixed << std::setprecision(6) << mapBounds.maxLongitude << "\n";
    oss << "  },\n";
    oss << "  \"simSpeed\": " << static_cast<int>(simSpeed) << ",\n";
    oss << "  \"isPaused\": " << (isPaused ? "true" : "false") << ",\n";
    oss << "  \"audioAlertsEnabled\": " << (audioAlertsEnabled ? "true" : "false") << ",\n";
    oss << "  \"autoScrollEventLog\": " << (autoScrollEventLog ? "true" : "false") << ",\n";
    oss << "  \"animatedPipelinesEnabled\": " << (animatedPipelinesEnabled ? "true" : "false") << "\n";
    oss << "}\n";
    return oss.str();
}

bool DashboardConfig::LoadFromJsonFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("\"windowWidth\"") != std::string::npos) {
            auto pos = line.find(':');
            if (pos != std::string::npos) windowWidth = std::stoi(line.substr(pos + 1));
        } else if (line.find("\"windowHeight\"") != std::string::npos) {
            auto pos = line.find(':');
            if (pos != std::string::npos) windowHeight = std::stoi(line.substr(pos + 1));
        } else if (line.find("\"coordinatorHost\"") != std::string::npos) {
            auto firstQuote = line.find('"', line.find(':'));
            if (firstQuote != std::string::npos) {
                auto secondQuote = line.find('"', firstQuote + 1);
                if (secondQuote != std::string::npos) {
                    coordinatorHost = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
                }
            }
        } else if (line.find("\"coordinatorPort\"") != std::string::npos) {
            auto pos = line.find(':');
            if (pos != std::string::npos) coordinatorPort = static_cast<uint16_t>(std::stoi(line.substr(pos + 1)));
        } else if (line.find("\"minLatitude\"") != std::string::npos) {
            auto pos = line.find(':');
            if (pos != std::string::npos) mapBounds.minLatitude = std::stod(line.substr(pos + 1));
        } else if (line.find("\"maxLatitude\"") != std::string::npos) {
            auto pos = line.find(':');
            if (pos != std::string::npos) mapBounds.maxLatitude = std::stod(line.substr(pos + 1));
        } else if (line.find("\"minLongitude\"") != std::string::npos) {
            auto pos = line.find(':');
            if (pos != std::string::npos) mapBounds.minLongitude = std::stod(line.substr(pos + 1));
        } else if (line.find("\"maxLongitude\"") != std::string::npos) {
            auto pos = line.find(':');
            if (pos != std::string::npos) mapBounds.maxLongitude = std::stod(line.substr(pos + 1));
        }
    }
    return true;
}

bool DashboardConfig::SaveToJsonFile(const std::string& filePath) const {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    file << ToJsonString();
    return true;
}

} // namespace dashboard
} // namespace aegis

