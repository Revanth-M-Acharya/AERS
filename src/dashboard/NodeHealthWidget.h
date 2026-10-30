/**
 * @file NodeHealthWidget.h
 * @brief SMART AEGIS (AERS) Phase 17 - Raspberry Pi 4 Hardware Telemetry & Health Monitor Widget.
 */

#ifndef AEGIS_DASHBOARD_NODE_HEALTH_WIDGET_H
#define AEGIS_DASHBOARD_NODE_HEALTH_WIDGET_H

#include "DashboardDataModel.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QFrame>
#include <memory>
#include <unordered_map>
#include <vector>

namespace aegis {
namespace dashboard {

/**
 * @brief Displays CPU %, RAM %, SoC Temperature °C, Ping Latency, and GPIO matrix for all cluster nodes.
 */
class NodeHealthWidget : public QWidget {
    Q_OBJECT

public:
    explicit NodeHealthWidget(std::shared_ptr<DashboardDataModel> dataModel,
                              QWidget* parent = nullptr);
    ~NodeHealthWidget() override = default;

    void RefreshData();

private:
    void SetupUi();

    struct NodeHealthCardWidgets {
        QFrame* cardFrame{nullptr};
        QLabel* lblNodeName{nullptr};
        QLabel* lblIpRole{nullptr};
        QProgressBar* progressCpu{nullptr};
        QLabel* lblCpuVal{nullptr};
        QProgressBar* progressRam{nullptr};
        QLabel* lblRamVal{nullptr};
        QLabel* lblTemp{nullptr};
        QLabel* lblPing{nullptr};
        std::vector<QLabel*> gpioPins;
    };

    std::shared_ptr<DashboardDataModel> m_dataModel;
    std::unordered_map<std::string, NodeHealthCardWidgets> m_nodeHealthCards;
};

} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_NODE_HEALTH_WIDGET_H

