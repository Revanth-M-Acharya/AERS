/**
 * @file MapRenderer.cpp
 * @brief SMART AEGIS (AERS) Phase 17 - 2D Vector Map Renderer Implementation.
 */

#include "MapRenderer.h"
#include "DashboardTheme.h"
#include <QtGui/QPainterPath>
#include <cmath>

namespace aegis {
namespace dashboard {

QPointF MapRenderer::GeoToScreen(double lat, double lon, const QRectF& viewport, const MapBoundingBox& bounds) {
    if (bounds.maxLatitude <= bounds.minLatitude || bounds.maxLongitude <= bounds.minLongitude) {
        return viewport.center();
    }

    double normX = (lon - bounds.minLongitude) / (bounds.maxLongitude - bounds.minLongitude);
    // Invert Y because latitude increases North, but screen pixels increase South
    double normY = (bounds.maxLatitude - lat) / (bounds.maxLatitude - bounds.minLatitude);

    double x = viewport.left() + normX * viewport.width();
    double y = viewport.top() + normY * viewport.height();
    return QPointF(x, y);
}

std::pair<double, double> MapRenderer::ScreenToGeo(const QPointF& pt, const QRectF& viewport, const MapBoundingBox& bounds) {
    if (viewport.width() <= 0.0 || viewport.height() <= 0.0) {
        return {bounds.minLatitude, bounds.minLongitude};
    }

    double normX = (pt.x() - viewport.left()) / viewport.width();
    double normY = (pt.y() - viewport.top()) / viewport.height();

    double lon = bounds.minLongitude + normX * (bounds.maxLongitude - bounds.minLongitude);
    double lat = bounds.maxLatitude - normY * (bounds.maxLatitude - bounds.minLatitude);
    return {lat, lon};
}

void MapRenderer::DrawGrid(QPainter* painter, const QRectF& viewport, const MapBoundingBox& bounds) {
    painter->save();
    painter->setPen(QPen(QColor(31, 41, 55, 120), 1, Qt::DotLine));

    // Draw 8 horizontal and 8 vertical grid lines
    const int divisions = 8;
    for (int i = 1; i < divisions; ++i) {
        double x = viewport.left() + (viewport.width() * i) / divisions;
        painter->drawLine(QPointF(x, viewport.top()), QPointF(x, viewport.bottom()));

        double y = viewport.top() + (viewport.height() * i) / divisions;
        painter->drawLine(QPointF(viewport.left(), y), QPointF(viewport.right(), y));
    }

    // Grid coordinates label in corner
    painter->setPen(QColor(107, 114, 128));
    painter->setFont(DashboardTheme::MonospaceFont(8));
    QString coordText = QString("BOUNDS: [%1, %2] to [%3, %4]")
                            .arg(bounds.minLatitude, 0, 'f', 4)
                            .arg(bounds.minLongitude, 0, 'f', 4)
                            .arg(bounds.maxLatitude, 0, 'f', 4)
                            .arg(bounds.maxLongitude, 0, 'f', 4);
    painter->drawText(QRectF(viewport.left() + 10, viewport.bottom() - 25, 400, 20),
                      Qt::AlignLeft | Qt::AlignVCenter, coordText);
    painter->restore();
}

void MapRenderer::DrawRoadSegment(QPainter* painter, const QPointF& p1, const QPointF& p2, const QString& roadName, double roadWidth) {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // Dark Asphalt Body
    painter->setPen(QPen(QColor(17, 24, 39), roadWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->drawLine(p1, p2);

    // Outer curb borders
    painter->setPen(QPen(QColor(55, 65, 81), roadWidth + 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->drawLine(p1, p2);
    painter->setPen(QPen(QColor(24, 32, 47), roadWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->drawLine(p1, p2);

    // Dashed Yellow Center Divider
    painter->setPen(QPen(QColor(245, 158, 11, 160), 2.0, Qt::DashLine, Qt::FlatCap));
    painter->drawLine(p1, p2);

    // Street Name text along vector
    if (!roadName.isEmpty()) {
        QPointF mid = (p1 + p2) * 0.5;
        double angleRad = std::atan2(p2.y() - p1.y(), p2.x() - p1.x());
        double angleDeg = angleRad * 180.0 / 3.14159265358979323846;
        if (angleDeg > 90.0 || angleDeg < -90.0) angleDeg += 180.0;

        painter->save();
        painter->translate(mid);
        painter->rotate(angleDeg);
        painter->setFont(DashboardTheme::MonospaceFont(8, true));
        painter->setPen(QColor(156, 163, 175, 200));
        painter->drawText(QRectF(-100, -14, 200, 12), Qt::AlignCenter, roadName);
        painter->restore();
    }

    painter->restore();
}

void MapRenderer::DrawGreenCorridor(QPainter* painter, const std::vector<QPointF>& waypoints, double pulseAlpha) {
    if (waypoints.size() < 2) return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // Glowing Neon Emerald Green Ribbon
    int alphaOuter = static_cast<int>(50.0 * pulseAlpha);
    int alphaCore = static_cast<int>(200.0 * pulseAlpha);

    // Outer glow
    painter->setPen(QPen(QColor(16, 185, 129, alphaOuter), 36.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    for (size_t i = 0; i < waypoints.size() - 1; ++i) {
        painter->drawLine(waypoints[i], waypoints[i + 1]);
    }

    // Mid glow
    painter->setPen(QPen(QColor(16, 185, 129, alphaCore / 2), 18.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    for (size_t i = 0; i < waypoints.size() - 1; ++i) {
        painter->drawLine(waypoints[i], waypoints[i + 1]);
    }

    // High intensity core line
    painter->setPen(QPen(QColor(16, 185, 129, alphaCore), 6.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    for (size_t i = 0; i < waypoints.size() - 1; ++i) {
        painter->drawLine(waypoints[i], waypoints[i + 1]);
    }

    painter->restore();
}

void MapRenderer::DrawIntersection(QPainter* painter, const IntersectionRecord& record, const QPointF& pos, double zoomFactor) {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    double boxSize = 24.0 * zoomFactor;
    QRectF box(pos.x() - boxSize * 0.5, pos.y() - boxSize * 0.5, boxSize, boxSize);

    // Intersection junction pad
    painter->setBrush(QColor(31, 41, 55));
    painter->setPen(QPen(record.isPreempted ? DashboardTheme::ColorGreen : DashboardTheme::ColorBorder, 2.0));
    painter->drawRoundedRect(box, 4.0, 4.0);

    // Traffic Signal Lamp Pill
    QColor activeLamp = DashboardTheme::ColorRed;
    if (record.signalState == "GREEN") {
        activeLamp = DashboardTheme::ColorGreen;
    } else if (record.signalState == "YELLOW" || record.signalState == "FLASHING_YELLOW") {
        activeLamp = DashboardTheme::ColorAmber;
    }

    // Draw active glowing lamp circle
    painter->setBrush(activeLamp);
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(pos, 6.0 * zoomFactor, 6.0 * zoomFactor);

    // Preemption halo
    if (record.isPreempted) {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(DashboardTheme::ColorGreen, 1.5, Qt::DashLine));
        painter->drawEllipse(pos, 14.0 * zoomFactor, 14.0 * zoomFactor);
    }

    // Label
    painter->setFont(DashboardTheme::MonospaceFont(8, true));
    painter->setPen(QColor(249, 250, 251));
    painter->drawText(QRectF(pos.x() - 60, pos.y() + boxSize * 0.5 + 2, 120, 14),
                      Qt::AlignCenter, QString::fromStdString(record.intersectionId));

    painter->restore();
}

void MapRenderer::DrawHospital(QPainter* painter, const HospitalCapacityRecord& record, const QPointF& pos, double zoomFactor) {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    double size = 32.0 * zoomFactor;
    QRectF shield(pos.x() - size * 0.5, pos.y() - size * 0.5, size, size);

    // Hospital Shield
    painter->setBrush(QColor(17, 24, 39));
    painter->setPen(QPen(DashboardTheme::ColorBlue, 2.5));
    painter->drawRoundedRect(shield, 6.0, 6.0);

    // Medical Cross
    painter->setBrush(DashboardTheme::ColorBlue);
    painter->setPen(Qt::NoPen);
    double cw = 6.0 * zoomFactor;
    double ch = 16.0 * zoomFactor;
    painter->drawRect(QRectF(pos.x() - cw * 0.5, pos.y() - ch * 0.5, cw, ch));
    painter->drawRect(QRectF(pos.x() - ch * 0.5, pos.y() - cw * 0.5, ch, cw));

    // Name & ICU Badge
    painter->setFont(DashboardTheme::HeaderFont(8, true));
    painter->setPen(DashboardTheme::ColorTextWhite);
    painter->drawText(QRectF(pos.x() - 75, pos.y() - size * 0.5 - 16, 150, 14),
                      Qt::AlignCenter, QString::fromStdString(record.name));

    // ICU Available count bubble
    QString icuText = QString("ICU: %1/%2").arg(record.icuBedsAvailable).arg(record.icuBedsTotal);
    painter->setFont(DashboardTheme::MonospaceFont(7, true));
    painter->setPen(record.icuBedsAvailable > 0 ? DashboardTheme::ColorGreen : DashboardTheme::ColorRed);
    painter->drawText(QRectF(pos.x() - 60, pos.y() + size * 0.5 + 2, 120, 12),
                      Qt::AlignCenter, icuText);

    painter->restore();
}

void MapRenderer::DrawAmbulance(QPainter* painter, const FleetVehicleRecord& record, const QPointF& pos, double headingDeg, bool strobeBlue, double zoomFactor) {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->translate(pos);
    painter->rotate(headingDeg);

    double w = 18.0 * zoomFactor;
    double h = 32.0 * zoomFactor;
    QRectF body(-w * 0.5, -h * 0.5, w, h);

    // Ambulance Van Silhouette
    painter->setBrush(DashboardTheme::ColorTextWhite);
    painter->setPen(QPen(QColor(17, 24, 39), 1.5));
    painter->drawRoundedRect(body, 3.0, 3.0);

    // Red Cross emblem on roof
    painter->setBrush(DashboardTheme::ColorRed);
    painter->setPen(Qt::NoPen);
    painter->drawRect(QRectF(-2.0, -5.0, 4.0, 10.0));
    painter->drawRect(QRectF(-5.0, -2.0, 10.0, 4.0));

    // Flashing Strobe Light Bar
    QColor strobeColor = strobeBlue ? DashboardTheme::ColorBlue : DashboardTheme::ColorRed;
    painter->setBrush(strobeColor);
    painter->drawEllipse(QPointF(0, -h * 0.35), 3.0 * zoomFactor, 3.0 * zoomFactor);

    // Vehicle ID Label
    painter->rotate(-headingDeg); // Keep label upright
    painter->setFont(DashboardTheme::MonospaceFont(8, true));
    painter->setPen(DashboardTheme::ColorCyan);
    painter->drawText(QRectF(-40, h * 0.5 + 2, 80, 12), Qt::AlignCenter, QString::fromStdString(record.vehicleId));

    painter->restore();
}

void MapRenderer::DrawIncidentMarker(QPainter* painter, const QPointF& pos, const QString& label, double pulseRadius) {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // Concentric Pulsing Rings
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(QColor(239, 68, 68, 80), 2.0));
    painter->drawEllipse(pos, pulseRadius * 1.6, pulseRadius * 1.6);

    painter->setPen(QPen(QColor(239, 68, 68, 160), 2.0));
    painter->drawEllipse(pos, pulseRadius, pulseRadius);

    // Incident Center Pin
    painter->setBrush(DashboardTheme::ColorRed);
    painter->setPen(QPen(DashboardTheme::ColorTextWhite, 1.5));
    painter->drawEllipse(pos, 5.0, 5.0);

    // Alert label
    if (!label.isEmpty()) {
        painter->setFont(DashboardTheme::MonospaceFont(8, true));
        painter->setPen(DashboardTheme::ColorRed);
        painter->drawText(QRectF(pos.x() - 60, pos.y() - pulseRadius - 16, 120, 14),
                          Qt::AlignCenter, label);
    }

    painter->restore();
}

} // namespace dashboard
} // namespace aegis
