/**
 * @file DashboardIcons.cpp
 * @brief SMART AEGIS (AERS) Phase 17 - Programmatic Vector Icon Generator Implementation.
 */

#include "DashboardIcons.h"
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace aegis {
namespace dashboard {

namespace {
QPixmap CreateTransparentPixmap(int size) {
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    return pixmap;
}
} // anonymous namespace

QIcon DashboardIcons::AmbulanceIcon(int size, const QColor& color) {
    QPixmap pix = CreateTransparentPixmap(size);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    // Vehicle body
    p.setBrush(color);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(QRectF(size * 0.1, size * 0.3, size * 0.8, size * 0.45), size * 0.08, size * 0.08);

    // Windshield cab cut
    p.setBrush(QColor(11, 15, 25));
    p.drawRect(QRectF(size * 0.65, size * 0.35, size * 0.2, size * 0.2));

    // Wheels
    p.setBrush(QColor(31, 41, 55));
    p.drawEllipse(QPointF(size * 0.3, size * 0.75), size * 0.12, size * 0.12);
    p.drawEllipse(QPointF(size * 0.7, size * 0.75), size * 0.12, size * 0.12);

    // Strobe on roof
    p.setBrush(QColor("#3B82F6"));
    p.drawRect(QRectF(size * 0.4, size * 0.22, size * 0.2, size * 0.08));

    return QIcon(pix);
}

QIcon DashboardIcons::HospitalIcon(int size, const QColor& color) {
    QPixmap pix = CreateTransparentPixmap(size);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    // Shield background
    p.setBrush(QColor(17, 24, 39));
    p.setPen(QPen(color, size * 0.08));
    p.drawRoundedRect(QRectF(size * 0.1, size * 0.1, size * 0.8, size * 0.8), size * 0.15, size * 0.15);

    // Red/White Medical Cross
    p.setBrush(color);
    p.setPen(Qt::NoPen);
    p.drawRect(QRectF(size * 0.4, size * 0.25, size * 0.2, size * 0.5));
    p.drawRect(QRectF(size * 0.25, size * 0.4, size * 0.5, size * 0.2));

    return QIcon(pix);
}

QIcon DashboardIcons::TrafficLightIcon(int size, const QColor& color) {
    QPixmap pix = CreateTransparentPixmap(size);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    // Light enclosure
    p.setBrush(QColor(31, 41, 55));
    p.setPen(QPen(QColor(55, 65, 81), 1));
    p.drawRoundedRect(QRectF(size * 0.25, size * 0.08, size * 0.5, size * 0.84), size * 0.12, size * 0.12);

    // Active Lamp
    p.setBrush(color);
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(size * 0.5, size * 0.5), size * 0.18, size * 0.18);

    return QIcon(pix);
}

QIcon DashboardIcons::NodeServerIcon(int size, const QColor& color) {
    QPixmap pix = CreateTransparentPixmap(size);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    p.setBrush(QColor(17, 24, 39));
    p.setPen(QPen(color, size * 0.08));
    p.drawRoundedRect(QRectF(size * 0.15, size * 0.15, size * 0.7, size * 0.3), size * 0.05, size * 0.05);
    p.drawRoundedRect(QRectF(size * 0.15, size * 0.55, size * 0.7, size * 0.3), size * 0.05, size * 0.05);

    p.setBrush(color);
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(size * 0.3, size * 0.3), size * 0.05, size * 0.05);
    p.drawEllipse(QPointF(size * 0.3, size * 0.7), size * 0.05, size * 0.05);

    return QIcon(pix);
}

QIcon DashboardIcons::WarningAlertIcon(int size, const QColor& color) {
    QPixmap pix = CreateTransparentPixmap(size);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.moveTo(size * 0.5, size * 0.12);
    path.lineTo(size * 0.9, size * 0.88);
    path.lineTo(size * 0.1, size * 0.88);
    path.closeSubpath();

    p.setBrush(color);
    p.setPen(Qt::NoPen);
    p.drawPath(path);

    p.setBrush(QColor(11, 15, 25));
    p.drawRect(QRectF(size * 0.46, size * 0.38, size * 0.08, size * 0.25));
    p.drawEllipse(QPointF(size * 0.5, size * 0.75), size * 0.05, size * 0.05);

    return QIcon(pix);
}

QIcon DashboardIcons::CheckmarkIcon(int size, const QColor& color) {
    QPixmap pix = CreateTransparentPixmap(size);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    p.setBrush(color);
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(size * 0.5, size * 0.5), size * 0.42, size * 0.42);

    p.setPen(QPen(QColor(11, 15, 25), size * 0.1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawLine(QPointF(size * 0.28, size * 0.5), QPointF(size * 0.44, size * 0.68));
    p.drawLine(QPointF(size * 0.44, size * 0.68), QPointF(size * 0.72, size * 0.34));

    return QIcon(pix);
}

QIcon DashboardIcons::GreenCorridorIcon(int size, const QColor& color) {
    QPixmap pix = CreateTransparentPixmap(size);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QPen(color, size * 0.12, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(size * 0.15, size * 0.85), QPointF(size * 0.5, size * 0.15));
    p.drawLine(QPointF(size * 0.5, size * 0.15), QPointF(size * 0.85, size * 0.85));

    return QIcon(pix);
}

QIcon DashboardIcons::HeartbeatPulseIcon(int size, const QColor& color) {
    QPixmap pix = CreateTransparentPixmap(size);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QPen(color, size * 0.09, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    QPainterPath path;
    path.moveTo(size * 0.1, size * 0.5);
    path.lineTo(size * 0.3, size * 0.5);
    path.lineTo(size * 0.4, size * 0.2);
    path.lineTo(size * 0.55, size * 0.8);
    path.lineTo(size * 0.7, size * 0.5);
    path.lineTo(size * 0.9, size * 0.5);
    p.drawPath(path);

    return QIcon(pix);
}

QIcon DashboardIcons::CpuIcon(int size, const QColor& color) {
    QPixmap pix = CreateTransparentPixmap(size);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    p.setBrush(QColor(17, 24, 39));
    p.setPen(QPen(color, size * 0.1));
    p.drawRect(QRectF(size * 0.25, size * 0.25, size * 0.5, size * 0.5));

    p.setPen(QPen(color, size * 0.08));
    p.drawLine(QPointF(size * 0.35, size * 0.1), QPointF(size * 0.35, size * 0.25));
    p.drawLine(QPointF(size * 0.65, size * 0.1), QPointF(size * 0.65, size * 0.25));
    p.drawLine(QPointF(size * 0.35, size * 0.75), QPointF(size * 0.35, size * 0.9));
    p.drawLine(QPointF(size * 0.65, size * 0.75), QPointF(size * 0.65, size * 0.9));

    return QIcon(pix);
}

QIcon DashboardIcons::MemoryIcon(int size, const QColor& color) {
    QPixmap pix = CreateTransparentPixmap(size);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    p.setBrush(QColor(17, 24, 39));
    p.setPen(QPen(color, size * 0.08));
    p.drawRect(QRectF(size * 0.1, size * 0.3, size * 0.8, size * 0.4));

    p.setBrush(color);
    p.setPen(Qt::NoPen);
    p.drawRect(QRectF(size * 0.2, size * 0.38, size * 0.12, size * 0.24));
    p.drawRect(QRectF(size * 0.44, size * 0.38, size * 0.12, size * 0.24));
    p.drawRect(QRectF(size * 0.68, size * 0.38, size * 0.12, size * 0.24));

    return QIcon(pix);
}

QIcon DashboardIcons::PlayIcon(int size, const QColor& color) {
    QPixmap pix = CreateTransparentPixmap(size);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.moveTo(size * 0.2, size * 0.15);
    path.lineTo(size * 0.85, size * 0.5);
    path.lineTo(size * 0.2, size * 0.85);
    path.closeSubpath();

    p.setBrush(color);
    p.setPen(Qt::NoPen);
    p.drawPath(path);
    return QIcon(pix);
}

QIcon DashboardIcons::PauseIcon(int size, const QColor& color) {
    QPixmap pix = CreateTransparentPixmap(size);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    p.setBrush(color);
    p.setPen(Qt::NoPen);
    p.drawRect(QRectF(size * 0.2, size * 0.15, size * 0.22, size * 0.7));
    p.drawRect(QRectF(size * 0.58, size * 0.15, size * 0.22, size * 0.7));
    return QIcon(pix);
}

QIcon DashboardIcons::ResetIcon(int size, const QColor& color) {
    QPixmap pix = CreateTransparentPixmap(size);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QPen(color, size * 0.12, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(QRectF(size * 0.15, size * 0.15, size * 0.7, size * 0.7), 45 * 16, 270 * 16);

    p.setBrush(color);
    p.setPen(Qt::NoPen);
    QPainterPath arrow;
    arrow.moveTo(size * 0.7, size * 0.15);
    arrow.lineTo(size * 0.85, size * 0.35);
    arrow.lineTo(size * 0.55, size * 0.35);
    arrow.closeSubpath();
    p.drawPath(arrow);

    return QIcon(pix);
}

QIcon DashboardIcons::SpeedIcon(int size, const QColor& color) {
    QPixmap pix = CreateTransparentPixmap(size);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QPen(color, size * 0.1, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(QRectF(size * 0.15, size * 0.2, size * 0.7, size * 0.7), 0, 180 * 16);

    p.drawLine(QPointF(size * 0.5, size * 0.55), QPointF(size * 0.72, size * 0.32));
    return QIcon(pix);
}

} // namespace dashboard
} // namespace aegis

