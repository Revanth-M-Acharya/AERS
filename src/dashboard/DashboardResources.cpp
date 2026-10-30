/**
 * @file DashboardResources.cpp
 * @brief SMART AEGIS (AERS) Phase 17 - Control Centre Resource Cache Implementation.
 */

#include "DashboardResources.h"
#include "DashboardIcons.h"
#include "DashboardTheme.h"

namespace aegis {
namespace dashboard {

DashboardResources& DashboardResources::Instance() {
    static DashboardResources instance;
    return instance;
}

DashboardResources::DashboardResources() = default;

QIcon DashboardResources::GetIcon(const std::string& key, int size) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string cacheKey = key + "_" + std::to_string(size);
    auto it = m_iconCache.find(cacheKey);
    if (it != m_iconCache.end()) {
        return it->second;
    }

    QIcon generated;
    if (key == "ambulance") {
        generated = DashboardIcons::AmbulanceIcon(size, DashboardTheme::ColorRed);
    } else if (key == "hospital") {
        generated = DashboardIcons::HospitalIcon(size, DashboardTheme::ColorBlue);
    } else if (key == "traffic_green") {
        generated = DashboardIcons::TrafficLightIcon(size, DashboardTheme::ColorGreen);
    } else if (key == "traffic_yellow") {
        generated = DashboardIcons::TrafficLightIcon(size, DashboardTheme::ColorAmber);
    } else if (key == "traffic_red") {
        generated = DashboardIcons::TrafficLightIcon(size, DashboardTheme::ColorRed);
    } else if (key == "server") {
        generated = DashboardIcons::NodeServerIcon(size, DashboardTheme::ColorCyan);
    } else if (key == "warning") {
        generated = DashboardIcons::WarningAlertIcon(size, DashboardTheme::ColorAmber);
    } else if (key == "pulse") {
        generated = DashboardIcons::HeartbeatPulseIcon(size, DashboardTheme::ColorGreen);
    } else {
        generated = DashboardIcons::CheckmarkIcon(size, DashboardTheme::ColorGreen);
    }

    m_iconCache[cacheKey] = generated;
    return generated;
}

void DashboardResources::ClearCache() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_iconCache.clear();
}

QIcon DashboardResources::AmbulanceIcon(int size) {
    return GetIcon("ambulance", size);
}

QIcon DashboardResources::HospitalIcon(int size) {
    return GetIcon("hospital", size);
}

QIcon DashboardResources::TrafficGreenIcon(int size) {
    return GetIcon("traffic_green", size);
}

QIcon DashboardResources::TrafficYellowIcon(int size) {
    return GetIcon("traffic_yellow", size);
}

QIcon DashboardResources::TrafficRedIcon(int size) {
    return GetIcon("traffic_red", size);
}

QIcon DashboardResources::ServerNodeIcon(int size) {
    return GetIcon("server", size);
}

QIcon DashboardResources::WarningIcon(int size) {
    return GetIcon("warning", size);
}

QIcon DashboardResources::PulseIcon(int size) {
    return GetIcon("pulse", size);
}

} // namespace dashboard
} // namespace aegis

