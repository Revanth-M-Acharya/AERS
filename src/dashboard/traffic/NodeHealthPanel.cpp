/**
 * @file NodeHealthPanel.cpp
 * @brief SMART AEGIS (AERS) Phase 18 - Real-Time Raspberry Pi 4 Node Hardware Health Panel Implementation.
 */

#include "NodeHealthPanel.h"
#include "dashboard/DashboardTheme.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFrame>

namespace aegis {
namespace dashboard {
namespace traffic {

NodeHealthPanel::NodeHealthPanel(std::shared_ptr<TrafficDashboardDataModel> dataModel,
                                 QWidget* parent)
    : QWidget(parent)
    , m_dataModel(std::move(dataModel))
{
    SetupUi();
    RefreshData();
}

void NodeHealthPanel::SetupUi() {
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
    m_lblNodeIdTitle = new QLabel("NODE HARDWARE HEALTH", card);
    m_lblNodeIdTitle->setFont(DashboardTheme::HeaderFont(10, true));
    m_lblNodeIdTitle->setStyleSheet("color: #F9FAFB; font-weight: bold; border: none;");
    headerLayout->addWidget(m_lblNodeIdTitle);
    headerLayout->addStretch();

    m_lblConnBadge = new QLabel("CONNECTED", card);
    m_lblConnBadge->setFont(DashboardTheme::BadgeFont(7, true));
    DashboardTheme::ApplyBadgeStyle(m_lblConnBadge, QColor("#064E3B"), QColor("#6EE7B7"));
    headerLayout->addWidget(m_lblConnBadge);
    cardLayout->addLayout(headerLayout);

    // CPU Row
    auto cpuRow = new QHBoxLayout();
    auto lblCpuTitle = new QLabel("CPU LOAD:", card);
    lblCpuTitle->setFont(DashboardTheme::BodyFont(8, true));
    lblCpuTitle->setStyleSheet("color: #D1D5DB; border: none;");
    lblCpuTitle->setFixedWidth(65);

    m_progressCpu = new QProgressBar(card);
    m_progressCpu->setFixedHeight(6);
    m_progressCpu->setTextVisible(false);
    m_progressCpu->setRange(0, 100);
    m_progressCpu->setValue(15);
    DashboardTheme::ApplyProgressBarStyle(m_progressCpu, QColor("#10B981"));

    m_lblCpuVal = new QLabel("15%", card);
    m_lblCpuVal->setFont(DashboardTheme::MetricFont(8, false));
    m_lblCpuVal->setStyleSheet("color: #6EE7B7; border: none;");
    m_lblCpuVal->setFixedWidth(35);

    cpuRow->addWidget(lblCpuTitle);
    cpuRow->addWidget(m_progressCpu);
    cpuRow->addWidget(m_lblCpuVal);
    cardLayout->addLayout(cpuRow);

    // Memory Row
    auto memRow = new QHBoxLayout();
    auto lblMemTitle = new QLabel("RAM LOAD:", card);
    lblMemTitle->setFont(DashboardTheme::BodyFont(8, true));
    lblMemTitle->setStyleSheet("color: #D1D5DB; border: none;");
    lblMemTitle->setFixedWidth(65);

    m_progressMem = new QProgressBar(card);
    m_progressMem->setFixedHeight(6);
    m_progressMem->setTextVisible(false);
    m_progressMem->setRange(0, 100);
    m_progressMem->setValue(32);
    DashboardTheme::ApplyProgressBarStyle(m_progressMem, QColor("#3B82F6"));

    m_lblMemVal = new QLabel("32%", card);
    m_lblMemVal->setFont(DashboardTheme::MetricFont(8, false));
    m_lblMemVal->setStyleSheet("color: #93C5FD; border: none;");
    m_lblMemVal->setFixedWidth(35);

    memRow->addWidget(lblMemTitle);
    memRow->addWidget(m_progressMem);
    memRow->addWidget(m_lblMemVal);
    cardLayout->addLayout(memRow);

    // Telemetry Metas: Temp, Latency, Loss, Heartbeat
    auto metaRow = new QHBoxLayout();
    m_lblTemp = new QLabel("SoC: 41.5°C", card);
    m_lblTemp->setFont(DashboardTheme::MetricFont(8, false));
    m_lblTemp->setStyleSheet("color: #F59E0B; border: none;");
    metaRow->addWidget(m_lblTemp);

    m_lblLatency = new QLabel("RTT: 0.4ms", card);
    m_lblLatency->setFont(DashboardTheme::MetricFont(8, false));
    m_lblLatency->setStyleSheet("color: #06B6D4; border: none;");
    metaRow->addWidget(m_lblLatency);

    m_lblPacketLoss = new QLabel("Loss: 0.0%", card);
    m_lblPacketLoss->setFont(DashboardTheme::MetricFont(8, false));
    m_lblPacketLoss->setStyleSheet("color: #10B981; border: none;");
    metaRow->addWidget(m_lblPacketLoss);

    m_lblHeartbeat = new QLabel("HB: 0.1s ago", card);
    m_lblHeartbeat->setFont(DashboardTheme::MetricFont(8, false));
    m_lblHeartbeat->setStyleSheet("color: #9CA3AF; border: none;");
    metaRow->addWidget(m_lblHeartbeat);
    metaRow->addStretch();
    cardLayout->addLayout(metaRow);

    // GPIO Matrix and Runtime Status
    auto gpioRow = new QHBoxLayout();
    auto lblRuntimeTitle = new QLabel("RUNTIME:", card);
    lblRuntimeTitle->setFont(DashboardTheme::BodyFont(7, false));
    lblRuntimeTitle->setStyleSheet("color: #9CA3AF; border: none;");
    gpioRow->addWidget(lblRuntimeTitle);

    m_lblRuntimeStatus = new QLabel("READY", card);
    m_lblRuntimeStatus->setFont(DashboardTheme::BadgeFont(7, true));
    DashboardTheme::ApplyBadgeStyle(m_lblRuntimeStatus, QColor("#064E3B"), QColor("#6EE7B7"));
    gpioRow->addWidget(m_lblRuntimeStatus);

    gpioRow->addStretch();

    auto lblGpioTitle = new QLabel("BCM2711 GPIO:", card);
    lblGpioTitle->setFont(DashboardTheme::BodyFont(7, false));
    lblGpioTitle->setStyleSheet("color: #9CA3AF; border: none;");
    gpioRow->addWidget(lblGpioTitle);

    m_gpioPinIndicators.clear();
    for (int p = 0; p < 8; ++p) {
        auto pin = new QLabel(card);
        pin->setFixedSize(8, 8);
        pin->setStyleSheet("background-color: #374151; border-radius: 2px;");
        gpioRow->addWidget(pin);
        m_gpioPinIndicators.push_back(pin);
    }

    cardLayout->addLayout(gpioRow);
    mainLayout->addWidget(card);
}

void NodeHealthPanel::RefreshData() {
    if (!m_dataModel) return;

    std::string selectedId = m_dataModel->GetSelectedIntersectionId();
    IntersectionTelemetry inter;
    if (!m_dataModel->GetIntersection(selectedId, inter)) {
        return;
    }

    m_lblNodeIdTitle->setText(QString::fromStdString("NODE HEALTH: " + inter.nodeId));

    // Connection
    if (inter.isOnline) {
        m_lblConnBadge->setText("CONNECTED");
        DashboardTheme::ApplyBadgeStyle(m_lblConnBadge, QColor("#064E3B"), QColor("#6EE7B7"));
    } else {
        m_lblConnBadge->setText("OFFLINE");
        DashboardTheme::ApplyBadgeStyle(m_lblConnBadge, QColor("#7F1D1D"), QColor("#FCA5A5"));
    }

    // CPU
    int cpuVal = static_cast<int>(inter.cpuPercent);
    m_progressCpu->setValue(cpuVal);
    m_lblCpuVal->setText(QString("%1%").arg(cpuVal));
    if (cpuVal > 80) {
        DashboardTheme::ApplyProgressBarStyle(m_progressCpu, QColor("#EF4444"));
    } else if (cpuVal > 60) {
        DashboardTheme::ApplyProgressBarStyle(m_progressCpu, QColor("#F59E0B"));
    } else {
        DashboardTheme::ApplyProgressBarStyle(m_progressCpu, QColor("#10B981"));
    }

    // Memory
    int memVal = static_cast<int>(inter.memPercent);
    m_progressMem->setValue(memVal);
    m_lblMemVal->setText(QString("%1%").arg(memVal));

    // Temp
    m_lblTemp->setText(QString("SoC: %1°C").arg(inter.tempCelsius, 0, 'f', 1));
    if (inter.tempCelsius > 70.0) {
        m_lblTemp->setStyleSheet("color: #EF4444; border: none; font-weight: bold;");
    } else {
        m_lblTemp->setStyleSheet("color: #F59E0B; border: none;");
    }

    // Latency & Loss
    m_lblLatency->setText(QString("RTT: %1ms").arg(inter.latencyMs, 0, 'f', 1));
    m_lblPacketLoss->setText(QString("Loss: %1%").arg(inter.packetLossPct, 0, 'f', 1));

    // Heartbeat
    m_lblHeartbeat->setText(QString("HB: %1s ago").arg(inter.lastHeartbeatSec, 0, 'f', 1));

    // Runtime
    if (inter.isReady) {
        m_lblRuntimeStatus->setText("READY");
        DashboardTheme::ApplyBadgeStyle(m_lblRuntimeStatus, QColor("#064E3B"), QColor("#6EE7B7"));
    } else {
        m_lblRuntimeStatus->setText("INITIALIZING");
        DashboardTheme::ApplyBadgeStyle(m_lblRuntimeStatus, QColor("#78350F"), QColor("#FDE68A"));
    }

    // GPIO Pins
    for (size_t i = 0; i < m_gpioPinIndicators.size(); ++i) {
        bool pinActive = (i < inter.activeGpioPins.size()) ? inter.activeGpioPins[i] : false;
        if (pinActive) {
            m_gpioPinIndicators[i]->setStyleSheet("background-color: #10B981; border-radius: 2px;");
        } else {
            m_gpioPinIndicators[i]->setStyleSheet("background-color: #374151; border-radius: 2px;");
        }
    }
}

} // namespace traffic
} // namespace dashboard
} // namespace aegis

