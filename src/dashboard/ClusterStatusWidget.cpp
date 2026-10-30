/**
 * @file ClusterStatusWidget.cpp
 * @brief SMART AEGIS (AERS) Phase 17 - 4-Node Cluster Health & Consensus Overview Widget Implementation.
 */

#include "ClusterStatusWidget.h"
#include "DashboardTheme.h"
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QFrame>

namespace aegis {
namespace dashboard {

ClusterStatusWidget::ClusterStatusWidget(std::shared_ptr<DashboardDataModel> dataModel, QWidget* parent)
    : QWidget(parent)
    , m_dataModel(std::move(dataModel))
{
    SetupUi();
    RefreshData();
}

void ClusterStatusWidget::SetupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(8);

    auto card = new QFrame(this);
    card->setObjectName("cardFrame");
    DashboardTheme::ApplyCardStyle(card);

    auto cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(12, 10, 12, 10);
    cardLayout->setSpacing(10);

    // Header row: Title + Quorum + Leader + Gate status
    auto headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);

    auto lblTitle = new QLabel("CLUSTER HEALTH & CONSENSUS", card);
    lblTitle->setFont(DashboardTheme::HeaderFont(10, true));
    lblTitle->setStyleSheet("color: #F9FAFB; font-weight: bold;");
    headerLayout->addWidget(lblTitle);

    headerLayout->addStretch();

    m_lblQuorumStatus = new QLabel("QUORUM: 4/4", card);
    m_lblQuorumStatus->setFont(DashboardTheme::BadgeFont(8, true));
    DashboardTheme::ApplyBadgeStyle(m_lblQuorumStatus, QColor("#064E3B"), QColor("#6EE7B7"));
    headerLayout->addWidget(m_lblQuorumStatus);

    m_lblConsensusLeader = new QLabel("LEADER: COORDINATOR", card);
    m_lblConsensusLeader->setFont(DashboardTheme::BadgeFont(8, true));
    DashboardTheme::ApplyBadgeStyle(m_lblConsensusLeader, QColor("#1E3A8A"), QColor("#93C5FD"));
    headerLayout->addWidget(m_lblConsensusLeader);

    m_lblEmergencyGate = new QLabel("GATE: OPEN", card);
    m_lblEmergencyGate->setFont(DashboardTheme::BadgeFont(8, true));
    DashboardTheme::ApplyBadgeStyle(m_lblEmergencyGate, QColor("#064E3B"), QColor("#34D399"));
    headerLayout->addWidget(m_lblEmergencyGate);

    cardLayout->addLayout(headerLayout);

    // 4 Node Cards in a grid or row
    auto nodesLayout = new QHBoxLayout();
    nodesLayout->setSpacing(8);

    const std::vector<std::pair<std::string, std::string>> nodes = {
        {"COORDINATOR", "Coordinator Core"},
        {"INT_A", "Brigade Rd"},
        {"INT_B", "MG Rd"},
        {"INT_C", "Hospital Appr"}
    };

    for (const auto& [nodeId, nodeDesc] : nodes) {
        auto nodeBox = new QFrame(card);
        nodeBox->setStyleSheet(
            "QFrame { background-color: #111827; border: 1px solid #374151; border-radius: 6px; padding: 6px; }"
        );
        auto nLayout = new QVBoxLayout(nodeBox);
        nLayout->setContentsMargins(6, 6, 6, 6);
        nLayout->setSpacing(4);

        // Top line: Node ID and Status badge
        auto topRow = new QHBoxLayout();
        auto lblNodeId = new QLabel(QString::fromStdString(nodeId), nodeBox);
        lblNodeId->setFont(DashboardTheme::HeaderFont(9, true));
        lblNodeId->setStyleSheet("color: #F3F4F6; font-weight: bold; border: none;");
        topRow->addWidget(lblNodeId);
        topRow->addStretch();

        auto statusBadge = new QLabel("ONLINE", nodeBox);
        statusBadge->setFont(DashboardTheme::BadgeFont(7, true));
        DashboardTheme::ApplyBadgeStyle(statusBadge, QColor("#064E3B"), QColor("#6EE7B7"));
        topRow->addWidget(statusBadge);
        nLayout->addLayout(topRow);

        // Role line
        auto roleLabel = new QLabel(QString::fromStdString(nodeDesc), nodeBox);
        roleLabel->setFont(DashboardTheme::BodyFont(8, false));
        roleLabel->setStyleSheet("color: #9CA3AF; border: none;");
        nLayout->addWidget(roleLabel);

        // Metrics line: Ping + Heartbeat
        auto metaRow = new QHBoxLayout();
        auto latencyLabel = new QLabel("0.4ms", nodeBox);
        latencyLabel->setFont(DashboardTheme::MetricFont(8, false));
        latencyLabel->setStyleSheet("color: #60A5FA; border: none;");
        metaRow->addWidget(latencyLabel);
        metaRow->addStretch();

        auto heartbeatAge = new QLabel("0.1s ago", nodeBox);
        heartbeatAge->setFont(DashboardTheme::MetricFont(8, false));
        heartbeatAge->setStyleSheet("color: #9CA3AF; border: none;");
        metaRow->addWidget(heartbeatAge);
        nLayout->addLayout(metaRow);

        nodesLayout->addWidget(nodeBox);

        NodeCardWidgets widgets;
        widgets.statusBadge = statusBadge;
        widgets.roleLabel = roleLabel;
        widgets.heartbeatAge = heartbeatAge;
        widgets.latencyLabel = latencyLabel;
        m_nodeCards[nodeId] = widgets;
    }

    cardLayout->addLayout(nodesLayout);
    mainLayout->addWidget(card);
}

void ClusterStatusWidget::RefreshData() {
    if (!m_dataModel) return;

    auto nodes = m_dataModel->GetClusterNodes();
    size_t onlineCount = 0;
    std::string leaderId = "NONE";
    bool gateOpen = true;

    for (const auto& node : nodes) {
        if (node.isOnline) {
            onlineCount++;
        }
        if (node.isLeader) {
            leaderId = node.nodeId;
        }

        auto it = m_nodeCards.find(node.nodeId);
        if (it != m_nodeCards.end()) {
            auto& widgets = it->second;
            if (node.isOnline) {
                widgets.statusBadge->setText("ONLINE");
                DashboardTheme::ApplyBadgeStyle(widgets.statusBadge, QColor("#064E3B"), QColor("#6EE7B7"));
            } else {
                widgets.statusBadge->setText("OFFLINE");
                DashboardTheme::ApplyBadgeStyle(widgets.statusBadge, QColor("#7F1D1D"), QColor("#FCA5A5"));
            }

            widgets.roleLabel->setText(QString::fromStdString(node.role + " | " + node.ipAddress));
            widgets.latencyLabel->setText(QString("%1ms").arg(node.latencyMs, 0, 'f', 1));
            widgets.heartbeatAge->setText(QString("%1s ago").arg(node.lastHeartbeatSec, 0, 'f', 1));
        }
    }

    // Quorum is majority: >= 3 of 4
    m_lblQuorumStatus->setText(QString("QUORUM: %1/%2").arg(onlineCount).arg(nodes.empty() ? 4 : nodes.size()));
    if (onlineCount >= 3) {
        DashboardTheme::ApplyBadgeStyle(m_lblQuorumStatus, QColor("#064E3B"), QColor("#6EE7B7"));
    } else {
        DashboardTheme::ApplyBadgeStyle(m_lblQuorumStatus, QColor("#7F1D1D"), QColor("#FCA5A5"));
        gateOpen = false;
    }

    m_lblConsensusLeader->setText(QString("LEADER: %1").arg(QString::fromStdString(leaderId)));
    if (leaderId != "NONE") {
        DashboardTheme::ApplyBadgeStyle(m_lblConsensusLeader, QColor("#1E3A8A"), QColor("#93C5FD"));
    } else {
        DashboardTheme::ApplyBadgeStyle(m_lblConsensusLeader, QColor("#78350F"), QColor("#FDE68A"));
        gateOpen = false;
    }

    if (gateOpen) {
        m_lblEmergencyGate->setText("GATE: OPEN");
        DashboardTheme::ApplyBadgeStyle(m_lblEmergencyGate, QColor("#064E3B"), QColor("#34D399"));
    } else {
        m_lblEmergencyGate->setText("GATE: BLOCKED");
        DashboardTheme::ApplyBadgeStyle(m_lblEmergencyGate, QColor("#7F1D1D"), QColor("#F87171"));
    }
}

} // namespace dashboard
} // namespace aegis

