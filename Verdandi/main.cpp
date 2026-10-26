#include "mainwindow.h"

#include <QApplication>
#include <QScreen>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QApplication::setWindowIcon(QIcon(":/icons/celtic.png"));
    MainWindow window;

    QRect screenGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    int width = screenGeometry.width() * 0.8;
    int height = screenGeometry.height() * 0.8;

    int x = screenGeometry.x() + (screenGeometry.width() - width) / 2;
    int y = screenGeometry.y() + (screenGeometry.height() - height) / 2;
    window.setGeometry(x, y, width, height);
    window.show();

    return application.exec();
}
