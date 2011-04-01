#include <QtGui/QApplication>
#include "rp6simul.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CRP6Simulator w;
    w.show();

    return a.exec();
}
