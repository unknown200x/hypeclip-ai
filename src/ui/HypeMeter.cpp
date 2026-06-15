#include "ui/HypeMeter.hpp"
#include <QPainter>
#include <QLinearGradient>
#include <algorithm>

namespace hypeclip {

HypeMeter::HypeMeter(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(24);
}

void HypeMeter::setValue(float v) { value_ = std::clamp(v, 0.0f, 1.0f); update(); }
void HypeMeter::setThreshold(float v) { threshold_ = std::clamp(v, 0.0f, 1.0f); update(); }

void HypeMeter::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF r = rect().adjusted(1, 1, -1, -1);
    const qreal radius = 6;

    // Track
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(30, 32, 38));
    p.drawRoundedRect(r, radius, radius);

    // Fill gradient (green -> amber -> red)
    QRectF fill = r;
    fill.setWidth(r.width() * value_);
    QLinearGradient g(r.topLeft(), r.topRight());
    g.setColorAt(0.0, QColor(40, 200, 120));
    g.setColorAt(0.6, QColor(240, 190, 60));
    g.setColorAt(1.0, QColor(235, 70, 70));
    p.setBrush(g);
    p.drawRoundedRect(fill, radius, radius);

    // Threshold tick
    const qreal x = r.left() + r.width() * threshold_;
    p.setPen(QPen(QColor(255, 255, 255, 180), 2));
    p.drawLine(QPointF(x, r.top()), QPointF(x, r.bottom()));

    // Label
    p.setPen(QColor(235, 235, 240));
    p.drawText(rect(), Qt::AlignCenter,
               QString::number(int(value_ * 100)) + "%");
}

} // namespace hypeclip
