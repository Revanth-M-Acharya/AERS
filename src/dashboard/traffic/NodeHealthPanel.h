/**
 * @file NodeHealthPanel.h
 * @brief SMART AEGIS (AERS) Phase 18 - Real-Time Raspberry Pi 4 Node Hardware Health Panel.
 */

#ifndef AEGIS_DASHBOARD_TRAFFIC_NODE_HEALTH_PANEL_H
#define AEGIS_DASHBOARD_TRAFFIC_NODE_HEALTH_PANEL_H

#include "TrafficDashboardDataModel.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <memory>
#include <vector>

namespace aegis {
namespace dashboard {
namespace traffic {

/**
 * @brief Displays CPU %, RAM %, SoC Temp, RTT Latency, Heartbeat, Packet Loss,
 *        and GPIO hardware pins for the selected traffic intersection node.
 */
class NodeHealthPanel : public QWidget {
    Q_OBJECT

public:
    explicit NodeHealthPanel(std::shared_ptr<TrafficDashboardDataModel> dataModel,
                             QWidget* parent = nullptr);
    ~NodeHealthPanel() override = default;

    void RefreshData();

private:
    void SetupUi();

    std::shared_ptr<TrafficDashboardDataModel> m_dataModel;

    QLabel* m_lblNodeIdTitle{nullptr};
    QLabel* m_lblConnBadge{nullptr};
    QLabel* m_lblRuntimeStatus{nullptr};

    QProgressBar* m_progressCpu{nullptr};
    QLabel* m_lblCpuVal{nullptr};

    QProgressBar* m_progressMem{nullptr};
    QLabel* m_lblMemVal{nullptr};

    QLabel* m_lblTemp{nullptr};
    QLabel* m_lblLatency{nullptr};
    QLabel* m_lblPacketLoss{nullptr};
    QLabel* m_lblHeartbeat{nullptr};

    std::vector<QLabel*> m_gpioPinIndicators;
};

} // namespace traffic
} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_TRAFFIC_NODE_HEALTH_PANEL_H

