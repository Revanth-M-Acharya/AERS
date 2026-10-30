/**
 * @file TrafficDashboardConfig.cpp
 * @brief SMART AEGIS (AERS) Phase 18 - Traffic Intersection Dashboard Configuration Model Implementation.
 */

#include "TrafficDashboardConfig.h"
#include <fstream>
#include <sstream>

namespace aegis {
namespace dashboard {
namespace traffic {

TrafficDashboardConfig::TrafficDashboardConfig() = default;

TrafficDashboardConfig TrafficDashboardConfig::DefaultConfig() {
    return TrafficDashboardConfig();
}

bool TrafficDashboardConfig::LoadFromJsonFile(const std::string& filePath) {
    std::ifstream ifs(filePath);
    if (!ifs.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << ifs.rdbuf();
    std::string content = buffer.str();

    // Helper lambda to extract string values between quotes after a key
    auto extractString = [&content](const std::string& key) -> std::string {
        size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return "";
        size_t colon = content.find(':', pos);
        if (colon == std::string::npos) return "";
        size_t firstQuote = content.find('"', colon);
        if (firstQuote == std::string::npos) return "";
        size_t secondQuote = content.find('"', firstQuote + 1);
        if (secondQuote == std::string::npos) return "";
        return content.substr(firstQuote + 1, secondQuote - firstQuote - 1);
    };

    // Helper lambda to extract numeric / integer values
    auto extractNumber = [&content](const std::string& key) -> double {
        size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return 0.0;
        size_t colon = content.find(':', pos);
        if (colon == std::string::npos) return 0.0;
        size_t start = content.find_first_of("-0123456789.", colon);
        if (start == std::string::npos) return 0.0;
        size_t end = content.find_first_not_of("-0123456789.", start);
        std::string numStr = (end == std::string::npos) ? content.substr(start) : content.substr(start, end - start);
        try {
            return std::stod(numStr);
        } catch (...) {
            return 0.0;
        }
    };

    // Helper lambda to extract booleans
    auto extractBool = [&content](const std::string& key, bool defaultVal) -> bool {
        size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos) return defaultVal;
        size_t colon = content.find(':', pos);
        if (colon == std::string::npos) return defaultVal;
        size_t valPos = content.find_first_of("tfTF", colon);
        if (valPos == std::string::npos) return defaultVal;
        return (content[valPos] == 't' || content[valPos] == 'T');
    };

    // Window settings
    double w = extractNumber("width");
    if (w > 0.0) windowWidth = static_cast<int>(w);
    double h = extractNumber("height");
    if (h > 0.0) windowHeight = static_cast<int>(h);
    fullscreen = extractBool("fullscreen", fullscreen);

    std::string title = extractString("title");
    if (!title.empty()) windowTitle = title;

    // Runtime
    double rInt = extractNumber("refreshIntervalMs");
    if (rInt > 0.0) refreshIntervalMs = static_cast<uint32_t>(rInt);
    double aInt = extractNumber("animationIntervalMs");
    if (aInt > 0.0) animationIntervalMs = static_cast<uint32_t>(aInt);
    double hbInt = extractNumber("heartbeatTimeoutMs");
    if (hbInt > 0.0) heartbeatTimeoutMs = static_cast<uint32_t>(hbInt);
    double logLim = extractNumber("logRetentionLimit");
    if (logLim > 0.0) logRetentionLimit = static_cast<uint32_t>(logLim);

    // Theme
    std::string theme = extractString("colorScheme");
    if (!theme.empty()) themeScheme = theme;
    std::string prim = extractString("primaryColor");
    if (!prim.empty()) colorPrimary = prim;
    std::string acc = extractString("accentColor");
    if (!acc.empty()) colorAccent = acc;
    std::string dng = extractString("dangerColor");
    if (!dng.empty()) colorDanger = dng;
    std::string wrn = extractString("warningColor");
    if (!wrn.empty()) colorWarning = wrn;
    std::string bg = extractString("backgroundColor");
    if (!bg.empty()) colorBg = bg;
    std::string card = extractString("cardColor");
    if (!card.empty()) colorCard = card;

    // Panels
    panels.showMap = extractBool("intersectionMap", panels.showMap);
    panels.showSignal = extractBool("trafficSignal", panels.showSignal);
    panels.showQueue = extractBool("queueMonitor", panels.showQueue);
    panels.showIrSensor = extractBool("irSensor", panels.showIrSensor);
    panels.showNodeHealth = extractBool("nodeHealth", panels.showNodeHealth);
    panels.showGreenCorridor = extractBool("greenCorridor", panels.showGreenCorridor);
    panels.showEventLog = extractBool("eventLog", panels.showEventLog);
    panels.showManualControl = extractBool("manualControl", panels.showManualControl);

    // Map bounds
    double minLat = extractNumber("minLatitude");
    if (minLat != 0.0) mapBounds.minLat = minLat;
    double maxLat = extractNumber("maxLatitude");
    if (maxLat != 0.0) mapBounds.maxLat = maxLat;
    double minLon = extractNumber("minLongitude");
    if (minLon != 0.0) mapBounds.minLon = minLon;
    double maxLon = extractNumber("maxLongitude");
    if (maxLon != 0.0) mapBounds.maxLon = maxLon;
    double zoom = extractNumber("defaultZoom");
    if (zoom > 0.0) mapBounds.defaultZoom = zoom;

    // Thresholds
    double qWarn = extractNumber("queueLengthWarning");
    if (qWarn > 0.0) thresholds.queueWarningVehicles = static_cast<uint32_t>(qWarn);
    double qCrit = extractNumber("queueLengthCritical");
    if (qCrit > 0.0) thresholds.queueCriticalVehicles = static_cast<uint32_t>(qCrit);
    double latWarn = extractNumber("latencyWarningMs");
    if (latWarn > 0.0) thresholds.latencyWarningMs = latWarn;
    double cpuWarn = extractNumber("cpuWarningPct");
    if (cpuWarn > 0.0) thresholds.cpuWarningPct = cpuWarn;
    double tempWarn = extractNumber("temperatureWarningC");
    if (tempWarn > 0.0) thresholds.tempWarningCelsius = tempWarn;

    return true;
}

std::string TrafficDashboardConfig::ToJsonString() const {
    std::ostringstream oss;
    oss << "{\n"
        << "  \"window\": {\n"
        << "    \"width\": " << windowWidth << ",\n"
        << "    \"height\": " << windowHeight << ",\n"
        << "    \"fullscreen\": " << (fullscreen ? "true" : "false") << ",\n"
        << "    \"title\": \"" << windowTitle << "\"\n"
        << "  },\n"
        << "  \"runtime\": {\n"
        << "    \"refreshIntervalMs\": " << refreshIntervalMs << ",\n"
        << "    \"animationIntervalMs\": " << animationIntervalMs << ",\n"
        << "    \"heartbeatTimeoutMs\": " << heartbeatTimeoutMs << ",\n"
        << "    \"logRetentionLimit\": " << logRetentionLimit << "\n"
        << "  },\n"
        << "  \"theme\": {\n"
        << "    \"colorScheme\": \"" << themeScheme << "\",\n"
        << "    \"primaryColor\": \"" << colorPrimary << "\",\n"
        << "    \"accentColor\": \"" << colorAccent << "\",\n"
        << "    \"dangerColor\": \"" << colorDanger << "\",\n"
        << "    \"warningColor\": \"" << colorWarning << "\",\n"
        << "    \"backgroundColor\": \"" << colorBg << "\",\n"
        << "    \"cardColor\": \"" << colorCard << "\"\n"
        << "  },\n"
        << "  \"panels\": {\n"
        << "    \"intersectionMap\": " << (panels.showMap ? "true" : "false") << ",\n"
        << "    \"trafficSignal\": " << (panels.showSignal ? "true" : "false") << ",\n"
        << "    \"queueMonitor\": " << (panels.showQueue ? "true" : "false") << ",\n"
        << "    \"irSensor\": " << (panels.showIrSensor ? "true" : "false") << ",\n"
        << "    \"nodeHealth\": " << (panels.showNodeHealth ? "true" : "false") << ",\n"
        << "    \"greenCorridor\": " << (panels.showGreenCorridor ? "true" : "false") << ",\n"
        << "    \"eventLog\": " << (panels.showEventLog ? "true" : "false") << ",\n"
        << "    \"manualControl\": " << (panels.showManualControl ? "true" : "false") << "\n"
        << "  },\n"
        << "  \"map\": {\n"
        << "    \"defaultZoom\": " << mapBounds.defaultZoom << ",\n"
        << "    \"minLatitude\": " << mapBounds.minLat << ",\n"
        << "    \"maxLatitude\": " << mapBounds.maxLat << ",\n"
        << "    \"minLongitude\": " << mapBounds.minLon << ",\n"
        << "    \"maxLongitude\": " << mapBounds.maxLon << "\n"
        << "  },\n"
        << "  \"thresholds\": {\n"
        << "    \"queueLengthWarning\": " << thresholds.queueWarningVehicles << ",\n"
        << "    \"queueLengthCritical\": " << thresholds.queueCriticalVehicles << ",\n"
        << "    \"latencyWarningMs\": " << thresholds.latencyWarningMs << ",\n"
        << "    \"cpuWarningPct\": " << thresholds.cpuWarningPct << ",\n"
        << "    \"temperatureWarningC\": " << thresholds.tempWarningCelsius << "\n"
        << "  }\n"
        << "}\n";
    return oss.str();
}

bool TrafficDashboardConfig::SaveToJsonFile(const std::string& filePath) const {
    std::ofstream ofs(filePath);
    if (!ofs.is_open()) return false;
    ofs << ToJsonString();
    return true;
}

} // namespace traffic
} // namespace dashboard
} // namespace aegis

