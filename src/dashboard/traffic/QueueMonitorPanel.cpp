/**
 * @file QueueMonitorPanel.cpp
 * @brief SMART AEGIS (AERS) Phase 18 - Real-Time Intersection Queue & Congestion Monitor Panel Implementation.
 */

#include "QueueMonitorPanel.h"
#include "dashboard/DashboardTheme.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFrame>

namespace aegis {
namespace dashboard {
namespace traffic {

QueueMonitorPanel::QueueMonitorPanel(std::shared_ptr<TrafficDashboardDataModel> dataModel,
                                     QWidget* parent)
    : QWidget(parent)
    , m_dataModel(std::move(dataModel))
{
    SetupUi();
    RefreshData();
}

void QueueMonitorPanel::SetupUi() {
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
    auto lblTitle = new QLabel("QUEUE & CONGESTION MONITOR", card);
    lblTitle->setFont(DashboardTheme::HeaderFont(10, true));
    lblTitle->setStyleSheet("color: #F9FAFB; font-weight: bold; border: none;");
    headerLayout->addWidget(lblTitle);
    headerLayout->addStretch();

    m_lblPeakBadge = new QLabel("NORMAL FLOW", card);
    m_lblPeakBadge->setFont(DashboardTheme::BadgeFont(7, true));
    DashboardTheme::ApplyBadgeStyle(m_lblPeakBadge, QColor("#064E3B"), QColor("#6EE7B7"));
    headerLayout->addWidget(m_lblPeakBadge);
    cardLayout->addLayout(headerLayout);

    // Metric Summary Row
    auto metricsRow = new QHBoxLayout();
    metricsRow->setSpacing(12);

    // 1. Queue Length
    auto qLayout = new QVBoxLayout();
    auto lblQTitle = new QLabel("QUEUE LENGTH", card);
    lblQTitle->setFont(DashboardTheme::BodyFont(7, false));
    lblQTitle->setStyleSheet("color: #9CA3AF; border: none;");
    m_lblQueueLength = new QLabel("0 VEH", card);
    m_lblQueueLength->setFont(DashboardTheme::MetricFont(12, true));
    m_lblQueueLength->setStyleSheet("color: #60A5FA; font-weight: bold; border: none;");
    qLayout->addWidget(lblQTitle);
    qLayout->addWidget(m_lblQueueLength);
    metricsRow->addLayout(qLayout);

    // 2. Wait Time
    auto wtLayout = new QVBoxLayout();
    auto lblWtTitle = new QLabel("EST. WAIT TIME", card);
    lblWtTitle->setFont(DashboardTheme::BodyFont(7, false));
    lblWtTitle->setStyleSheet("color: #9CA3AF; border: none;");
    m_lblWaitTime = new QLabel("0.0s", card);
    m_lblWaitTime->setFont(DashboardTheme::MetricFont(12, true));
    m_lblWaitTime->setStyleSheet("color: #F59E0B; font-weight: bold; border: none;");
    wtLayout->addWidget(lblWtTitle);
    wtLayout->addWidget(m_lblWaitTime);
    metricsRow->addLayout(wtLayout);

    // 3. Throughput
    auto tpLayout = new QVBoxLayout();
    auto lblTpTitle = new QLabel("THROUGHPUT", card);
    lblTpTitle->setFont(DashboardTheme::BodyFont(7, false));
    lblTpTitle->setStyleSheet("color: #9CA3AF; border: none;");
    m_lblThroughput = new QLabel("0 VPM", card);
    m_lblThroughput->setFont(DashboardTheme::MetricFont(12, true));
    m_lblThroughput->setStyleSheet("color: #10B981; font-weight: bold; border: none;");
    tpLayout->addWidget(lblTpTitle);
    tpLayout->addWidget(m_lblThroughput);
    metricsRow->addLayout(tpLayout);

    cardLayout->addLayout(metricsRow);

    // Progress 1: Traffic Density
    auto densityRow = new QHBoxLayout();
    auto lblDensTitle = new QLabel("DENSITY:", card);
    lblDensTitle->setFont(DashboardTheme::BodyFont(8, true));
    lblDensTitle->setStyleSheet("color: #D1D5DB; border: none;");
    lblDensTitle->setFixedWidth(65);

    m_progressDensity = new QProgressBar(card);
    m_progressDensity->setFixedHeight(7);
    m_progressDensity->setTextVisible(false);
    m_progressDensity->setRange(0, 100);
    m_progressDensity->setValue(35);
    DashboardTheme::ApplyProgressBarStyle(m_progressDensity, QColor("#3B82F6"));

    m_lblDensityPct = new QLabel("35%", card);
    m_lblDensityPct->setFont(DashboardTheme::MetricFont(8, false));
    m_lblDensityPct->setStyleSheet("color: #93C5FD; border: none;");
    m_lblDensityPct->setFixedWidth(35);

    densityRow->addWidget(lblDensTitle);
    densityRow->addWidget(m_progressDensity);
    densityRow->addWidget(m_lblDensityPct);
    cardLayout->addLayout(densityRow);

    // Progress 2: Lane Occupancy
    auto occRow = new QHBoxLayout();
    auto lblOccTitle = new QLabel("OCCUPANCY:", card);
    lblOccTitle->setFont(DashboardTheme::BodyFont(8, true));
    lblOccTitle->setStyleSheet("color: #D1D5DB; border: none;");
    lblOccTitle->setFixedWidth(65);

    m_progressOccupancy = new QProgressBar(card);
    m_progressOccupancy->setFixedHeight(7);
    m_progressOccupancy->setTextVisible(false);
    m_progressOccupancy->setRange(0, 100);
    m_progressOccupancy->setValue(30);
    DashboardTheme::ApplyProgressBarStyle(m_progressOccupancy, QColor("#10B981"));

    m_lblOccupancyPct = new QLabel("30%", card);
    m_lblOccupancyPct->setFont(DashboardTheme::MetricFont(8, false));
    m_lblOccupancyPct->setStyleSheet("color: #6EE7B7; border: none;");
    m_lblOccupancyPct->setFixedWidth(35);

    occRow->addWidget(lblOccTitle);
    occRow->addWidget(m_progressOccupancy);
    occRow->addWidget(m_lblOccupancyPct);
    cardLayout->addLayout(occRow);

    mainLayout->addWidget(card);
}

void QueueMonitorPanel::RefreshData() {
    if (!m_dataModel) return;

    std::string selectedId = m_dataModel->GetSelectedIntersectionId();
    QueueTelemetry q;
    if (!m_dataModel->GetQueue(selectedId, q)) {
        return;
    }

    m_lblQueueLength->setText(QString("%1 VEH").arg(q.queueLengthVehicles));
    m_lblWaitTime->setText(QString("%1s").arg(q.estimatedWaitTimeSec, 0, 'f', 1));
    m_lblThroughput->setText(QString("%1 VPM").arg(q.currentThroughputVpm));

    int densVal = static_cast<int>(q.trafficDensityPct);
    m_progressDensity->setValue(densVal);
    m_lblDensityPct->setText(QString("%1%").arg(densVal));
    if (densVal > 75) {
        DashboardTheme::ApplyProgressBarStyle(m_progressDensity, QColor("#EF4444"));
    } else if (densVal > 50) {
        DashboardTheme::ApplyProgressBarStyle(m_progressDensity, QColor("#F59E0B"));
    } else {
        DashboardTheme::ApplyProgressBarStyle(m_progressDensity, QColor("#3B82F6"));
    }

    int occVal = static_cast<int>(q.laneOccupancyPct);
    m_progressOccupancy->setValue(occVal);
    m_lblOccupancyPct->setText(QString("%1%").arg(occVal));

    if (q.isPeakLoad) {
        m_lblPeakBadge->setText("PEAK LOAD");
        DashboardTheme::ApplyBadgeStyle(m_lblPeakBadge, QColor("#7F1D1D"), QColor("#FCA5A5"));
    } else {
        m_lblPeakBadge->setText("NORMAL FLOW");
        DashboardTheme::ApplyBadgeStyle(m_lblPeakBadge, QColor("#064E3B"), QColor("#6EE7B7"));
    }
}

} // namespace traffic
} // namespace dashboard
} // namespace aegis

