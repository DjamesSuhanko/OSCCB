#include "modernprogressbar.h"
#include <QPainter>
#include <QStyleOptionProgressBar>
#include <QtMath>

ModernProgressBar::ModernProgressBar(QWidget* parent)
    : QProgressBar(parent)
{
    setTextVisible(false); // você pode ligar se quiser mostrar "42%"
    setMinimum(0);
    setMaximum(100);
    // Para barras verticais, use setOrientation(Qt::Vertical)
}

void ModernProgressBar::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    // Fundo total do widget
    if (m_bg.alpha() > 0) {
        p.setPen(Qt::NoPen);
        p.setBrush(m_bg);
        p.drawRoundedRect(rect(), m_radius, m_radius);
    }

    // Área interna da barra
    const QRectF R = rect().adjusted(m_padding, m_padding, -m_padding, -m_padding);

    // Segurança
    if (R.width() <= 0 || R.height() <= 0) return;

    // Proporção 0..1
    const int minv = minimum();
    const int maxv = maximum();
    const int rng  = qMax(1, maxv - minv);
    const double frac = clamp01(double(value() - minv) / double(rng));

    // Segmentação
    const int N = qMax(1, m_segments);
    const bool vertical = (orientation() == Qt::Vertical);

    // Cálculo de tamanho de cada gomo e espaçamento total
    if (!vertical) {
        // Horizontal
        const double totalSpacing = m_spacing * (N - 1);
        const double segW = qMax(1.0, (R.width() - totalSpacing) / double(N));
        const double segH = R.height();

        // quantos gomos cheios + parcial
        const double totalSegs = frac * N;
        const int full = int(std::floor(totalSegs + 1e-9));
        const double part = totalSegs - full;

        // Desenha gomos
        p.setPen(Qt::NoPen);
        for (int i = 0; i < N; ++i) {
            const double x = R.x() + i * (segW + m_spacing);
            QRectF seg(x, R.y(), segW, segH);

            // trilho
            p.setBrush(m_track);
            p.drawRoundedRect(seg, m_radius, m_radius);

            // preenchimento
            if (i < full) {
                p.setBrush(m_fill);
                p.drawRoundedRect(seg, m_radius, m_radius);
            } else if (i == full && part > 0.0) {
                p.setBrush(m_fill);
                QRectF partial = seg;
                partial.setWidth(segW * part);
                // arredondar só a esquerda para não “apontar” no meio
                // truque: desenha parcial e depois um retângulo “tampa” pequeno
                p.drawRoundedRect(partial, m_radius, m_radius);
            }
        }
    } else {
        // Vertical (de baixo para cima)
        const double totalSpacing = m_spacing * (N - 1);
        const double segH = qMax(1.0, (R.height() - totalSpacing) / double(N));
        const double segW = R.width();

        const double totalSegs = frac * N;
        const int full = int(std::floor(totalSegs + 1e-9));
        const double part = totalSegs - full;

        p.setPen(Qt::NoPen);
        for (int i = 0; i < N; ++i) {
            // i = 0 é o gomo de baixo
            const double y = R.bottom() - (i+1)*segH - i*m_spacing + (segH - segH); // simplifica
            const double yTop = R.bottom() - (i+1)*segH - i*m_spacing;
            QRectF seg(R.x(), yTop, segW, segH);

            // trilho
            p.setBrush(m_track);
            p.drawRoundedRect(seg, m_radius, m_radius);

            // preenchimento
            if (i < full) {
                p.setBrush(m_fill);
                p.drawRoundedRect(seg, m_radius, m_radius);
            } else if (i == full && part > 0.0) {
                p.setBrush(m_fill);
                QRectF partial = seg;
                partial.setY(seg.y() + (1.0 - part) * segH);
                partial.setHeight(segH * part);
                p.drawRoundedRect(partial, m_radius, m_radius);
            }
        }
    }

    // Texto central (se ligado)
    if (isTextVisible()) {
        p.setPen(m_text);
        QFont f = p.font();
        f.setBold(true);
        p.setFont(f);

        // Use o texto do QProgressBar (ex.: "42%") ou formate o seu
        QString t = text();
        if (t.isEmpty()) {
            t = QString::number(int(std::round(frac * 100.0))) + "%";
        }
        p.drawText(rect(), Qt::AlignCenter, t);
    }
}
