#include "mainwindow.h"

#include <QMainWindow>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    /* Names to use later for QSettings */
    QCoreApplication::setApplicationName("Pogeymon-Online");
    QCoreApplication::setOrganizationName("Dreambelievers");
    a.setWindowIcon(QIcon("db/icon.png"));

    MainWindow w;

    w.show();
    return a.exec();
}
