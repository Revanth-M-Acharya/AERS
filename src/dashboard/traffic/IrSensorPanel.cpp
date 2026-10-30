/**
 * @file IrSensorPanel.cpp
 * @brief SMART AEGIS (AERS) Phase 18 - Real-Time Optical IR Vehicle Detection Sensor Panel Implementation.
 */

#include "IrSensorPanel.h"
#include "dashboard/DashboardTheme.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFrame>

namespace aegis {
namespace dashboard {
namespace traffic {

IrSensorPanel::IrSensorPanel(std::shared_ptr<TrafficDashboardDataModel> dataModel,
                             QWidget* parent)
    : QWidget(parent)
    , m_dataModel(std::move(dataModel))
{
    SetupUi();
    RefreshData();
}

void IrSensorPanel::SetupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(8);

    auto card = new QFrame(this);
    card->setObjectName("cardFrame");
    DashboardTheme::ApplyCardStyle(card);

    auto cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(12, 10, 12, 10);
    cardLayout->setSpacing(8);

    // Header
    auto headerLayout = new QHBoxLayout();
    auto lblTitle = new QLabel("OPTICAL IR SENSORS (PHASE 20)", card);
    lblTitle->setFont(DashboardTheme::HeaderFont(10, true));
    lblTitle->setStyleSheet("color: #F9FAFB; font-weight: bold; border: none;");
    headerLayout->addWidget(lblTitle);
    headerLayout->addStretch();

    auto lblTag = new QLabel("GPIO PIN INTERRUPT", card);
    lblTag->setFont(DashboardTheme::BadgeFont(7, true));
    DashboardTheme::ApplyBadgeStyle(lblTag, QColor("#1E3A8A"), QColor("#93C5FD"));
    headerLayout->addWidget(lblTag);
    cardLayout->addLayout(headerLayout);

    // Two-Column Sensor Grid
    auto sensorRow = new QHBoxLayout();
    sensorRow->setSpacing(10);

    // 1. Approach Sensor Box
    auto appBox = new QFrame(card);
    appBox->setStyleSheet("background-color: #111827; border: 1px solid #374151; border-radius: 6px;");
    auto appLayout = new QVBoxLayout(appBox);
    appLayout->setContentsMargins(8, 8, 8, 8);
    appLayout->setSpacing(4);

    auto topAppRow = new QHBoxLayout();
    auto lblAppTitle = new QLabel("APPROACH BEAM", appBox);
    lblAppTitle->setFont(DashboardTheme::HeaderFont(8, true));
    lblAppTitle->setStyleSheet("color: #E5E7EB; font-weight: bold; border: none;");
    topAppRow->addWidget(lblAppTitle);
    topAppRow->addStretch();

    m_lblApproachHealth = new QLabel("HEALTHY", appBox);
    m_lblApproachHealth->setFont(DashboardTheme::BadgeFont(7, true));
    DashboardTheme::ApplyBadgeStyle(m_lblApproachHealth, QColor("#064E3B"), QColor("#6EE7B7"));
    topAppRow->addWidget(m_lblApproachHealth);
    appLayout->addLayout(topAppRow);

    m_lblApproachStatus = new QLabel("BEAM: CLEAR", appBox);
    m_lblApproachStatus->setFont(DashboardTheme::BodyFont(8, true));
    m_lblApproachStatus->setStyleSheet("color: #10B981; border: none;");
    appLayout->addWidget(m_lblApproachStatus);

    m_lblApproachCount = new QLabel("Detected: 0", appBox);
    m_lblApproachCount->setFont(DashboardTheme::MetricFont(8, false));
    m_lblApproachCount->setStyleSheet("color: #9CA3AF; border: none;");
    appLayout->addWidget(m_lblApproachCount);

    sensorRow->addWidget(appBox);

    // 2. Departure Sensor Box
    auto depBox = new QFrame(card);
    depBox->setStyleSheet("background-color: #111827; border: 1px solid #374151; border-radius: 6px;");
    auto depLayout = new QVBoxLayout(depBox);
    depLayout->setContentsMargins(8, 8, 8, 8);
    depLayout->setSpacing(4);

    auto topDepRow = new QHBoxLayout();
    auto lblDepTitle = new QLabel("DEPARTURE BEAM", depBox);
    lblDepTitle->setFont(DashboardTheme::HeaderFont(8, true));
    lblDepTitle->setStyleSheet("color: #E5E7EB; font-weight: bold; border: none;");
    topDepRow->addWidget(lblDepTitle);
    topDepRow->addStretch();

    m_lblDepartureHealth = new QLabel("HEALTHY", depBox);
    m_lblDepartureHealth->setFont(DashboardTheme::BadgeFont(7, true));
    DashboardTheme::ApplyBadgeStyle(m_lblDepartureHealth, QColor("#064E3B"), QColor("#6EE7B7"));
    topDepRow->addWidget(m_lblDepartureHealth);
    depLayout->addLayout(topDepRow);

    m_lblDepartureStatus = new QLabel("BEAM: CLEAR", depBox);
    m_lblDepartureStatus->setFont(DashboardTheme::BodyFont(8, true));
    m_lblDepartureStatus->setStyleSheet("color: #10B981; border: none;");
    depLayout->addWidget(m_lblDepartureStatus);

    m_lblDepartureCount = new QLabel("Detected: 0", depBox);
    m_lblDepartureCount->setFont(DashboardTheme::MetricFont(8, false));
    m_lblDepartureCount->setStyleSheet("color: #9CA3AF; border: none;");
    depLayout->addWidget(m_lblDepartureCount);

    sensorRow->addWidget(depBox);
    cardLayout->addLayout(sensorRow);

    // Timestamp
    m_lblLastTimestamp = new QLabel("Last Trigger: --", card);
    m_lblLastTimestamp->setFont(DashboardTheme::BodyFont(7, false));
    m_lblLastTimestamp->setStyleSheet("color: #6B7280; border: none;");
    cardLayout->addWidget(m_lblLastTimestamp);

    mainLayout->addWidget(card);
}

void IrSensorPanel::RefreshData() {
    if (!m_dataModel) return;

    std::string selectedId = m_dataModel->GetSelectedIntersectionId();
    IrSensorTelemetry ir;
    if (!m_dataModel->GetIrSensors(selectedId, ir)) {
        return;
    }

    // Approach
    if (ir.approachDetected) {
        m_lblApproachStatus->setText("VEHICLE DETECTED");
        m_lblApproachStatus->setStyleSheet("color: #F59E0B; font-weight: bold; border: none;");
    } else {
        m_lblApproachStatus->setText("BEAM: CLEAR");
        m_lblApproachStatus->setStyleSheet("color: #10B981; border: none;");
    }
    m_lblApproachHealth->setText(QString::fromStdString(ir.approachHealth));
    m_lblApproachCount->setText(QString("Detected: %1").arg(ir.approachCount));

    // Departure
    if (ir.departureDetected) {
        m_lblDepartureStatus->setText("VEHICLE DETECTED");
        m_lblDepartureStatus->setStyleSheet("color: #F59E0B; font-weight: bold; border: none;");
    } else {
        m_lblDepartureStatus->setText("BEAM: CLEAR");
        m_lblDepartureStatus->setStyleSheet("color: #10B981; border: none;");
    }
    m_lblDepartureHealth->setText(QString::fromStdString(ir.departureHealth));
    m_lblDepartureCount->setText(QString("Detected: %1").arg(ir.departureCount));

    // Timestamp
    m_lblLastTimestamp->setText(QString("Last Trigger: %1")
                                .arg(QString::fromStdString(ir.lastDetectionIso.empty() ? "--" : ir.lastDetectionIso)));
}

} // namespace traffic
} // namespace dashboard
} // namespace aegis

