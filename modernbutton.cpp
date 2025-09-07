#include "modernbutton.h"
#include <QPainter>
#include <QStyleOptionButton>
#include <QFontMetrics>
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
#include <QEnterEvent>
#endif

ModernButton::ModernButton(QWidget* parent)
    : QPushButton(parent)
{
    setCheckable(true);           // toggle on/off
    setFlat(true);                // remove 3D
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(32);
    // Para receber eventos de hover
    setAttribute(Qt::WA_Hover, true);
}

void ModernButton::setIconSizePx(int px)
{
    m_iconPx = qMax(0, px);
    QPushButton::setIconSize(QSize(m_iconPx, m_iconPx));
    update();
}

void ModernButton::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // --------- plano de fundo ----------
    QColor bg = m_normal;
    if (isChecked())      bg = m_checked;
    else if (isDown())    bg = m_pressed;
    else if (m_hovering)  bg = m_hover;

    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRoundedRect(rect(), m_radius, m_radius);

    // --------- conteúdo (ícone + texto) ----------
    // Caixa interna com padding
    QRect r = rect().adjusted(m_pad, m_pad, -m_pad, -m_pad);

    // Medidas do texto
    const QString t = text();
    QFont f = p.font(); f.setBold(true);
    p.setFont(f);
    QFontMetrics fm(f);
#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
    const int textW = t.isEmpty() ? 0 : fm.horizontalAdvance(t);
#else
    const int textW = t.isEmpty() ? 0 : fm.width(t);
#endif
    const int textH = fm.height();

    // Medidas do ícone
    const bool hasIcon = !icon().isNull() && m_iconPx > 0;
    const int  iconW = hasIcon ? m_iconPx : 0;
    const int  iconH = hasIcon ? m_iconPx : 0;

    // Layout: [icon] <spacing> [text] centralizados
    int contentW = 0;
    if (hasIcon && !t.isEmpty()) contentW = iconW + m_spacing + textW;
    else if (hasIcon)            contentW = iconW;
    else                         contentW = textW;

    const int cx = r.x() + (r.width()  - contentW)/2;
    const int cy = r.y() + (r.height() - qMax(iconH, textH))/2;

    int x = cx;

    p.setPen(m_text);

    // Ícone
    if (hasIcon) {
        QPixmap px = icon().pixmap(m_iconPx, m_iconPx);
        const int iy = cy + (qMax(iconH, textH) - iconH)/2;
        p.drawPixmap(QPoint(x, iy), px);
        x += iconW;
        if (!t.isEmpty()) x += m_spacing;
    }

    // Texto
    if (!t.isEmpty()) {
        const QRect textRect(x, cy, textW, qMax(iconH, textH));
        p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, t);
    }
}

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
void ModernButton::enterEvent(QEvent* e)
{
    m_hovering = true;
    update();
    QPushButton::enterEvent(e);
}
#else
void ModernButton::enterEvent(QEnterEvent* e)
{
    m_hovering = true;
    update();
    QPushButton::enterEvent(e);
}
#endif

void ModernButton::leaveEvent(QEvent* e)
{
    m_hovering = false;
    update();
    QPushButton::leaveEvent(e);
}
