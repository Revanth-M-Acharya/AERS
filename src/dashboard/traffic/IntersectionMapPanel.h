/**
 * @file IntersectionMapPanel.h
 * @brief SMART AEGIS (AERS) Phase 18 - Real-Time Interactive Intersection Vector Map Canvas.
 */

#ifndef AEGIS_DASHBOARD_INTERSECTION_MAP_PANEL_H
#define AEGIS_DASHBOARD_INTERSECTION_MAP_PANEL_H

#include "TrafficDashboardDataModel.h"
#include "TrafficDashboardConfig.h"
#include <QtWidgets/QWidget>
#include <QtCore/QTimer>
#include <QtCore/QPointF>
#include <memory>
#include <vector>

namespace aegis {
namespace dashboard {
namespace traffic {

/**
 * @brief Interactive 2D Vector Map Canvas for municipal traffic signal monitoring.
 */
class IntersectionMapPanel : public QWidget {
    Q_OBJECT

public:
    explicit IntersectionMapPanel(std::shared_ptr<TrafficDashboardDataModel> dataModel,
                                  const TrafficDashboardConfig& config = TrafficDashboardConfig(),
                                  QWidget* parent = nullptr);
    ~IntersectionMapPanel() override = default;

    void RefreshData();
    void ResetCamera();

protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void OnAnimationTick();

private:
    QPointF GeoToScreen(double lat, double lon, const QRectF& bounds) const;

    std::shared_ptr<TrafficDashboardDataModel> m_dataModel;
    TrafficDashboardConfig m_config;

    QTimer* m_animTimer{nullptr};
    double m_pulsePhase{0.0};
    double m_zoomLevel{1.0};
    QPointF m_panOffset{0.0, 0.0};
    bool m_isPanning{false};
    QPoint m_lastMousePos;

    struct ClickableNode {
        std::string nodeId;
        QRectF rect;
    };
    std::vector<ClickableNode> m_clickableNodes;
};

} // namespace traffic
} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_INTERSECTION_MAP_PANEL_H

