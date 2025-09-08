#include "moderndial.h"
#include <QPainter>
#include <QtMath>
#include <cmath>

// ⚠️ NÃO force o wrapping aqui — a MainWindow já define.
//    Se quiser, comente a linha; manteremos visível para referência.
//    setWrapping(false) aqui quebrou a detecção de voltas na sua lógica.
ModernDial::ModernDial(QWidget* parent) : QDial(parent) {
    setNotchesVisible(false);
    setMouseTracking(true);
    // setWrapping(false); // REMOVIDO: quem controla é a MainWindow (wrapping(true))
}

static inline double clamp01(double x) {
    if (x < 0.0) return 0.0;
    if (x > 1.0) return 1.0;
    return x;
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

    // --------- Normalização do valor 0..1 DENTRO DA VOLTA ----------
    const int minv = minimum();
    const int maxv = maximum();
    double norm01 = 0.0; // posição na volta atual
    if (maxv > minv) {
        norm01 = double(value() - minv) / double(maxv - minv);
        norm01 = clamp01(norm01);
    }

    // --------- PROGRESSO (VERDE) PROPORCIONAL AO TOTAL ----------
    // Tente ler "progress01" da MainWindow (0..1 total após 10 voltas).
    // Se não vier, faça fallback para norm01/turns (cresce devagar).
    double total01 = -1.0;
    if (this->property("progress01").isValid()) {
        bool ok = false;
        total01 = this->property("progress01").toDouble(&ok);
        if (!ok) total01 = -1.0;
    }
    if (total01 < 0.0) {
        const int turns = qMax(1, m_turns);
        total01 = clamp01(norm01 / double(turns));
    }

    double progressDeg = spanDeg * total01;
    if (total01 >= 1.0) progressDeg = spanDeg; // garante círculo cheio no 100%

    QPen penProg(m_progressColor, m_thickness, Qt::SolidLine, Qt::RoundCap);
    p.setPen(penProg);
    if (progressDeg > 0.0) {
        const int start16 = int(startDeg * 16);
        const int span16  = int(-progressDeg * 16); // negativo = horário
        p.drawArc(rect, start16, span16);
    }

    // --------- HANDLE (BOLINHA) — UMA volta por ciclo do QDial ----------
    // A bolinha deve representar a posição DENTRO da volta atual.
    // NÃO multiplique por "turns" aqui — as N voltas visuais acontecem
    // porque a MainWindow atualiza o value (0..999) várias vezes (wrapping).
    const double handleDegWithinSpan = spanDeg * norm01;

    // Posição do handle (horário: startDeg - handleDeg)
    const double angleDeg = startDeg - handleDegWithinSpan;
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
        if (m_displayTurnPercent && m_turns > 1) {
            // % da volta atual (0..100 com base em norm01)
            percentToShow = norm01 * 100.0;
        } else {
            // % TOTAL (0..100 com base em total01)
            percentToShow = total01 * 100.0;
        }

        const QString txt = QString::number(int(std::round(percentToShow))) + "%";
        p.drawText(QRectF(0,0,w,h), Qt::AlignCenter, txt);
    }
}
