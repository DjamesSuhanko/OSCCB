#pragma once
#include <QWidget>
#include <QColor>

class PercentBarWidget : public QWidget {
    Q_OBJECT
public:
    explicit PercentBarWidget(QWidget* parent=nullptr);

    // API encadeável
    PercentBarWidget& value01(float v);           // 0..1
    PercentBarWidget& valuePercent(int pct);      // 0..100
    PercentBarWidget& color(const QColor& c);
    PercentBarWidget& background(const QColor& c);
    PercentBarWidget& radius(int r);
    PercentBarWidget& padding(int px);
    PercentBarWidget& showText(bool on);
    PercentBarWidget& textColor(const QColor& c);

    float value01() const { return m_v; }

protected:
    void paintEvent(QPaintEvent*) override;
    QSize sizeHint() const override;          // <- só declaração
    QSize minimumSizeHint() const override;   // <- só declaração

private:
    float  m_v = 0.0f;          // 0..1
    QColor m_fg = QColor("#99bad5");
    QColor m_bg = QColor(0,0,0,60);
    QColor m_tx = Qt::white;
    int    m_radius = 4;
    int    m_padding = 2;
    bool   m_showText = false;
};
