#include <QtGui/QApplication>
#include "syncexclient.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CSyncexClient w;
    w.show();

    return a.exec();
}
