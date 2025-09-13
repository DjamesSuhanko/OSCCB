#include "ChannelMeterWidget.h"
#include <QPainter>
#include <QtMath>

ChannelMeterWidget::ChannelMeterWidget(QWidget* parent)
    : QWidget(parent)
{
    setAutoFillBackground(true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);

    m_timer.setTimerType(Qt::PreciseTimer);
    m_timer.setInterval(m_intervalMs);
    connect(&m_timer, &QTimer::timeout, this, &ChannelMeterWidget::tick);
    m_timer.start();
}

// ===== API FLUENTE =====
ChannelMeterWidget& ChannelMeterWidget::value01(float v){
    if (v < 0.f) v = 0.f; else if (v > 1.f) v = 1.f;
    m_target = v; return *this;
}
ChannelMeterWidget& ChannelMeterWidget::fps(int v){
    m_fps = qBound(10, v, 120);
    m_intervalMs = int(1000.0 / double(m_fps) + 0.5);
    m_timer.setInterval(m_intervalMs);
    return *this;
}
ChannelMeterWidget& ChannelMeterWidget::decay(float perSec){
    m_decay01ps = qMax(0.f, perSec);
    return *this;
}
ChannelMeterWidget& ChannelMeterWidget::radius(int r){
    m_radius = qMax(0, r); update(); return *this;
}
ChannelMeterWidget& ChannelMeterWidget::padding(int px){
    m_padding = qMax(0, px); update(); return *this;
}
ChannelMeterWidget& ChannelMeterWidget::gradient(bool on){
    m_grad = on; update(); return *this;
}

void ChannelMeterWidget::tick(){
    const float step = m_decay01ps * (m_intervalMs / 1000.f);
    bool ch=false;
    if (m_target >= m_disp) { if (qAbs(m_target - m_disp) > 0.001f) { m_disp = m_target; ch=true; } }
    else {
        float nd = m_disp - step;
        if (nd < m_target) nd = m_target;
        if (qAbs(nd - m_disp) > 0.001f) { m_disp = nd; ch=true; }
    }
    if (ch) update();
}

void ChannelMeterWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    const QRect R = rect().adjusted(m_padding, m_padding, -m_padding, -m_padding);
    if (!autoFillBackground()) p.fillRect(rect(), palette().window());

    const int H = R.height();
    const int fillH = int(qBound(0.f, m_disp, 1.f) * H + 0.5f);
    QRect bar(R.x(), R.y() + (H - fillH), R.width(), fillH);

    if (m_grad) {
        QLinearGradient g(bar.bottomLeft(), bar.topLeft());
        g.setColorAt(0.00, grad_base);
        g.setColorAt(0.70, grad_med);
        g.setColorAt(1.00, grad_top);
        p.fillRect(bar, g);
    } else {
       // p.fillRect(bar, palette().highlight());
        const QBrush solidBrush = m_solid.isValid() && m_solid.alpha() > 0 ? QBrush(m_solid) : QBrush(palette().highlight());
        p.fillRect(bar, solidBrush);
    }

    if (m_radius > 0) {
        QPainterPath path; path.addRoundedRect(bar, m_radius, m_radius);
        p.fillPath(path, p.brush());
    }
}



ChannelMeterWidget& ChannelMeterWidget::gradientStops(const QColor& c0, const QColor& c1, const QColor& cTop){
    grad_base=c0; grad_med=c1; grad_top=cTop; update(); return *this;
}

ChannelMeterWidget& ChannelMeterWidget::color(const QColor& c){
    m_solid = c; update(); return *this;
}
