/**
 * @file MapScene.cpp
 * @brief SMART AEGIS (AERS) Phase 17 - Interactive 2D City Map Scene Implementation.
 */

#include "MapScene.h"
#include "MapRenderer.h"
#include "DashboardTheme.h"
#include <QtGui/QPainter>
#include <QtGui/QWheelEvent>
#include <QtGui/QMouseEvent>
#include <cmath>

namespace aegis {
namespace dashboard {

MapScene::MapScene(std::shared_ptr<DashboardDataModel> dataModel,
                   const DashboardConfig& config,
                   QWidget* parent)
    : QGraphicsView(parent)
    , m_dataModel(std::move(dataModel))
    , m_config(config)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShape(QFrame::NoFrame);
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setMouseTracking(true);

    // 30 FPS Animation Timer
    connect(&m_animationTimer, &QTimer::timeout, this, &MapScene::OnAnimationTimerTick);
    m_animationTimer.start(static_cast<int>(m_config.mapAnimationIntervalMs));
}

MapScene::~MapScene() {
    m_animationTimer.stop();
}

void MapScene::SetConfig(const DashboardConfig& config) {
    m_config = config;
    if (m_animationTimer.isActive()) {
        m_animationTimer.setInterval(static_cast<int>(m_config.mapAnimationIntervalMs));
    }
    viewport()->update();
}

void MapScene::ResetCamera() {
    m_zoomLevel = 1.0;
    m_panOffset = QPointF(0.0, 0.0);
    viewport()->update();
}

void MapScene::OnAnimationTimerTick() {
    m_frameCounter++;

    // Strobe light toggling at ~4 Hz
    if (m_frameCounter % 7 == 0) {
        m_strobeToggle = !m_strobeToggle;
    }

    // Incident marker pulsing ring animation
    if (m_pulseExpanding) {
        m_pulseRadius += 0.8;
        if (m_pulseRadius > 24.0) m_pulseExpanding = false;
    } else {
        m_pulseRadius -= 0.8;
        if (m_pulseRadius < 8.0) m_pulseExpanding = true;
    }

    viewport()->update();
}

void MapScene::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF vp = viewport()->rect();

    // 1. Fill Deep Obsidian Canvas Background
    painter.fillRect(vp, DashboardTheme::ColorBgObsidian);

    // Apply pan & zoom transform
    painter.save();
    painter.translate(vp.center() + m_panOffset);
    painter.scale(m_zoomLevel, m_zoomLevel);
    painter.translate(-vp.center());

    // 2. Tactical Coordinate Grid
    MapRenderer::DrawGrid(&painter, vp, m_config.mapBounds);

    if (!m_dataModel) {
        painter.restore();
        return;
    }

    // 3. Draw Municipal Street Segments
    auto intersections = m_dataModel->GetAllIntersections();
    std::unordered_map<std::string, QPointF> interScreenPositions;
    for (const auto& inter : intersections) {
        interScreenPositions[inter.intersectionId] = MapRenderer::GeoToScreen(
            inter.latitude, inter.longitude, vp, m_config.mapBounds);
    }

    auto hospitals = m_dataModel->GetAllHospitals();
    std::unordered_map<std::string, QPointF> hospScreenPositions;
    for (const auto& hosp : hospitals) {
        hospScreenPositions[hosp.hospitalId] = MapRenderer::GeoToScreen(
            hosp.latitude, hosp.longitude, vp, m_config.mapBounds);
    }

    // Connecting street avenues
    if (interScreenPositions.count("INT_A") && interScreenPositions.count("INT_B")) {
        MapRenderer::DrawRoadSegment(&painter, interScreenPositions["INT_A"], interScreenPositions["INT_B"], "Brigade Road", 28.0);
    }
    if (interScreenPositions.count("INT_B") && interScreenPositions.count("INT_C")) {
        MapRenderer::DrawRoadSegment(&painter, interScreenPositions["INT_B"], interScreenPositions["INT_C"], "MG Road", 28.0);
    }
    if (interScreenPositions.count("INT_C") && hospScreenPositions.count("HOSP_CITY_TRAUMA")) {
        MapRenderer::DrawRoadSegment(&painter, interScreenPositions["INT_C"], hospScreenPositions["HOSP_CITY_TRAUMA"], "Hospital Approach", 26.0);
    }

    // 4. Draw Active Green Corridors
    auto emergencies = m_dataModel->GetAllActiveEmergencies();
    for (const auto& em : emergencies) {
        if (!em.preemptedIntersections.empty()) {
            std::vector<QPointF> corridorPts;
            for (const auto& nodeName : em.preemptedIntersections) {
                if (interScreenPositions.count(nodeName)) {
                    corridorPts.push_back(interScreenPositions[nodeName]);
                } else if (hospScreenPositions.count(nodeName)) {
                    corridorPts.push_back(hospScreenPositions[nodeName]);
                }
            }
            if (hospScreenPositions.count(em.selectedHospitalId)) {
                corridorPts.push_back(hospScreenPositions[em.selectedHospitalId]);
            }

            double pulseAlpha = 0.7 + 0.3 * std::sin(m_frameCounter * 0.15);
            MapRenderer::DrawGreenCorridor(&painter, corridorPts, pulseAlpha);
        }

        // Draw Incident Marker Pin
        if (em.isActive) {
            // Scene marker near start of corridor or first intersection
            QPointF scenePt = vp.center();
            if (!em.routeWaypoints.empty() && interScreenPositions.count(em.routeWaypoints.front())) {
                scenePt = interScreenPositions[em.routeWaypoints.front()] + QPointF(-40, -40);
            }
            MapRenderer::DrawIncidentMarker(&painter, scenePt, "CODE RED: " + QString::fromStdString(em.incidentId), m_pulseRadius);
        }
    }

    // 5. Draw Intersections
    for (const auto& inter : intersections) {
        if (interScreenPositions.count(inter.intersectionId)) {
            MapRenderer::DrawIntersection(&painter, inter, interScreenPositions[inter.intersectionId], 1.0);
        }
    }

    // 6. Draw Hospitals
    for (const auto& hosp : hospitals) {
        if (hospScreenPositions.count(hosp.hospitalId)) {
            MapRenderer::DrawHospital(&painter, hosp, hospScreenPositions[hosp.hospitalId], 1.0);
        }
    }

    // 7. Draw Ambulances
    auto fleet = m_dataModel->GetAllFleetVehicles();
    for (const auto& veh : fleet) {
        QPointF vehPos = MapRenderer::GeoToScreen(veh.latitude, veh.longitude, vp, m_config.mapBounds);

        // Calculate heading angle towards destination
        double heading = 45.0;
        if (interScreenPositions.count(veh.destination)) {
            QPointF dest = interScreenPositions[veh.destination];
            heading = std::atan2(dest.y() - vehPos.y(), dest.x() - vehPos.x()) * 180.0 / 3.14159265358979323846 + 90.0;
        }

        MapRenderer::DrawAmbulance(&painter, veh, vehPos, heading, m_strobeToggle, 1.0);
    }

    painter.restore();

    // 8. Top-Right Tactical Map Controls HUD
    painter.setPen(QColor(156, 163, 175));
    painter.setFont(DashboardTheme::MonospaceFont(9, true));
    QString hud = QString("ZOOM: %1x | SIM: %2x %3")
                      .arg(m_zoomLevel, 0, 'f', 1)
                      .arg(static_cast<int>(m_config.simSpeed))
                      .arg(m_config.isPaused ? "[PAUSED]" : "[RUNNING]");
    painter.drawText(QRectF(vp.right() - 220, vp.top() + 10, 210, 20), Qt::AlignRight | Qt::AlignVCenter, hud);
}

void MapScene::wheelEvent(QWheelEvent* event) {
    double numDegrees = event->angleDelta().y() / 8.0;
    double numSteps = numDegrees / 15.0;
    double factor = std::pow(1.15, numSteps);

    m_zoomLevel = std::max(0.4, std::min(5.0, m_zoomLevel * factor));
    viewport()->update();
    event->accept();
}

void MapScene::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    } else {
        QGraphicsView::mousePressEvent(event);
    }
}

void MapScene::mouseMoveEvent(QMouseEvent* event) {
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();
        m_panOffset += QPointF(delta.x(), delta.y());
        viewport()->update();
        event->accept();
    } else {
        QGraphicsView::mouseMoveEvent(event);
    }
}

void MapScene::mouseReleaseEvent(QMouseEvent* event) {
    if (m_isPanning) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    } else {
        QGraphicsView::mouseReleaseEvent(event);
    }
}

void MapScene::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    viewport()->update();
}

} // namespace dashboard
} // namespace aegis

