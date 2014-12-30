#include "mainwindow.h"
#include <QtGui/QApplication>
#include <ctime>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow wgt;

    qsrand(time(0));

    wgt.show();
    return app.exec();
}
