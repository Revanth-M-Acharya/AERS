/**
 * @file DashboardResources.h
 * @brief SMART AEGIS (AERS) Phase 17 - Control Centre Resource Cache & Asset Management.
 */

#ifndef AEGIS_DASHBOARD_RESOURCES_H
#define AEGIS_DASHBOARD_RESOURCES_H

#include <QtGui/QIcon>
#include <QtGui/QPixmap>
#include <QtCore/QString>
#include <unordered_map>
#include <string>
#include <mutex>

namespace aegis {
namespace dashboard {

/**
 * @brief High-performance asset cache storing pre-rendered vector graphics and visual alerts.
 */
class DashboardResources {
public:
    static DashboardResources& Instance();

    QIcon GetIcon(const std::string& key, int size = 24);
    void ClearCache();

    // High-level quick accessors
    QIcon AmbulanceIcon(int size = 24);
    QIcon HospitalIcon(int size = 24);
    QIcon TrafficGreenIcon(int size = 24);
    QIcon TrafficYellowIcon(int size = 24);
    QIcon TrafficRedIcon(int size = 24);
    QIcon ServerNodeIcon(int size = 24);
    QIcon WarningIcon(int size = 24);
    QIcon PulseIcon(int size = 24);

private:
    DashboardResources();
    ~DashboardResources() = default;
    DashboardResources(const DashboardResources&) = delete;
    DashboardResources& operator=(const DashboardResources&) = delete;

    std::unordered_map<std::string, QIcon> m_iconCache;
    std::mutex m_mutex;
};

} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_RESOURCES_H

