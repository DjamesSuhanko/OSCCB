#include "moderncombobox.h"

#include <QPainter>
#include <QStyleOptionViewItem>
#include <QListView>
#include <QStyle>
#include <QPaintEvent>
#include <QApplication>

// ===================== ModernComboItemDelegate =====================

ModernComboItemDelegate::ModernComboItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {}

void ModernComboItemDelegate::setItemRadius(int r)              { m_radius = r; }
void ModernComboItemDelegate::setItemHPad(int px)               { m_hpad = px; }
void ModernComboItemDelegate::setItemVPad(int px)               { m_vpad = px; }
void ModernComboItemDelegate::setItemTextColor(const QColor& c) { m_text = c; }
void ModernComboItemDelegate::setItemBgNormal(const QColor& c)  { m_bgNormal = c; }
void ModernComboItemDelegate::setItemBgHover(const QColor& c)   { m_bgHover = c; }
void ModernComboItemDelegate::setItemBgSelected(const QColor& c){ m_bgSel = c; }
void ModernComboItemDelegate::setItemMinHeight(int h)           { m_minHeight = h; }

QSize ModernComboItemDelegate::sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& idx) const {
    QSize s = QStyledItemDelegate::sizeHint(opt, idx);
    s.setHeight(qMax(s.height(), m_minHeight));
    return s + QSize(0, m_vpad);
}

void ModernComboItemDelegate::paint(QPainter* p, const QStyleOptionViewItem& o, const QModelIndex& idx) const {
    QStyleOptionViewItem opt = o;
    initStyleOption(&opt, idx);

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);

    // Fundo do item (normal/hover/selecionado)
    QRect r = opt.rect.adjusted(2, 1, -2, -1);
    QColor bg = m_bgNormal;
    if (opt.state & QStyle::State_Selected)       bg = m_bgSel;
    else if (opt.state & QStyle::State_MouseOver) bg = m_bgHover;

    p->setPen(Qt::NoPen);
    p->setBrush(bg);
    p->drawRoundedRect(r, m_radius, m_radius);

    // Ícone do item
    int left = r.left() + m_hpad;
    if (!opt.icon.isNull()) {
        const int sz = 18;
        QRect ir(left, r.center().y()-sz/2, sz, sz);
        opt.icon.paint(p, ir);
        left = ir.right() + 8;
    }

    // Texto
    p->setPen(m_text);
    QRect tr(left, r.top(), r.right()-left - m_hpad, r.height());
    p->drawText(tr, Qt::AlignVCenter | Qt::AlignLeft, opt.text);

    p->restore();
}

// ========================== ModernComboBox ==========================

ModernComboBox::ModernComboBox(QWidget* parent)
    : QComboBox(parent)
    , m_delegate(new ModernComboItemDelegate(this))
{
    // Comportamento padrão: NÃO editável
    setEditable(false);
    setInsertPolicy(QComboBox::NoInsert);
    setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    setMinimumContentsLength(0);

    // View: QListView com delegate
    auto* lv = new QListView(this);
    lv->setMouseTracking(true);
    lv->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    lv->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    lv->setItemDelegate(m_delegate);
    setView(lv);

    // Paleta/estilo inicial
    updateStyleSheet();
}

ModernComboBox::~ModernComboBox() = default;

// ---------------- Aparência da caixa ----------------
void ModernComboBox::setPaletteColors(const ComboPalette& pal) {
    m_pal = pal;
    // repassa cores do popup ao delegate
    m_delegate->setItemTextColor(pal.itemText);
    m_delegate->setItemBgNormal(pal.itemBgNormal);
    m_delegate->setItemBgHover(pal.itemBgHover);
    m_delegate->setItemBgSelected(pal.itemBgSelected);
    updateStyleSheet();
    update();
}

void ModernComboBox::setCornerRadius(int r)    { m_radius = r; updateStyleSheet(); update(); }
void ModernComboBox::setBorderWidth(int px)    { m_borderW = px; updateStyleSheet(); update(); }
void ModernComboBox::setPadding(int px)        { m_padding = px; updateStyleSheet(); update(); }
void ModernComboBox::setLeadingIcon(const QIcon& ic) { m_leading = ic; update(); }
void ModernComboBox::setPlaceholderTextModern(const QString& t) { m_placeholder = t; update(); }
void ModernComboBox::setPopupMaxHeight(int px) { m_popupMaxH = px; }

// ---------------- Aparência dos itens (delegate) ----------------
void ModernComboBox::setItemRadius(int r)             { m_delegate->setItemRadius(r); }
void ModernComboBox::setItemHPad(int px)              { m_delegate->setItemHPad(px); }
void ModernComboBox::setItemVPad(int px)              { m_delegate->setItemVPad(px); }
void ModernComboBox::setItemTextColor(const QColor& c){ m_delegate->setItemTextColor(c); }
void ModernComboBox::setItemBgNormal(const QColor& c) { m_delegate->setItemBgNormal(c); }
void ModernComboBox::setItemBgHover(const QColor& c)  { m_delegate->setItemBgHover(c); }
void ModernComboBox::setItemBgSelected(const QColor& c){m_delegate->setItemBgSelected(c); }
void ModernComboBox::setItemMinHeight(int h)          { m_delegate->setItemMinHeight(h); }

// ---------------- Pintura da caixa ----------------
void ModernComboBox::paintEvent(QPaintEvent*) {
    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // fundo + borda
    QRect r = rect().adjusted(1,1,-1,-1);
    p.setPen(QPen(m_pal.border, m_borderW));
    p.setBrush(m_pal.bg);
    p.drawRoundedRect(r, m_radius, m_radius);

    // conteúdo (ícone + texto)
    int left = r.left() + m_padding;

    if (!m_leading.isNull()) {
        const int sz = 16;
        QRect ir(left, r.center().y() - sz/2, sz, sz);
        m_leading.paint(&p, ir);
        left = ir.right() + 6;
    }

    p.setPen(m_pal.text);
    QString text = (currentIndex() >= 0) ? currentText() : m_placeholder;
    int arrowSpace = 16 + 6; // seta + gap
    QRect tr(left, r.top(), r.right() - m_padding - arrowSpace - left, r.height());
    p.drawText(tr, Qt::AlignVCenter | Qt::AlignLeft, text);

    // seta
    QStyleOption arrow;
    arrow.rect  = QRect(r.right() - m_padding - 12, r.center().y()-6, 12, 12);
    arrow.state = QStyle::State_Enabled;
    // pinta a seta com cor custom (desenha PRIMITIVE, depois colore)
    QPixmap pm(12,12); pm.fill(Qt::transparent);
    {
        QPainter pp(&pm);
        style()->drawPrimitive(QStyle::PE_IndicatorArrowDown, &arrow, &pp, this);
    }
    // coloriza o pixmap da seta (tint)
    QImage img = pm.toImage().convertToFormat(QImage::Format_ARGB32);
    for (int y=0;y<img.height();++y){
        QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x=0;x<img.width();++x){
            int a = qAlpha(line[x]);
            line[x] = qRgba(m_pal.arrow.red(), m_pal.arrow.green(), m_pal.arrow.blue(), a);
        }
    }
    p.drawImage(arrow.rect.topLeft(), img);
}

// ---------------- Popup ----------------
void ModernComboBox::showPopup() {
    QComboBox::showPopup();
    if (m_popupMaxH > 0) {
        if (auto* w = view()->window()) {
            const QSize s = w->sizeHint();
            w->resize(qMax(width(), s.width()), qMin(m_popupMaxH, s.height()));
        }
    }
}

bool ModernComboBox::event(QEvent* e) {
    // Mantém responsivo visualmente em hover/focus
    switch (e->type()) {
    case QEvent::Enter:
    case QEvent::Leave:
    case QEvent::FocusIn:
    case QEvent::FocusOut:
        update();
        break;
    default:
        break;
    }
    return QComboBox::event(e);
}

// ---------------- StyleSheet do popup ----------------
void ModernComboBox::updateStyleSheet() {
    // Estiliza apenas o popup; a caixa é pintada via paintEvent.
    setStyleSheet(QString(
                      "QComboBox { background: transparent; border: %1px solid %2; border-radius: %3px; padding-left: %4px; padding-right: %4px; }"
                      "QListView { background: %5; border: 1px solid %2; border-radius: %3px; outline: none; }"
                      ).arg(m_borderW)
                      .arg(m_pal.border.name())
                      .arg(m_radius)
                      .arg(m_padding)
                      .arg(m_pal.bg.name()));
}
