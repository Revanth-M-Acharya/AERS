/**
 * @file DashboardIcons.h
 * @brief SMART AEGIS (AERS) Phase 17 - Programmatic Vector Icon Generator.
 */

#ifndef AEGIS_DASHBOARD_ICONS_H
#define AEGIS_DASHBOARD_ICONS_H

#include <QtGui/QIcon>
#include <QtGui/QPixmap>
#include <QtGui/QColor>

namespace aegis {
namespace dashboard {

/**
 * @brief Zero-external-asset vector icon generator rendering crisp, high-DPI icons.
 */
class DashboardIcons {
public:
    static QIcon AmbulanceIcon(int size = 24, const QColor& color = QColor("#EF4444"));
    static QIcon HospitalIcon(int size = 24, const QColor& color = QColor("#3B82F6"));
    static QIcon TrafficLightIcon(int size = 24, const QColor& color = QColor("#10B981"));
    static QIcon NodeServerIcon(int size = 24, const QColor& color = QColor("#06B6D4"));
    static QIcon WarningAlertIcon(int size = 24, const QColor& color = QColor("#F59E0B"));
    static QIcon CheckmarkIcon(int size = 24, const QColor& color = QColor("#10B981"));
    static QIcon GreenCorridorIcon(int size = 24, const QColor& color = QColor("#10B981"));
    static QIcon HeartbeatPulseIcon(int size = 24, const QColor& color = QColor("#10B981"));
    static QIcon CpuIcon(int size = 16, const QColor& color = QColor("#06B6D4"));
    static QIcon MemoryIcon(int size = 16, const QColor& color = QColor("#3B82F6"));

    // Simulation playback controls
    static QIcon PlayIcon(int size = 16, const QColor& color = QColor("#10B981"));
    static QIcon PauseIcon(int size = 16, const QColor& color = QColor("#F59E0B"));
    static QIcon ResetIcon(int size = 16, const QColor& color = QColor("#EF4444"));
    static QIcon SpeedIcon(int size = 16, const QColor& color = QColor("#3B82F6"));
};

} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_ICONS_H

