/**
 * @file TrafficNotificationPanel.cpp
 * @brief SMART AEGIS (AERS) Phase 18 - Real-Time Traffic Operations Notification Banner Panel Implementation.
 */

#include "TrafficNotificationPanel.h"
#include "dashboard/DashboardTheme.h"
#include "dashboard/DashboardIcons.h"
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>

namespace aegis {
namespace dashboard {
namespace traffic {

TrafficNotificationPanel::TrafficNotificationPanel(QWidget* parent)
    : QFrame(parent)
{
    SetupUi();
}

void TrafficNotificationPanel::SetupUi() {
    setObjectName("trafficNotificationPanel");
    setFrameShape(QFrame::StyledPanel);
    setFixedWidth(380);
    hide();

    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 10, 12, 10);
    mainLayout->setSpacing(10);

    m_lblIcon = new QLabel(this);
    m_lblIcon->setFixedSize(24, 24);
    mainLayout->addWidget(m_lblIcon, 0, Qt::AlignTop);

    auto textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);

    m_lblTitle = new QLabel(this);
    m_lblTitle->setFont(DashboardTheme::HeaderFont(10, true));
    m_lblTitle->setStyleSheet("color: #F9FAFB; font-weight: bold; border: none;");
    textLayout->addWidget(m_lblTitle);

    m_lblMessage = new QLabel(this);
    m_lblMessage->setFont(DashboardTheme::BodyFont(8, false));
    m_lblMessage->setStyleSheet("color: #D1D5DB; border: none;");
    m_lblMessage->setWordWrap(true);
    textLayout->addWidget(m_lblMessage);

    mainLayout->addLayout(textLayout, 1);

    m_btnClose = new QPushButton("✕", this);
    m_btnClose->setFixedSize(20, 20);
    m_btnClose->setStyleSheet(
        "QPushButton { background: transparent; border: none; color: #9CA3AF; font-size: 11pt; font-weight: bold; }"
        "QPushButton:hover { color: #F3F4F6; }"
    );
    connect(m_btnClose, &QPushButton::clicked, this, &TrafficNotificationPanel::Dismiss);
    mainLayout->addWidget(m_btnClose, 0, Qt::AlignTop);

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &TrafficNotificationPanel::OnTimerTimeout);
}

void TrafficNotificationPanel::ShowNotification(const QString& title,
                                               const QString& message,
                                               const QString& severity,
                                               int durationMs) {
    m_currentTitle = title;
    m_currentMessage = message;
    m_currentSeverity = severity;

    m_lblTitle->setText(title);
    m_lblMessage->setText(message);

    if (severity == "CRITICAL" || severity == "ERROR") {
        setStyleSheet(
            "QFrame#trafficNotificationPanel { background-color: #1F1315; border: 2px solid #EF4444; border-radius: 8px; }"
        );
        m_lblIcon->setPixmap(DashboardIcons::WarningAlertIcon(24).pixmap(24, 24));
    } else if (severity == "WARN") {
        setStyleSheet(
            "QFrame#trafficNotificationPanel { background-color: #1F1A12; border: 2px solid #F59E0B; border-radius: 8px; }"
        );
        m_lblIcon->setPixmap(DashboardIcons::WarningAlertIcon(24).pixmap(24, 24));
    } else { // SUCCESS / INFO
        setStyleSheet(
            "QFrame#trafficNotificationPanel { background-color: #0E1F1A; border: 2px solid #10B981; border-radius: 8px; }"
        );
        m_lblIcon->setPixmap(DashboardIcons::CheckmarkIcon(24).pixmap(24, 24));
    }

    show();
    raise();

    if (durationMs > 0) {
        m_timer->start(durationMs);
    }
}

void TrafficNotificationPanel::Dismiss() {
    m_timer->stop();
    hide();
}

void TrafficNotificationPanel::OnTimerTimeout() {
    Dismiss();
}

} // namespace traffic
} // namespace dashboard
} // namespace aegis

