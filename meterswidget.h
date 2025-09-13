#pragma once
#include <QWidget>
#include <QVector>
#include <QTimer>
#include <QPainterPath>

class MetersWidget : public QWidget {
    Q_OBJECT
public:
    explicit MetersWidget(int numChannels = 8, QWidget* parent = nullptr);

    // valores 0..1 vindos do mixer/processing
    void setChannel01(int ch, float v01);
    void setLR01(float left01, float right01);

    // configuração
    void setFps(int fps);                           // padrão 30 (33 ms)
    void setDecayPerSecond(float decay01ps);        // queda linear por segundo em 0..1 (padrão 0.8)
    void setBarPadding(int px);                     // padding entre barras (padrão 4)
    void setBarRadius(int r);                       // canto arredondado (padrão 3)
    void setBackgroundAutoFill(bool enable);        // autoFill + opaco

    QSize minimumSizeHint() const override { return {200, 80}; }
    QSize sizeHint() const override { return {400, 160}; }

protected:
    void paintEvent(QPaintEvent* ev) override;

private:
    void tick();

    int m_n = 8;
    QVector<float> m_target;   // 0..1 (medição atual)
    QVector<float> m_display;  // 0..1 (mostrado com decay)
    float m_targetL = 0.f, m_targetR = 0.f;
    float m_displayL = 0.f, m_displayR = 0.f;

    QTimer m_timer;
    float m_decay01ps = 0.8f;  // cai 0.8 por segundo (ex.: de 1.0 a 0.2 em 1s)
    int   m_intervalMs = 33;   // ~30 fps
    int   m_pad = 4;           // espaçamento entre barras
    int   m_radius = 3;        // cantos
};
