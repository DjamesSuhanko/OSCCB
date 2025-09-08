#pragma once
#include <QPushButton>
#include <QColor>

class ModernButton : public QPushButton
{
    Q_OBJECT
public:
    explicit ModernButton(QWidget* parent = nullptr);

    // Estilo configurável
    void setNormalColor (const QColor& c) { m_normal  = c; update(); }
    void setHoverColor  (const QColor& c) { m_hover   = c; update(); }
    void setPressedColor(const QColor& c) { m_pressed = c; update(); }
    void setCheckedColor(const QColor& c) { m_checked = c; update(); }
    void setTextColor   (const QColor& c) { m_text    = c; update(); }
    void setRadius(int r)                 { m_radius  = r; update(); }
    void setPadding(int px)               { m_pad     = px; update(); }

    void setIconSizePx(int px);
    void setSpacing(int px) { m_spacing = px; update(); }

protected:
    void paintEvent(QPaintEvent* e) override;

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    void enterEvent(QEvent* e) override;
#else
    void enterEvent(QEnterEvent* e) override;
#endif
    void leaveEvent(QEvent* e) override;

    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    bool event(QEvent* ev) override; // trata touch também

private:
    QColor m_normal  = QColor("#333333");
    QColor m_hover   = QColor("#444444");
    QColor m_pressed = QColor("#222222");
    QColor m_checked = QColor("#00C853");
    QColor m_text    = Qt::white;

    int  m_radius  = 10;
    int  m_pad     = 10;
    int  m_iconPx  = 20;
    int  m_spacing = 8;
    bool m_hovering = false;

    bool m_pressedActive = false; // <--- novo, corrige bug do pressed
};
