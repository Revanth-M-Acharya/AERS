/**
 * @file IntersectionMapPanel.cpp
 * @brief SMART AEGIS (AERS) Phase 18 - Real-Time Interactive Intersection Vector Map Canvas Implementation.
 */

#include "IntersectionMapPanel.h"
#include "dashboard/DashboardTheme.h"
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtGui/QWheelEvent>
#include <QtGui/QMouseEvent>
#include <cmath>

namespace aegis {
namespace dashboard {
namespace traffic {

IntersectionMapPanel::IntersectionMapPanel(std::shared_ptr<TrafficDashboardDataModel> dataModel,
                                           const TrafficDashboardConfig& config,
                                           QWidget* parent)
    : QWidget(parent)
    , m_dataModel(std::move(dataModel))
    , m_config(config)
{
    setMinimumSize(400, 300);
    setMouseTracking(true);

    m_animTimer = new QTimer(this);
    m_animTimer->setInterval(static_cast<int>(m_config.animationIntervalMs));
    connect(m_animTimer, &QTimer::timeout, this, &IntersectionMapPanel::OnAnimationTick);
    m_animTimer->start();
}

void IntersectionMapPanel::ResetCamera() {
    m_zoomLevel = 1.0;
    m_panOffset = QPointF(0.0, 0.0);
    update();
}

void IntersectionMapPanel::RefreshData() {
    update();
}

void IntersectionMapPanel::OnAnimationTick() {
    m_pulsePhase += 0.08;
    if (m_pulsePhase > 6.28318) m_pulsePhase = 0.0;
    update();
}

QPointF IntersectionMapPanel::GeoToScreen(double lat, double lon, const QRectF& bounds) const {
    const auto& b = m_config.mapBounds;
    double normX = (lon - b.minLon) / (b.maxLon - b.minLon);
    double normY = (b.maxLat - lat) / (b.maxLat - b.minLat); // Invert Y

    double cx = bounds.width() * 0.5;
    double cy = bounds.height() * 0.5;

    double x = (normX * bounds.width() - cx) * m_zoomLevel + cx + m_panOffset.x();
    double y = (normY * bounds.height() - cy) * m_zoomLevel + cy + m_panOffset.y();
    return QPointF(x, y);
}

void IntersectionMapPanel::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QRectF bounds = rect();

    // 1. Background Obsidian Grid
    painter.fillRect(bounds, QColor("#0B0F19"));

    painter.setPen(QPen(QColor(22, 31, 48, 140), 1.0, Qt::DashLine));
    const double gridSize = 40.0 * m_zoomLevel;
    double startX = std::fmod(m_panOffset.x(), gridSize);
    double startY = std::fmod(m_panOffset.y(), gridSize);
    for (double x = startX; x < bounds.width(); x += gridSize) {
        painter.drawLine(QPointF(x, 0), QPointF(x, bounds.height()));
    }
    for (double y = startY; y < bounds.height(); y += gridSize) {
        painter.drawLine(QPointF(0, y), QPointF(bounds.width(), y));
    }

    if (!m_dataModel) return;

    auto intersections = m_dataModel->GetAllIntersections();
    std::string selectedId = m_dataModel->GetSelectedIntersectionId();
    auto sched = m_dataModel->GetCorridorSchedule();

    // 2. Road Avenues
    if (intersections.size() >= 2) {
        for (size_t i = 0; i + 1 < intersections.size(); ++i) {
            QPointF p1 = GeoToScreen(intersections[i].latitude, intersections[i].longitude, bounds);
            QPointF p2 = GeoToScreen(intersections[i + 1].latitude, intersections[i + 1].longitude, bounds);

            // Asphalt road base
            painter.setPen(QPen(QColor(31, 41, 55), 36.0 * m_zoomLevel, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.drawLine(p1, p2);

            // Dashed Yellow Centerline
            painter.setPen(QPen(QColor(245, 158, 11, 180), 2.0 * m_zoomLevel, Qt::DashLine));
            painter.drawLine(p1, p2);
        }
    }

    // 3. Green Corridor Glow Ribbon
    if (!sched.activeCorridorId.empty() && intersections.size() >= 2) {
        painter.save();
        QColor greenWaveColor(16, 185, 129, 130);
        painter.setPen(QPen(greenWaveColor, 18.0 * m_zoomLevel, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        for (size_t i = 0; i + 1 < intersections.size(); ++i) {
            QPointF p1 = GeoToScreen(intersections[i].latitude, intersections[i].longitude, bounds);
            QPointF p2 = GeoToScreen(intersections[i + 1].latitude, intersections[i + 1].longitude, bounds);
            painter.drawLine(p1, p2);
        }
        painter.restore();
    }

    // 4. Render Intersections
    m_clickableNodes.clear();

    for (const auto& inter : intersections) {
        QPointF pt = GeoToScreen(inter.latitude, inter.longitude, bounds);
        double nodeRadius = 26.0 * m_zoomLevel;

        QRectF nodeRect(pt.x() - nodeRadius, pt.y() - nodeRadius, nodeRadius * 2.0, nodeRadius * 2.0);
        m_clickableNodes.push_back({inter.nodeId, nodeRect});

        bool isSelected = (inter.nodeId == selectedId);

        // Selection Pulse Glow
        if (isSelected) {
            double pulseR = nodeRadius + 8.0 + 4.0 * std::sin(m_pulsePhase);
            painter.setPen(QPen(QColor(6, 182, 212, 180), 3.0));
            painter.setBrush(QBrush(QColor(6, 182, 212, 35)));
            painter.drawEllipse(pt, pulseR, pulseR);
        }

        // Node Body
        painter.setPen(QPen(isSelected ? QColor("#06B6D4") : (inter.isPreempted ? QColor("#10B981") : QColor("#374151")), 2.0));
        painter.setBrush(QBrush(QColor("#111827")));
        painter.drawEllipse(pt, nodeRadius, nodeRadius);

        // Mini 3-Lamp Traffic Signal Inside Node
        double lampR = 4.0 * m_zoomLevel;
        double spacing = 11.0 * m_zoomLevel;

        QPointF pRed(pt.x() - spacing, pt.y());
        QPointF pYellow(pt.x(), pt.y());
        QPointF pGreen(pt.x() + spacing, pt.y());

        // Red
        bool redOn = (inter.signalState == "RED");
        painter.setPen(Qt::NoPen);
        painter.setBrush(QBrush(redOn ? QColor("#EF4444") : QColor("#451A1A")));
        painter.drawEllipse(pRed, lampR, lampR);

        // Yellow
        bool yellowOn = (inter.signalState == "YELLOW" || inter.signalState == "FLASHING_YELLOW");
        painter.setBrush(QBrush(yellowOn ? QColor("#F59E0B") : QColor("#45300B")));
        painter.drawEllipse(pYellow, lampR, lampR);

        // Green
        bool greenOn = (inter.signalState == "GREEN");
        painter.setBrush(QBrush(greenOn ? QColor("#10B981") : QColor("#063726")));
        painter.drawEllipse(pGreen, lampR, lampR);

        // Header Label above node
        painter.setPen(QPen(QColor("#F9FAFB")));
        QFont titleFont = DashboardTheme::HeaderFont(static_cast<int>(std::max(8.0, 9.0 * m_zoomLevel)), true);
        painter.setFont(titleFont);
        QString label = QString::fromStdString(inter.nodeId + " (" + inter.name + ")");
        painter.drawText(QRectF(pt.x() - 120, pt.y() - nodeRadius - 26, 240, 20), Qt::AlignCenter, label);

        // Status Sub-badge below node
        QString statusText = QString("%1 • %2s").arg(QString::fromStdString(inter.currentPhase)).arg(inter.countdownSec);
        if (inter.isPreempted) {
            statusText += " [PREEMPTED]";
            painter.setPen(QPen(QColor("#10B981")));
        } else if (inter.manualOverrideActive) {
            statusText += " [OVERRIDE]";
            painter.setPen(QPen(QColor("#F59E0B")));
        } else {
            painter.setPen(QPen(QColor("#9CA3AF")));
        }
        QFont subFont = DashboardTheme::BodyFont(static_cast<int>(std::max(7.0, 8.0 * m_zoomLevel)), false);
        painter.setFont(subFont);
        painter.drawText(QRectF(pt.x() - 120, pt.y() + nodeRadius + 6, 240, 18), Qt::AlignCenter, statusText);
    }

    // 5. Canvas HUD overlay
    painter.setPen(QPen(QColor("#9CA3AF")));
    painter.setFont(DashboardTheme::BodyFont(8, false));
    painter.drawText(QRectF(12, bounds.height() - 24, 300, 18), Qt::AlignLeft,
                     "Drag to Pan • Wheel to Zoom • Click Node to Inspect");
}

void IntersectionMapPanel::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // Check if clicked on a node
        for (const auto& node : m_clickableNodes) {
            if (node.rect.contains(event->pos())) {
                if (m_dataModel) {
                    m_dataModel->SetSelectedIntersectionId(node.nodeId);
                }
                update();
                return;
            }
        }
        m_isPanning = true;
        m_lastMousePos = event->pos();
    }
}

void IntersectionMapPanel::mouseMoveEvent(QMouseEvent* event) {
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_panOffset += QPointF(delta.x(), delta.y());
        m_lastMousePos = event->pos();
        update();
    }
}

void IntersectionMapPanel::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isPanning = false;
    }
}

void IntersectionMapPanel::wheelEvent(QWheelEvent* event) {
    double factor = (event->angleDelta().y() > 0) ? 1.15 : 0.87;
    m_zoomLevel *= factor;
    if (m_zoomLevel < 0.4) m_zoomLevel = 0.4;
    if (m_zoomLevel > 5.0) m_zoomLevel = 5.0;
    update();
}

void IntersectionMapPanel::resizeEvent(QResizeEvent* /*event*/) {
    update();
}

} // namespace traffic
} // namespace dashboard
} // namespace aegis

