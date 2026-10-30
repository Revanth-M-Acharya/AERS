/**
 * @file MapScene.h
 * @brief SMART AEGIS (AERS) Phase 17 - Interactive 2D City Map Scene & Viewport.
 */

#ifndef AEGIS_DASHBOARD_MAP_SCENE_H
#define AEGIS_DASHBOARD_MAP_SCENE_H

#include "DashboardConfig.h"
#include "DashboardDataModel.h"
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsScene>
#include <QtCore/QTimer>
#include <memory>

namespace aegis {
namespace dashboard {

/**
 * @brief Interactive high-DPI 2D Vector Map Viewport supporting smooth zoom, pan, and real-time animation.
 */
class MapScene : public QGraphicsView {
    Q_OBJECT

public:
    explicit MapScene(std::shared_ptr<DashboardDataModel> dataModel,
                      const DashboardConfig& config = DashboardConfig(),
                      QWidget* parent = nullptr);
    ~MapScene() override;

    void SetConfig(const DashboardConfig& config);
    void ResetCamera();
    void UpdateScene() { if (viewport()) viewport()->update(); }

protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void OnAnimationTimerTick();

private:
    std::shared_ptr<DashboardDataModel> m_dataModel;
    DashboardConfig m_config;

    QTimer m_animationTimer;
    uint32_t m_frameCounter{0};
    double m_pulseRadius{10.0};
    bool m_pulseExpanding{true};
    bool m_strobeToggle{false};

    // Camera navigation
    double m_zoomLevel{1.0};
    QPointF m_panOffset{0.0, 0.0};
    bool m_isPanning{false};
    QPoint m_lastMousePos;
};

} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_MAP_SCENE_H
