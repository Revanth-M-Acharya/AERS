/**
 * @file DashboardTheme.cpp
 * @brief SMART AEGIS (AERS) Phase 17 - Control Centre Modern EOC Theme Implementation.
 */

#include "DashboardTheme.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>

namespace aegis {
namespace dashboard {

const QColor DashboardTheme::ColorBgObsidian   = QColor(11, 15, 25);     // #0B0F19
const QColor DashboardTheme::ColorBgCard       = QColor(17, 24, 39);     // #111827
const QColor DashboardTheme::ColorBgCardHover  = QColor(31, 41, 55);     // #1F2937
const QColor DashboardTheme::ColorBorder       = QColor(55, 65, 81);     // #374151
const QColor DashboardTheme::ColorBorderActive = QColor(30, 58, 138);    // #1E3A8A

const QColor DashboardTheme::ColorGreen        = QColor(16, 185, 129);   // #10B981
const QColor DashboardTheme::ColorAmber        = QColor(245, 158, 11);   // #F59E0B
const QColor DashboardTheme::ColorRed          = QColor(239, 68, 68);    // #EF4444
const QColor DashboardTheme::ColorBlue         = QColor(59, 130, 246);   // #3B82F6
const QColor DashboardTheme::ColorCyan         = QColor(6, 182, 212);    // #06B6D4
const QColor DashboardTheme::ColorPurple       = QColor(139, 92, 246);   // #8B5CF6

const QColor DashboardTheme::ColorTextWhite    = QColor(249, 250, 251);  // #F9FAFB
const QColor DashboardTheme::ColorTextMuted    = QColor(156, 163, 175);  // #9CA3AF
const QColor DashboardTheme::ColorTextDim      = QColor(107, 114, 128);  // #6B7280

QFont DashboardTheme::MonospaceFont(int pointSize, bool bold) {
    QFont font("Menlo", pointSize);
    font.setStyleHint(QFont::Monospace);
    font.setBold(bold);
    return font;
}

QFont DashboardTheme::TitleFont(int pointSize, bool bold) {
    QFont font("Helvetica Neue", pointSize);
    font.setStyleHint(QFont::SansSerif);
    font.setBold(bold);
    return font;
}

QFont DashboardTheme::HeaderFont(int pointSize, bool bold) {
    QFont font("Helvetica Neue", pointSize);
    font.setStyleHint(QFont::SansSerif);
    font.setBold(bold);
    return font;
}

QFont DashboardTheme::MetricFont(int pointSize, bool bold) {
    QFont font("Helvetica Neue", pointSize);
    font.setStyleHint(QFont::SansSerif);
    font.setBold(bold);
    return font;
}

QFont DashboardTheme::BadgeFont(int pointSize, bool bold) {
    QFont font("Helvetica Neue", pointSize);
    font.setStyleHint(QFont::SansSerif);
    font.setBold(bold);
    return font;
}

QFont DashboardTheme::BodyFont(int pointSize, bool bold) {
    QFont font("Helvetica Neue", pointSize);
    font.setStyleHint(QFont::SansSerif);
    font.setBold(bold);
    return font;
}

QString DashboardTheme::CardStyleSheet() {
    return "background-color: #111827; "
           "border: 1px solid #374151; "
           "border-radius: 6px;";
}

QString DashboardTheme::HeaderBadgeStyleSheet(const QString& colorHex) {
    return QString("background-color: rgba(%1, 0.15); "
                   "border: 1px solid %1; "
                   "color: %1; "
                   "border-radius: 4px; "
                   "padding: 2px 8px; "
                   "font-weight: bold;").arg(colorHex);
}

QString DashboardTheme::PriorityPillStyleSheet(const QString& priorityLevel) {
    if (priorityLevel == "P1_CRITICAL" || priorityLevel == "CRITICAL" || priorityLevel == "P1") {
        return "background-color: #7F1D1D; color: #FCA5A5; border: 1px solid #EF4444; border-radius: 4px; padding: 2px 6px; font-weight: bold;";
    } else if (priorityLevel == "P2_HIGH" || priorityLevel == "HIGH" || priorityLevel == "P2") {
        return "background-color: #78350F; color: #FDE68A; border: 1px solid #F59E0B; border-radius: 4px; padding: 2px 6px; font-weight: bold;";
    } else if (priorityLevel == "P3_MODERATE" || priorityLevel == "MODERATE" || priorityLevel == "P3") {
        return "background-color: #1E3A8A; color: #BFDBFE; border: 1px solid #3B82F6; border-radius: 4px; padding: 2px 6px; font-weight: bold;";
    } else {
        return "background-color: #064E3B; color: #A7F3D0; border: 1px solid #10B981; border-radius: 4px; padding: 2px 6px; font-weight: bold;";
    }
}

QString DashboardTheme::GetApplicationStyleSheet() {
    return R"(
        QMainWindow {
            background-color: #0B0F19;
            color: #F9FAFB;
        }

        QWidget {
            background-color: transparent;
            color: #F9FAFB;
            font-family: 'Helvetica Neue', 'Segoe UI', -apple-system, sans-serif;
            font-size: 11px;
        }

        QFrame#cardFrame {
            background-color: #111827;
            border: 1px solid #374151;
            border-radius: 6px;
        }

        QTableWidget, QTableView {
            background-color: #111827;
            border: 1px solid #374151;
            gridline-color: #1F2937;
            color: #F9FAFB;
            selection-background-color: #1E3A8A;
            selection-color: #FFFFFF;
            border-radius: 4px;
        }

        QHeaderView::section {
            background-color: #1F2937;
            color: #9CA3AF;
            padding: 4px 8px;
            border: none;
            border-bottom: 1px solid #374151;
            font-weight: bold;
            font-size: 10px;
            text-transform: uppercase;
        }

        QPushButton {
            background-color: #1F2937;
            border: 1px solid #374151;
            color: #F9FAFB;
            border-radius: 4px;
            padding: 5px 12px;
            font-weight: 600;
        }

        QPushButton:hover {
            background-color: #374151;
            border-color: #4B5563;
        }

        QPushButton:pressed {
            background-color: #111827;
        }

        QPushButton:disabled {
            background-color: #111827;
            color: #6B7280;
            border-color: #1F2937;
        }

        QPushButton#btnActionRed {
            background-color: #7F1D1D;
            border: 1px solid #EF4444;
            color: #FEE2E2;
        }

        QPushButton#btnActionRed:hover {
            background-color: #991B1B;
        }

        QPushButton#btnActionGreen {
            background-color: #064E3B;
            border: 1px solid #10B981;
            color: #D1FAE5;
        }

        QPushButton#btnActionGreen:hover {
            background-color: #065F46;
        }

        QLineEdit, QTextEdit {
            background-color: #111827;
            border: 1px solid #374151;
            color: #F9FAFB;
            border-radius: 4px;
            padding: 4px 8px;
        }

        QLineEdit:focus, QTextEdit:focus {
            border: 1px solid #3B82F6;
        }

        QScrollBar:vertical {
            background: #0B0F19;
            width: 8px;
            margin: 0px;
        }

        QScrollBar::handle:vertical {
            background: #374151;
            min-height: 20px;
            border-radius: 4px;
        }

        QScrollBar::handle:vertical:hover {
            background: #4B5563;
        }

        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }

        QScrollBar:horizontal {
            background: #0B0F19;
            height: 8px;
            margin: 0px;
        }

        QScrollBar::handle:horizontal {
            background: #374151;
            min-width: 20px;
            border-radius: 4px;
        }

        QScrollBar::handle:horizontal:hover {
            background: #4B5563;
        }

        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            height: 0px;
        }

        QProgressBar {
            border: 1px solid #374151;
            border-radius: 4px;
            background-color: #1F2937;
            text-align: center;
            color: #F9FAFB;
            font-size: 9px;
            font-weight: bold;
        }

        QProgressBar::chunk {
            background-color: #10B981;
            border-radius: 3px;
        }

        QTabWidget::pane {
            border: 1px solid #374151;
            background-color: #111827;
            border-radius: 4px;
        }

        QTabBar::tab {
            background-color: #1F2937;
            color: #9CA3AF;
            padding: 6px 14px;
            margin-right: 2px;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            font-weight: 600;
        }

        QTabBar::tab:selected {
            background-color: #111827;
            color: #F9FAFB;
            border-bottom: 2px solid #06B6D4;
        }

        QComboBox {
            background-color: #1F2937;
            border: 1px solid #374151;
            border-radius: 4px;
            padding: 4px 8px;
            color: #F9FAFB;
        }

        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
    )";
}

void DashboardTheme::ApplyCardStyle(QWidget* widget) {
    if (widget) {
        widget->setStyleSheet(CardStyleSheet());
    }
}

void DashboardTheme::ApplyBadgeStyle(QLabel* label, const QColor& bgColor, const QColor& textColor) {
    if (label) {
        label->setStyleSheet(QString("background-color: %1; color: %2; border-radius: 4px; padding: 2px 6px; font-weight: bold;")
                             .arg(bgColor.name(), textColor.name()));
    }
}

void DashboardTheme::ApplyTheme(QWidget* widget) {
    if (widget) {
        widget->setStyleSheet(GetApplicationStyleSheet());
    }
}

void DashboardTheme::ApplyProgressBarStyle(QProgressBar* bar, const QColor& color) {
    if (bar) {
        bar->setStyleSheet(QString(
            "QProgressBar { background-color: #1F2937; border: 1px solid #374151; border-radius: 4px; }"
            "QProgressBar::chunk { background-color: %1; border-radius: 3px; }"
        ).arg(color.name()));
    }
}

} // namespace dashboard
} // namespace aegis
