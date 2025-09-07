#include "moderndial.h"
#include <QPainter>
#include <QtMath>
#include <cmath>

ModernDial::ModernDial(QWidget* parent) : QDial(parent) {
    setNotchesVisible(false);
    setMouseTracking(true);
    setWrapping(false); // quem “multiplica” é m_turns, não o wrap do QDial
}

void ModernDial::paintEvent(QPaintEvent* e) {
    Q_UNUSED(e);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const int w = width();
    const int h = height();
    const int size = qMin(w, h);

    const int margin = m_thickness/2 + 6;
    const QRectF rect(margin, margin, size - 2*margin, size - 2*margin);

    // Geometria do arco
    const double startDeg = m_fullCircle ? kStartDegFull : kStartDeg270;   // graus “Qt”
    const double spanDeg  = m_fullCircle ? kSpanDegFull  : kSpanDeg270;

    // Fundo transparente
    p.fillRect(rect.adjusted(-margin,-margin,margin,margin), Qt::transparent);

    // Trilho base
    QPen penTrack(m_trackColor, m_thickness, Qt::SolidLine, Qt::RoundCap);
    p.setPen(penTrack);
    p.drawArc(rect, int(startDeg * 16), int(spanDeg * 16));

    // Normalização do valor 0..1
    const int minv = minimum();
    const int maxv = maximum();
    double norm01 = 0.0;
    if (maxv > minv) {
        norm01 = double(value() - minv) / double(maxv - minv);
        if (norm01 < 0.0) norm01 = 0.0;
        if (norm01 > 1.0) norm01 = 1.0;
    }

    // --------- PROGRESSO (VERDE) PROPORCIONAL AO TOTAL ----------
    // Cresce linearmente 0..360° conforme 0..100% (sem wrap)
    double progressDeg = spanDeg * norm01;
    if (value() == maxv) progressDeg = spanDeg; // garante círculo cheio no 100%

    QPen penProg(m_progressColor, m_thickness, Qt::SolidLine, Qt::RoundCap);
    p.setPen(penProg);
    if (progressDeg > 0.0) {
        const int start16 = int(startDeg * 16);
        const int span16  = int(-progressDeg * 16); // negativo = horário
        p.drawArc(rect, start16, span16);
    }

    // --------- HANDLE (BOLINHA) MULTI-VOLTAS ----------
    // Handle roda N voltas entre min→max, mas seu ângulo visível é o “wrap” na volta atual
    const int   turns     = qMax(1, m_turns);
    const double totalDeg = spanDeg * norm01 * turns;     // quanto “rodou” no total
    double handleDeg = std::fmod(totalDeg, spanDeg);      // volta atual (0..spanDeg)
    if (handleDeg < 0) handleDeg += spanDeg;
    if (value() == maxv && turns > 0) handleDeg = spanDeg;

    // Posição do handle (horário: startDeg - handleDeg)
    const double angleDeg = startDeg - handleDeg;
    const double angleRad = qDegreesToRadians(angleDeg);

    const QPointF center = rect.center();
    const double  radius = rect.width() / 2.0;

    // y de tela cresce para baixo → usa -sin
    const QPointF knobPos = center + QPointF(std::cos(angleRad)*radius,
                                             -std::sin(angleRad)*radius);

    const int handleRadius = m_thickness + 2;
    p.setPen(Qt::NoPen);
    p.setBrush(m_handleColor);
    p.drawEllipse(knobPos, handleRadius, handleRadius);

    // --------- TEXTO (opcional) ----------
    if (m_showValue) {
        p.setPen(m_textColor);
        QFont f = p.font();
        f.setBold(true);
        p.setFont(f);

        double percentToShow = 0.0;
        if (m_displayTurnPercent && turns > 1) {
            // % da volta atual (casa com a posição do handle, não com o arco verde)
            const double turnsProgress = norm01 * turns;
            const double fracTurn      = turnsProgress - std::floor(turnsProgress);
            percentToShow = (value() == maxv) ? 100.0 : (fracTurn * 100.0);
        } else {
            // % TOTAL (casa com o arco verde)
            percentToShow = norm01 * 100.0;
        }

        const QString txt = QString::number(int(std::round(percentToShow))) + "%";
        p.drawText(QRectF(0,0,w,h), Qt::AlignCenter, txt);
    }
}
