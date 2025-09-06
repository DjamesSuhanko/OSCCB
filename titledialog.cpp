#include "titledialog.h"
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QLabel>

TitleEditDialog::TitleEditDialog(const QString& currentText, QWidget* parent)
    : QDialog(parent)
{
    // Flags que ajudam no Android fullscreen
    setWindowFlag(Qt::Dialog, true);
    setWindowFlag(Qt::WindowTitleHint, true);
    setWindowFlag(Qt::CustomizeWindowHint, true);
    setWindowFlag(Qt::WindowStaysOnTopHint, true); // garante estar na frente
    setModal(true);
    setAttribute(Qt::WA_DeleteOnClose, true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16,16,16,16);
    layout->setSpacing(12);

    auto *label = new QLabel(tr("Novo título:"), this);
    edit = new QLineEdit(this);
    edit->setText(currentText);
    edit->selectAll();

    buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    layout->addWidget(label);
    layout->addWidget(edit);
    layout->addWidget(buttons);

    connect(buttons, SIGNAL(accepted()), this, SLOT(onAccepted()));
    connect(buttons, SIGNAL(rejected()), this, SLOT(onRejected()));
}

QString TitleEditDialog::textValue() const {
    return edit ? edit->text() : QString();
}

void TitleEditDialog::onAccepted() {
    emit textConfirmed(textValue());
    accept();
}

void TitleEditDialog::onRejected() {
    reject();
}
