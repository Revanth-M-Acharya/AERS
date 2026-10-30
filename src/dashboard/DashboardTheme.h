/**
 * @file DashboardTheme.h
 * @brief SMART AEGIS (AERS) Phase 17 - Control Centre Modern EOC Theme & Styling Engine.
 */

#ifndef AEGIS_DASHBOARD_THEME_H
#define AEGIS_DASHBOARD_THEME_H

#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtCore/QString>

class QWidget;
class QLabel;

class QProgressBar;

namespace aegis {
namespace dashboard {

/**
 * @brief Centralized aesthetic design system and stylesheet provider for EOC command center.
 */
class DashboardTheme {
public:
    // Core Obsidian Dark Palette
    static const QColor ColorBgObsidian;      // #0B0F19 - Main Canvas Background
    static const QColor ColorBgCard;          // #111827 - Surface Card
    static const QColor ColorBgCardHover;     // #1F2937 - Card Hover / Inset
    static const QColor ColorBorder;          // #374151 - Subdued Outline
    static const QColor ColorBorderActive;    // #1E3A8A - Highlighted Blueprint

    // Operational Status Accents
    static const QColor ColorGreen;           // #10B981 - Safe / Active Green Wave
    static const QColor ColorAmber;           // #F59E0B - Caution / Flashing Yellow / P2
    static const QColor ColorRed;             // #EF4444 - Critical Emergency / All-Red / P1
    static const QColor ColorBlue;            // #3B82F6 - Police / Ambulance / Telecom
    static const QColor ColorCyan;            // #06B6D4 - QNX RTOS / Telemetry
    static const QColor ColorPurple;          // #8B5CF6 - Multi-Agency Tactical

    // Typography Accents
    static const QColor ColorTextWhite;       // #F9FAFB - High-contrast text
    static const QColor ColorTextMuted;       // #9CA3AF - Secondary captions
    static const QColor ColorTextDim;         // #6B7280 - Disabled / subtle text

    // Typography
    static QFont MonospaceFont(int pointSize = 9, bool bold = false);
    static QFont TitleFont(int pointSize = 11, bool bold = true);
    static QFont HeaderFont(int pointSize = 13, bool bold = true);
    static QFont MetricFont(int pointSize = 16, bool bold = true);
    static QFont BadgeFont(int pointSize = 8, bool bold = true);
    static QFont BodyFont(int pointSize = 9, bool bold = false);

    // Stylesheet Builders
    static QString GetApplicationStyleSheet();
    static QString CardStyleSheet();
    static QString HeaderBadgeStyleSheet(const QString& colorHex);
    static QString PriorityPillStyleSheet(const QString& priorityLevel);

    // In-place styling helpers
    static void ApplyTheme(QWidget* widget);
    static void ApplyCardStyle(QWidget* widget);
    static void ApplyBadgeStyle(QLabel* label, const QColor& bgColor, const QColor& textColor);
    static void ApplyProgressBarStyle(QProgressBar* bar, const QColor& color);
};

} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_THEME_H
