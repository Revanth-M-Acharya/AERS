/**
 * @file MapRenderer.h
 * @brief SMART AEGIS (AERS) Phase 17 - 2D Vector Map Renderer & Geographic Projection Engine.
 */

#ifndef AEGIS_DASHBOARD_MAP_RENDERER_H
#define AEGIS_DASHBOARD_MAP_RENDERER_H

#include "DashboardConfig.h"
#include "DashboardDataModel.h"
#include <QtCore/QRectF>
#include <QtCore/QPointF>
#include <QtGui/QPainter>
#include <vector>

namespace aegis {
namespace dashboard {

/**
 * @brief Scalable vector map renderer producing anti-aliased EOC tactical city visualizations.
 */
class MapRenderer {
public:
    static QPointF GeoToScreen(double lat, double lon, const QRectF& viewport, const MapBoundingBox& bounds);
    static std::pair<double, double> ScreenToGeo(const QPointF& pt, const QRectF& viewport, const MapBoundingBox& bounds);

    // Drawing primitives
    static void DrawGrid(QPainter* painter, const QRectF& viewport, const MapBoundingBox& bounds);
    static void DrawRoadSegment(QPainter* painter, const QPointF& p1, const QPointF& p2, const QString& roadName, double roadWidth = 28.0);
    static void DrawGreenCorridor(QPainter* painter, const std::vector<QPointF>& waypoints, double pulseAlpha = 0.8);
    static void DrawIntersection(QPainter* painter, const IntersectionRecord& record, const QPointF& pos, double zoomFactor = 1.0);
    static void DrawHospital(QPainter* painter, const HospitalCapacityRecord& record, const QPointF& pos, double zoomFactor = 1.0);
    static void DrawAmbulance(QPainter* painter, const FleetVehicleRecord& record, const QPointF& pos, double headingDeg = 0.0, bool strobeBlue = true, double zoomFactor = 1.0);
    static void DrawIncidentMarker(QPainter* painter, const QPointF& pos, const QString& label, double pulseRadius = 15.0);
};

} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_MAP_RENDERER_H

