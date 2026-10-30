/**
 * @file ClusterStatusWidget.h
 * @brief SMART AEGIS (AERS) Phase 17 - 4-Node Cluster Health & Consensus Overview Widget.
 */

#ifndef AEGIS_DASHBOARD_CLUSTER_STATUS_WIDGET_H
#define AEGIS_DASHBOARD_CLUSTER_STATUS_WIDGET_H

#include "DashboardDataModel.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <memory>
#include <unordered_map>

namespace aegis {
namespace dashboard {

/**
 * @brief Displays the 4-node cluster health, leader consensus state, and emergency gating barrier.
 */
class ClusterStatusWidget : public QWidget {
    Q_OBJECT

public:
    explicit ClusterStatusWidget(std::shared_ptr<DashboardDataModel> dataModel,
                                QWidget* parent = nullptr);
    ~ClusterStatusWidget() override = default;

    void RefreshData();

private:
    void SetupUi();

    std::shared_ptr<DashboardDataModel> m_dataModel;

    QLabel* m_lblEmergencyGate{nullptr};
    QLabel* m_lblConsensusLeader{nullptr};
    QLabel* m_lblQuorumStatus{nullptr};

    // Node mini-cards: COORDINATOR, INT_A, INT_B, INT_C
    struct NodeCardWidgets {
        QLabel* statusBadge{nullptr};
        QLabel* roleLabel{nullptr};
        QLabel* heartbeatAge{nullptr};
        QLabel* latencyLabel{nullptr};
    };
    std::unordered_map<std::string, NodeCardWidgets> m_nodeCards;
};

} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_CLUSTER_STATUS_WIDGET_H

