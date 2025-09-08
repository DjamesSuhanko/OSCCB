#pragma once
#include <QProgressBar>
#include <QColor>

class ModernProgressBar : public QProgressBar
{
    Q_OBJECT
public:
    explicit ModernProgressBar(QWidget* parent = nullptr);

    // Estilo (getters/setters simples; pode também usar setProperty no .ui/.cpp)
    void setSegmentCount(int n)      { m_segments = qMax(1, n); update(); }
    void setSegmentSpacing(int px)   { m_spacing  = qMax(0, px); update(); }
    void setRadius(int r)            { m_radius   = qMax(0, r); update(); }
    void setPadding(int px)          { m_padding  = qMax(0, px); update(); }

    void setTrackColor(const QColor& c)   { m_track = c; update(); }
    void setFillColor(const QColor& c)    { m_fill  = c; update(); }
    void setBackgroundColor(const QColor& c) { m_bg = c; update(); }
    void setTextColor(const QColor& c)    { m_text = c; update(); }

protected:
    void paintEvent(QPaintEvent* e) override;

private:
    // estilo
    int    m_segments = 20;                 // número de “gomos”
    int    m_spacing  = 2;                  // espaço entre gomos (px)
    int    m_radius   = 6;                  // cantos arredondados dos gomos
    int    m_padding  = 6;                  // padding interno da barra

    QColor m_track = QColor("#2b2d31");     // cor do gomo vazio
    QColor m_fill  = QColor("#00C853");     // cor do gomo preenchido
    QColor m_bg    = Qt::transparent;       // fundo atrás da barra
    QColor m_text  = Qt::white;             // texto central (se textVisible=true)

    // util
    static inline double clamp01(double x) {
        return x < 0.0 ? 0.0 : (x > 1.0 ? 1.0 : x);
    }
};
