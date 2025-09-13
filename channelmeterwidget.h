#pragma once
#include <QWidget>
#include <QTimer>
#include <QPainterPath>

class ChannelMeterWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChannelMeterWidget(QWidget* parent=nullptr);

    // ===== API FLUENTE =====
    ChannelMeterWidget& value01(float v);     // 0..1
    ChannelMeterWidget& fps(int v);           // 10..120 (default 30)
    ChannelMeterWidget& decay(float perSec);  // queda 0..1 por segundo (default 1.0)
    ChannelMeterWidget& radius(int r);        // px (default 3)
    ChannelMeterWidget& padding(int px);      // px (default 2)
    ChannelMeterWidget& gradient(bool on);    // default true
    ChannelMeterWidget& gradientStops(const QColor& c0, const QColor& c1, const QColor& cTop);
    ChannelMeterWidget& color(const QColor& c); // cor quando gradient(false)


    // getters básicos (se precisar)
    float value01() const { return m_target; }

    QSize minimumSizeHint() const override { return {10, 40}; }
    QSize sizeHint()        const override { return {16, 80}; }

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void tick();

    QTimer m_timer;
    int    m_fps        = 30;
    int    m_intervalMs = 33;
    float  m_decay01ps  = 1.0f;
    int    m_radius     = 3;
    int    m_padding    = 2;
    bool   m_grad       = true;

    float  m_target = 0.f;  // entrada 0..1
    float  m_disp   = 0.f;  // exibido (attack instant + decay)

    QColor grad_base = QColor("#99bad5");
    QColor grad_med  = QColor("#728b9f");
    QColor grad_top  = QColor("#35414a");

    QColor m_solid = QColor(Qt::transparent);   // usa palette se ficar transparente
};
