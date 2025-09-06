#pragma once
#include <QDialog>

class QLineEdit;
class QDialogButtonBox;

class TitleEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit TitleEditDialog(const QString& currentText,
                             QWidget* parent = nullptr);

    QString textValue() const;

signals:
    void textConfirmed(const QString& text);

private slots:
    void onAccepted();   // OK
    void onRejected();   // Cancel

private:
    QLineEdit* edit = nullptr;
    QDialogButtonBox* buttons = nullptr;
};
