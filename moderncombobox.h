#pragma once

#include <QComboBox>
#include <QStyledItemDelegate>
#include <QColor>
#include <QIcon>

/**
 * ModernComboBox — apenas aparência (Qt Widgets, Qt 5.12+ / Qt 6)
 * - Não-editável; comportamento padrão do QComboBox.
 * - Paleta e layout personalizáveis.
 * - Itens do popup estilizados por delegate (hover/seleção/altura).
 */

class ModernComboItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit ModernComboItemDelegate(QObject* parent=nullptr);

    // Estilo dos itens
    void setItemRadius(int r);
    void setItemHPad(int px);
    void setItemVPad(int px);
    void setItemTextColor(const QColor& c);
    void setItemBgNormal(const QColor& c);
    void setItemBgHover(const QColor& c);
    void setItemBgSelected(const QColor& c);
    void setItemMinHeight(int h);

    // QStyledItemDelegate
    QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override;
    void  paint(QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const override;

private:
    QColor m_text      {Qt::white};
    QColor m_bgNormal  {QColor("#1e1e1e")};
    QColor m_bgHover   {QColor("#2a2a2a")};
    QColor m_bgSel     {QColor("#00C853")};
    int    m_radius    {8};
    int    m_hpad      {12};
    int    m_vpad      {6};
    int    m_minHeight {28};
};


class ModernComboBox : public QComboBox {
    Q_OBJECT
public:
    explicit ModernComboBox(QWidget* parent=nullptr);
    ~ModernComboBox() override;

    struct ComboPalette {
        QColor bg;       // fundo da caixa
        QColor border;   // cor da borda
        QColor text;     // cor do texto atual
        QColor arrow;    // cor da seta
        // Cores do popup (itens)
        QColor itemBgNormal;
        QColor itemBgHover;
        QColor itemBgSelected;
        QColor itemText;
    };

    // Aparência da caixa
    void setPaletteColors(const ComboPalette& pal);
    void setCornerRadius(int r);      // raio da caixa
    void setBorderWidth(int px);      // largura da borda
    void setPadding(int px);          // padding interno (esq/dir)
    void setLeadingIcon(const QIcon& ic); // ícone à esquerda (opcional)
    void setPlaceholderTextModern(const QString& t); // texto quando sem seleção
    void setPopupMaxHeight(int px);   // altura máx. do popup (opcional)

    // Aparência dos itens (repasse ao delegate)
    void setItemRadius(int r);
    void setItemHPad(int px);
    void setItemVPad(int px);
    void setItemTextColor(const QColor& c);
    void setItemBgNormal(const QColor& c);
    void setItemBgHover(const QColor& c);
    void setItemBgSelected(const QColor& c);
    void setItemMinHeight(int h);

protected:
    void paintEvent(QPaintEvent*) override;
    void showPopup() override;
    bool event(QEvent* e) override;

private:
    void updateStyleSheet();  // aplica ao popup (QListView)

    // estado visual
    ComboPalette m_pal {
        QColor("#1f1f1f"), // bg
        QColor("#3a3a3a"), // border
        QColor("#ffffff"), // text
        QColor("#ffffff"), // arrow
        // itens
        QColor("#1f1f1f"), // item bg normal
        QColor("#2a2a2a"), // item bg hover
        QColor("#00C853"), // item bg selected
        QColor("#ffffff")  // item text
    };
    int   m_radius  {10};
    int   m_borderW {1};
    int   m_padding {10};
    int   m_popupMaxH {260};
    QIcon m_leading;
    QString m_placeholder;

    ModernComboItemDelegate* m_delegate {nullptr};
};
