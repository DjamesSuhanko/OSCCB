#include "PercentBarWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

PercentBarWidget::PercentBarWidget(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

PercentBarWidget& PercentBarWidget::value01(float v) {
    v = qBound(0.0f, v, 1.0f);
    if (!qFuzzyCompare(m_v, v)) { m_v = v; update(); }
    return *this;
}

PercentBarWidget& PercentBarWidget::valuePercent(int pct) {
    return value01(qBound(0, pct, 100) / 100.0f);
}

PercentBarWidget& PercentBarWidget::color(const QColor& c)      { m_fg = c; update(); return *this; }
PercentBarWidget& PercentBarWidget::background(const QColor& c) { m_bg = c; update(); return *this; }
PercentBarWidget& PercentBarWidget::radius(int r)               { m_radius = qMax(0, r); update(); return *this; }
PercentBarWidget& PercentBarWidget::padding(int px)             { m_padding = qMax(0, px); update(); return *this; }
PercentBarWidget& PercentBarWidget::showText(bool on)           { m_showText = on; update(); return *this; }
PercentBarWidget& PercentBarWidget::textColor(const QColor& c)  { m_tx = c; update(); return *this; }

void PercentBarWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF R = rect().adjusted(m_padding, m_padding, -m_padding, -m_padding);
    if (R.isEmpty()) return;

    // Fundo
    QPainterPath bg; bg.addRoundedRect(R, m_radius, m_radius);
    p.fillPath(bg, m_bg);

    // Barra
    const qreal w = R.width() * m_v;
    if (w > 0.5) {
        QRectF fillR(R.x(), R.y(), w, R.height());
        if (w < m_radius*2) p.fillRect(fillR, m_fg);
        else {
            QPainterPath fg; fg.addRoundedRect(fillR, m_radius, m_radius);
            p.fillPath(fg, m_fg);
        }
    }

    // Texto opcional
    if (m_showText) {
        p.setPen(m_tx);
        p.drawText(R.toRect(), Qt::AlignCenter,
                   QString::number(int(std::round(m_v * 100.0f))) + QLatin1Char('%'));
    }
}

QSize PercentBarWidget::sizeHint() const        { return {120, 12}; }
QSize PercentBarWidget::minimumSizeHint() const { return {40, 8}; }
