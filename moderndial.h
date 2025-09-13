#pragma once
#include <QDial>
#include <QColor>

/*
 * ModernDial
 * - Arco custom (anti-alias), 360° (ou 270° opcional)
 * - Handle pode dar N voltas (turns) de min→max
 * - Progresso verde cresce PROPORCIONAL ao total (0..100%), sem “wrap”
 * - Sentido horário (clockwise)
 * - Pode mostrar % total ou % da volta atual (opcional)
 */

class ModernDial : public QDial {
    Q_OBJECT
    // Aparência
    Q_PROPERTY(QColor trackColor     MEMBER m_trackColor     DESIGNABLE true)
    Q_PROPERTY(QColor progressColor  MEMBER m_progressColor  DESIGNABLE true)
    Q_PROPERTY(QColor handleColor    MEMBER m_handleColor    DESIGNABLE true)
    Q_PROPERTY(QColor textColor      MEMBER m_textColor      DESIGNABLE true)
    Q_PROPERTY(int    thickness      MEMBER m_thickness      DESIGNABLE true)
    Q_PROPERTY(bool   showValue      MEMBER m_showValue      DESIGNABLE true)

    // Comportamento
    Q_PROPERTY(int    turns          MEMBER m_turns          DESIGNABLE true)     // quantas voltas min→max (ex.: 10)
    Q_PROPERTY(bool   fullCircle     MEMBER m_fullCircle     DESIGNABLE true)     // arco 360° (true) ou 270° (false)
    Q_PROPERTY(bool   displayTurnPercent MEMBER m_displayTurnPercent DESIGNABLE true) // mostra % da volta atual

public:
    explicit ModernDial(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* e) override;

private:
    // Aparência (defaults)
    QColor m_trackColor     = QColor(230,230,230);
    QColor m_progressColor  = QColor("#99bad5");
    QColor m_handleColor    = QColor(30,30,30);
    QColor m_textColor      = QColor(30,30,30);
    int    m_thickness      = 10;
    bool   m_showValue      = true;

    // Comportamento
    int  m_turns            = 10;   // padrão já útil para seu caso
    bool m_fullCircle       = true;
    bool m_displayTurnPercent = false;

    // Ângulos no sistema do Qt (0° = 3h, CCW positivo)
    static constexpr double kStartDegFull = 90.0;   // topo (12h)
    static constexpr double kSpanDegFull  = 360.0;
    static constexpr double kStartDeg270  = 135.0;  // knob clássico
    static constexpr double kSpanDeg270   = 270.0;
};
