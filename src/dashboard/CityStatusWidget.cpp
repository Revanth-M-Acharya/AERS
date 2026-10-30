/**
 * @file CityStatusWidget.cpp
 * @brief SMART AEGIS (AERS) Phase 17 - Municipal City Operations Status Bar Implementation.
 */

#include "CityStatusWidget.h"
#include "DashboardTheme.h"
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QFrame>

namespace aegis {
namespace dashboard {

CityStatusWidget::CityStatusWidget(std::shared_ptr<DashboardDataModel> dataModel, QWidget* parent)
    : QWidget(parent)
    , m_dataModel(std::move(dataModel))
{
    SetupUi();
    RefreshData();
}

void CityStatusWidget::SetupUi() {
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(8);

    auto card = new QFrame(this);
    card->setObjectName("cardFrame");
    DashboardTheme::ApplyCardStyle(card);

    auto layout = new QHBoxLayout(card);
    layout->setContentsMargins(12, 6, 12, 6);
    layout->setSpacing(16);

    // 1. Alert Level
    auto alertLayout = new QVBoxLayout();
    auto lblAlertTitle = new QLabel("MUNICIPAL ALERT", card);
    lblAlertTitle->setFont(DashboardTheme::HeaderFont(8, true));
    lblAlertTitle->setStyleSheet("color: #9CA3AF;");

    m_lblAlertLevel = new QLabel("NORMAL", card);
    m_lblAlertLevel->setFont(DashboardTheme::MetricFont(12, true));
    m_lblAlertLevel->setStyleSheet("color: #10B981; font-weight: bold;");

    alertLayout->addWidget(lblAlertTitle);
    alertLayout->addWidget(m_lblAlertLevel);
    layout->addLayout(alertLayout);

    // Separator
    auto sep1 = new QFrame(card);
    sep1->setFrameShape(QFrame::VLine);
    sep1->setStyleSheet("color: #374151;");
    layout->addWidget(sep1);

    // 2. Active Green Corridors
    auto corrLayout = new QVBoxLayout();
    auto lblCorrTitle = new QLabel("ACTIVE CORRIDORS", card);
    lblCorrTitle->setFont(DashboardTheme::HeaderFont(8, true));
    lblCorrTitle->setStyleSheet("color: #9CA3AF;");

    m_lblActiveCorridors = new QLabel("0", card);
    m_lblActiveCorridors->setFont(DashboardTheme::MetricFont(14, true));
    m_lblActiveCorridors->setStyleSheet("color: #10B981; font-weight: bold;");

    corrLayout->addWidget(lblCorrTitle);
    corrLayout->addWidget(m_lblActiveCorridors);
    layout->addLayout(corrLayout);

    // 3. Preempted Signals
    auto preLayout = new QVBoxLayout();
    auto lblPreTitle = new QLabel("SIGNALS PREEMPTED", card);
    lblPreTitle->setFont(DashboardTheme::HeaderFont(8, true));
    lblPreTitle->setStyleSheet("color: #9CA3AF;");

    m_lblPreemptedCount = new QLabel("0", card);
    m_lblPreemptedCount->setFont(DashboardTheme::MetricFont(14, true));
    m_lblPreemptedCount->setStyleSheet("color: #06B6D4; font-weight: bold;");

    preLayout->addWidget(lblPreTitle);
    preLayout->addWidget(m_lblPreemptedCount);
    layout->addLayout(preLayout);

    // 4. Avg Transit ETA
    auto etaLayout = new QVBoxLayout();
    auto lblEtaTitle = new QLabel("AVG TRANSIT ETA", card);
    lblEtaTitle->setFont(DashboardTheme::HeaderFont(8, true));
    lblEtaTitle->setStyleSheet("color: #9CA3AF;");

    m_lblAvgTransitEta = new QLabel("0.0s", card);
    m_lblAvgTransitEta->setFont(DashboardTheme::MetricFont(14, true));
    m_lblAvgTransitEta->setStyleSheet("color: #F59E0B; font-weight: bold;");

    etaLayout->addWidget(lblEtaTitle);
    etaLayout->addWidget(m_lblAvgTransitEta);
    layout->addLayout(etaLayout);

    // 5. Fleet Availability
    auto fleetLayout = new QVBoxLayout();
    auto lblFleetTitle = new QLabel("FLEET READINESS", card);
    lblFleetTitle->setFont(DashboardTheme::HeaderFont(8, true));
    lblFleetTitle->setStyleSheet("color: #9CA3AF;");

    auto fleetProgressLayout = new QHBoxLayout();
    m_progressFleet = new QProgressBar(card);
    m_progressFleet->setRange(0, 100);
    m_progressFleet->setValue(100);
    m_progressFleet->setFixedSize(100, 12);

    m_lblFleetPct = new QLabel("100%", card);
    m_lblFleetPct->setFont(DashboardTheme::MonospaceFont(9, true));
    m_lblFleetPct->setStyleSheet("color: #10B981;");

    fleetProgressLayout->addWidget(m_progressFleet);
    fleetProgressLayout->addWidget(m_lblFleetPct);

    fleetLayout->addWidget(lblFleetTitle);
    fleetLayout->addLayout(fleetProgressLayout);
    layout->addLayout(fleetLayout);

    // 6. Total Handled
    auto handledLayout = new QVBoxLayout();
    auto lblHandledTitle = new QLabel("TOTAL MISSIONS", card);
    lblHandledTitle->setFont(DashboardTheme::HeaderFont(8, true));
    lblHandledTitle->setStyleSheet("color: #9CA3AF;");

    m_lblTotalHandled = new QLabel("0", card);
    m_lblTotalHandled->setFont(DashboardTheme::MetricFont(14, true));
    m_lblTotalHandled->setStyleSheet("color: #3B82F6; font-weight: bold;");

    handledLayout->addWidget(lblHandledTitle);
    handledLayout->addWidget(m_lblTotalHandled);
    layout->addLayout(handledLayout);

    layout->addStretch();
    mainLayout->addWidget(card);
}

void CityStatusWidget::RefreshData() {
    if (!m_dataModel) return;

    CitySummaryMetrics m = m_dataModel->GetCityMetrics();
    m_lblAlertLevel->setText(QString::fromStdString(m.cityConditionStr));
    if (m.cityConditionStr == "CRITICAL" || m.cityConditionStr == "DISASTER") {
        m_lblAlertLevel->setStyleSheet("color: #EF4444; font-weight: bold;");
    } else if (m.cityConditionStr == "TACTICAL_ALERT") {
        m_lblAlertLevel->setStyleSheet("color: #F59E0B; font-weight: bold;");
    } else {
        m_lblAlertLevel->setStyleSheet("color: #10B981; font-weight: bold;");
    }

    m_lblActiveCorridors->setText(QString::number(m.activeCorridorsCount));
    m_lblPreemptedCount->setText(QString::number(m.preemptedIntersectionsCount));
    m_lblAvgTransitEta->setText(QString("%1s").arg(m.avgTransitEtaSec, 0, 'f', 1));

    int fleetVal = static_cast<int>(m.fleetAvailabilityPct);
    m_progressFleet->setValue(fleetVal);
    m_lblFleetPct->setText(QString("%1%").arg(fleetVal));
    m_lblTotalHandled->setText(QString::number(m.totalEmergenciesHandled));
}

} // namespace dashboard
} // namespace aegis

