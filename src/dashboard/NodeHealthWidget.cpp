/**
 * @file NodeHealthWidget.cpp
 * @brief SMART AEGIS (AERS) Phase 17 - Raspberry Pi 4 Hardware Telemetry & Health Monitor Widget Implementation.
 */

#include "NodeHealthWidget.h"
#include "DashboardTheme.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QScrollArea>

namespace aegis {
namespace dashboard {

NodeHealthWidget::NodeHealthWidget(std::shared_ptr<DashboardDataModel> dataModel, QWidget* parent)
    : QWidget(parent)
    , m_dataModel(std::move(dataModel))
{
    SetupUi();
    RefreshData();
}

void NodeHealthWidget::SetupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(8);

    auto scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background: transparent;");

    auto container = new QWidget(scrollArea);
    auto containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(4, 4, 4, 4);
    containerLayout->setSpacing(8);

    const std::vector<std::pair<std::string, std::string>> initialNodes = {
        {"COORDINATOR", "Coordinator Server (192.168.10.1)"},
        {"INT_A", "Brigade Rd Controller (192.168.10.11)"},
        {"INT_B", "MG Rd Controller (192.168.10.12)"},
        {"INT_C", "Hospital Appr Controller (192.168.10.13)"}
    };

    for (const auto& [nodeId, desc] : initialNodes) {
        auto card = new QFrame(container);
        card->setObjectName("cardFrame");
        DashboardTheme::ApplyCardStyle(card);

        auto cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(10, 8, 10, 8);
        cardLayout->setSpacing(6);

        // Header: Node name + role/IP
        auto topRow = new QHBoxLayout();
        auto lblNode = new QLabel(QString::fromStdString(nodeId), card);
        lblNode->setFont(DashboardTheme::HeaderFont(10, true));
        lblNode->setStyleSheet("color: #F9FAFB; font-weight: bold; border: none;");
        topRow->addWidget(lblNode);
        topRow->addStretch();

        auto lblRole = new QLabel(QString::fromStdString(desc), card);
        lblRole->setFont(DashboardTheme::BodyFont(8, false));
        lblRole->setStyleSheet("color: #9CA3AF; border: none;");
        topRow->addWidget(lblRole);
        cardLayout->addLayout(topRow);

        // CPU row
        auto cpuRow = new QHBoxLayout();
        auto lblCpuTitle = new QLabel("CPU:", card);
        lblCpuTitle->setFont(DashboardTheme::BodyFont(8, true));
        lblCpuTitle->setStyleSheet("color: #D1D5DB; border: none;");
        lblCpuTitle->setFixedWidth(35);

        auto progressCpu = new QProgressBar(card);
        progressCpu->setFixedHeight(6);
        progressCpu->setTextVisible(false);
        progressCpu->setRange(0, 100);
        progressCpu->setValue(18);
        DashboardTheme::ApplyProgressBarStyle(progressCpu, QColor("#10B981"));

        auto lblCpuVal = new QLabel("18%", card);
        lblCpuVal->setFont(DashboardTheme::MetricFont(8, false));
        lblCpuVal->setStyleSheet("color: #6EE7B7; border: none;");
        lblCpuVal->setFixedWidth(35);

        cpuRow->addWidget(lblCpuTitle);
        cpuRow->addWidget(progressCpu);
        cpuRow->addWidget(lblCpuVal);
        cardLayout->addLayout(cpuRow);

        // RAM row
        auto ramRow = new QHBoxLayout();
        auto lblRamTitle = new QLabel("RAM:", card);
        lblRamTitle->setFont(DashboardTheme::BodyFont(8, true));
        lblRamTitle->setStyleSheet("color: #D1D5DB; border: none;");
        lblRamTitle->setFixedWidth(35);

        auto progressRam = new QProgressBar(card);
        progressRam->setFixedHeight(6);
        progressRam->setTextVisible(false);
        progressRam->setRange(0, 100);
        progressRam->setValue(34);
        DashboardTheme::ApplyProgressBarStyle(progressRam, QColor("#3B82F6"));

        auto lblRamVal = new QLabel("34%", card);
        lblRamVal->setFont(DashboardTheme::MetricFont(8, false));
        lblRamVal->setStyleSheet("color: #93C5FD; border: none;");
        lblRamVal->setFixedWidth(35);

        ramRow->addWidget(lblRamTitle);
        ramRow->addWidget(progressRam);
        ramRow->addWidget(lblRamVal);
        cardLayout->addLayout(ramRow);

        // Telemetry: Temp + Latency + GPIO
        auto metaRow = new QHBoxLayout();
        auto lblTemp = new QLabel("SoC: 41.2°C", card);
        lblTemp->setFont(DashboardTheme::MetricFont(8, false));
        lblTemp->setStyleSheet("color: #F59E0B; border: none;");
        metaRow->addWidget(lblTemp);

        auto lblPing = new QLabel("RTT: 0.4ms", card);
        lblPing->setFont(DashboardTheme::MetricFont(8, false));
        lblPing->setStyleSheet("color: #06B6D4; border: none;");
        metaRow->addWidget(lblPing);
        metaRow->addStretch();

        // 8-pin GPIO state row
        auto lblGpioTitle = new QLabel("GPIO:", card);
        lblGpioTitle->setFont(DashboardTheme::BodyFont(7, false));
        lblGpioTitle->setStyleSheet("color: #6B7280; border: none;");
        metaRow->addWidget(lblGpioTitle);

        std::vector<QLabel*> pinLabels;
        for (int p = 0; p < 8; ++p) {
            auto pin = new QLabel(card);
            pin->setFixedSize(8, 8);
            pin->setStyleSheet("background-color: #374151; border-radius: 2px;");
            metaRow->addWidget(pin);
            pinLabels.push_back(pin);
        }
        cardLayout->addLayout(metaRow);

        containerLayout->addWidget(card);

        NodeHealthCardWidgets w;
        w.cardFrame = card;
        w.lblNodeName = lblNode;
        w.lblIpRole = lblRole;
        w.progressCpu = progressCpu;
        w.lblCpuVal = lblCpuVal;
        w.progressRam = progressRam;
        w.lblRamVal = lblRamVal;
        w.lblTemp = lblTemp;
        w.lblPing = lblPing;
        w.gpioPins = std::move(pinLabels);

        m_nodeHealthCards[nodeId] = w;
    }

    containerLayout->addStretch();
    scrollArea->setWidget(container);
    mainLayout->addWidget(scrollArea);
}

void NodeHealthWidget::RefreshData() {
    if (!m_dataModel) return;

    auto nodes = m_dataModel->GetClusterNodes();

    for (const auto& node : nodes) {
        auto it = m_nodeHealthCards.find(node.nodeId);
        if (it == m_nodeHealthCards.end()) continue;

        auto& w = it->second;

        // CPU
        int cpuInt = static_cast<int>(node.cpuUsagePct);
        w.progressCpu->setValue(cpuInt);
        w.lblCpuVal->setText(QString("%1%").arg(cpuInt));
        if (cpuInt > 85) {
            DashboardTheme::ApplyProgressBarStyle(w.progressCpu, QColor("#EF4444"));
        } else if (cpuInt > 65) {
            DashboardTheme::ApplyProgressBarStyle(w.progressCpu, QColor("#F59E0B"));
        } else {
            DashboardTheme::ApplyProgressBarStyle(w.progressCpu, QColor("#10B981"));
        }

        // RAM
        int ramInt = static_cast<int>(node.memoryUsagePct);
        w.progressRam->setValue(ramInt);
        w.lblRamVal->setText(QString("%1%").arg(ramInt));

        // Temp
        w.lblTemp->setText(QString("SoC: %1°C").arg(node.temperatureCelsius, 0, 'f', 1));
        if (node.temperatureCelsius > 75.0) {
            w.lblTemp->setStyleSheet("color: #EF4444; border: none; font-weight: bold;");
        } else if (node.temperatureCelsius > 60.0) {
            w.lblTemp->setStyleSheet("color: #F59E0B; border: none;");
        } else {
            w.lblTemp->setStyleSheet("color: #10B981; border: none;");
        }

        // Ping
        w.lblPing->setText(QString("RTT: %1ms").arg(node.latencyMs, 0, 'f', 1));

        // GPIO Pins
        for (size_t i = 0; i < w.gpioPins.size(); ++i) {
            bool pinActive = (i < node.activeGpioPins.size()) ? node.activeGpioPins[i] : false;
            if (pinActive) {
                w.gpioPins[i]->setStyleSheet("background-color: #10B981; border-radius: 2px;");
            } else {
                w.gpioPins[i]->setStyleSheet("background-color: #374151; border-radius: 2px;");
            }
        }
    }
}

} // namespace dashboard
} // namespace aegis

