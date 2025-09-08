#include "modernbutton.h"

#include <QPainter>
#include <QFontMetrics>
#include <QApplication>
#include <QMouseEvent>
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
#include <QEnterEvent>
#endif

ModernButton::ModernButton(QWidget* parent)
    : QPushButton(parent)
{
    // Por padrão este botão NÃO é checkable.
    // (Para botões de mute, ligue setCheckable(true) no seu setup.)
    setCheckable(false);
    setFlat(true);
    setCursor(Qt::PointingHandCursor);

    setAttribute(Qt::WA_Hover, true);
    setAttribute(Qt::WA_AcceptTouchEvents, true); // tablets / touch
    setMinimumHeight(32);
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

    // ======== plano de fundo por estado ========
    // prioridade: checked (se checkable) > pressed real > hover > normal
    QColor bg = m_normal;
    if (isCheckable() && isChecked()) {
        bg = m_checked;
    } else if (m_pressedActive) {
        bg = m_pressed;
    } else if (m_hovering) {
        bg = m_hover;
    }

    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRoundedRect(rect(), m_radius, m_radius);

    // ======== conteúdo: ícone + texto (horizontal) ========
    const QRect contentRect = rect().adjusted(m_pad, m_pad, -m_pad, -m_pad);

    // Fonte
    p.setPen(m_text);
    QFont f = p.font();
    f.setBold(true);
    p.setFont(f);
    QFontMetrics fm(f);

    const QString t = text();
#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
    const int textW = t.isEmpty() ? 0 : fm.horizontalAdvance(t);
#else
    const int textW = t.isEmpty() ? 0 : fm.width(t);
#endif
    const int textH = fm.height();

    const bool hasIcon = !icon().isNull() && m_iconPx > 0;
    const int  iconW   = hasIcon ? m_iconPx : 0;
    const int  iconH   = hasIcon ? m_iconPx : 0;

    int contentW = 0;
    if (hasIcon && !t.isEmpty()) contentW = iconW + m_spacing + textW;
    else if (hasIcon)            contentW = iconW;
    else                         contentW = textW;

    const int baseH = qMax(iconH, textH);
    const int cx = contentRect.x() + (contentRect.width()  - contentW)/2;
    const int cy = contentRect.y() + (contentRect.height() - baseH)/2;

    int x = cx;

    // Ícone
    if (hasIcon) {
        QPixmap pm = icon().pixmap(m_iconPx, m_iconPx);
        const int iy = cy + (baseH - iconH)/2;
        p.drawPixmap(QPoint(x, iy), pm);
        x += iconW;
        if (!t.isEmpty()) x += m_spacing;
    }

    // Texto
    if (!t.isEmpty()) {
        const QRect textRect(x, cy, textW, baseH);
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

// ========== Pressed real (mouse/touch) ==========

void ModernButton::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        m_pressedActive = true;
        update();
    }
    QPushButton::mousePressEvent(e);
}

void ModernButton::mouseReleaseEvent(QMouseEvent* e)
{
    if (m_pressedActive) {
        m_pressedActive = false;
        update();
    }
    QPushButton::mouseReleaseEvent(e);
}

bool ModernButton::event(QEvent* ev)
{
    switch (ev->type()) {
    case QEvent::TouchBegin:
        m_pressedActive = true;
        update();
        break;
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
        m_pressedActive = false;
        update();
        break;
    default:
        break;
    }
    return QPushButton::event(ev);
}
