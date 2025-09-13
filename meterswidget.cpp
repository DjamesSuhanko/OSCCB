#include "MetersWidget.h"
#include <QPainter>
#include <QtMath>

MetersWidget::MetersWidget(int numChannels, QWidget* parent)
    : QWidget(parent),
    m_n(qMax(1, numChannels)),
    m_target(m_n, 0.f),
    m_display(m_n, 0.f)
{
    setBackgroundAutoFill(true);

    m_timer.setTimerType(Qt::PreciseTimer);
    m_timer.setInterval(m_intervalMs);
    connect(&m_timer, &QTimer::timeout, this, &MetersWidget::tick);
    m_timer.start();
}

void MetersWidget::setChannel01(int ch, float v01) {
    if (ch < 0 || ch >= m_n) return;
    if (v01 < 0.f) v01 = 0.f; else if (v01 > 1.f) v01 = 1.f;
    m_target[ch] = v01;
}

void MetersWidget::setLR01(float left01, float right01) {
    if (left01  < 0.f) left01  = 0.f; else if (left01  > 1.f) left01  = 1.f;
    if (right01 < 0.f) right01 = 0.f; else if (right01 > 1.f) right01 = 1.f;
    m_targetL = left01; m_targetR = right01;
}

void MetersWidget::setFps(int fps) {
    fps = qBound(10, fps, 120);
    m_intervalMs = int(1000.0 / double(fps) + 0.5);
    m_timer.setInterval(m_intervalMs);
}

void MetersWidget::setDecayPerSecond(float decay01ps) {
    m_decay01ps = qMax(0.f, decay01ps);
}

void MetersWidget::setBarPadding(int px) { m_pad = qMax(0, px); update(); }
void MetersWidget::setBarRadius(int r)   { m_radius = qMax(0, r); update(); }

void MetersWidget::setBackgroundAutoFill(bool enable) {
    setAutoFillBackground(enable);
    if (enable) setAttribute(Qt::WA_OpaquePaintEvent, true);
    update();
}

void MetersWidget::tick() {
    // passo de decay por frame (linear 0..1)
    const float step = m_decay01ps * (m_intervalMs / 1000.f);
    bool changed = false;

    auto adv = [&](float target, float& disp){
        if (target > disp) { // attack instantâneo
            if (qAbs(target - disp) > 0.001f) { disp = target; return true; }
            return false;
        } else {
            float nd = disp - step;
            if (nd < target) nd = target;
            if (qAbs(nd - disp) > 0.001f) { disp = nd; return true; }
            return false;
        }
    };

    // canais
    for (int i=0;i<m_n;++i) changed |= adv(m_target[i], m_display[i]);
    // LR
    changed |= adv(m_targetL, m_displayL);
    changed |= adv(m_targetR, m_displayR);

    if (changed) update(); // 1 repaint para tudo
}

void MetersWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    const int W = width(), H = height();
    const int totalBars = m_n + 2; // N canais + L + R
    const int gaps = totalBars + 1;
    const int usableW = W - m_pad * gaps;
    const int barW = qMax(1, usableW / totalBars);
    const int baseY = H;

    // cores
    QColor bg = palette().window().color();
    QColor bar = palette().highlight().color();       // barras CH
    QColor lr  = palette().highlight().color().darker(115); // LR levemente diferente

    // fundo (se não estiver usando autoFillBackground)
    if (!autoFillBackground()) {
        p.fillRect(rect(), bg);
    }

    auto drawBar = [&](int x, float v01, const QColor& c){
        int h = int(qBound(0.f, v01, 1.f) * H + 0.5f);
        QRect r(x, baseY - h, barW, h);
        if (m_radius > 0) {
            QPainterPath path; path.addRoundedRect(r.adjusted(0,0,0,0), m_radius, m_radius);
            p.fillPath(path, c);
        } else {
            p.fillRect(r, c);
        }
    };

    // desenha canais
    int x = m_pad;
    for (int i=0;i<m_n;++i) {
        drawBar(x, m_display[i], bar);
        x += barW + m_pad;
    }

    // separador mínimo antes do LR
    x += m_pad;

    // L e R
    drawBar(x, m_displayL, lr);
    x += barW + m_pad;
    drawBar(x, m_displayR, lr);

    // opcional: marcas horizontais (25/50/75%)
    p.setPen(palette().mid().color());
    for (int k=1;k<=3;++k) {
        int y = baseY - (H * k)/4;
        p.drawLine(0, y, W, y);
    }
}
