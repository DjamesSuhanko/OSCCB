#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setStyle("Fusion");


    QPalette p;
    p.setColor(QPalette::Window,        QColor("#1f2125"));
    p.setColor(QPalette::Base,          QColor("#2a2d34"));
    p.setColor(QPalette::AlternateBase, QColor("#23262b"));
    p.setColor(QPalette::Button,        QColor("#2f333a"));
    p.setColor(QPalette::Text,          QColor("#e9eef3"));
    p.setColor(QPalette::ButtonText,    QColor("#e9eef3"));
    p.setColor(QPalette::BrightText,    QColor("#ffffff"));
    p.setColor(QPalette::Highlight,     QColor("#3aaed8"));
    p.setColor(QPalette::HighlightedText, QColor("#0b0d10"));
    QApplication::setPalette(p);


    MainWindow w;
    //w.show();
    w.showFullScreen();
    return a.exec();
}
