#include "mainwindow.h"

#include <QApplication>
#include <QScreen>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    int targetWidth = screenGeometry.width() * 0.8;
    int targetHeight = screenGeometry.height() * 0.8;
    w.resize(targetWidth, targetHeight);
    w.show();
    return QApplication::exec();
}
