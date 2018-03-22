#include <QtGui/QApplication>
#include "syncexserver.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CSyncexServer w;
    w.show();

    return a.exec();
}
