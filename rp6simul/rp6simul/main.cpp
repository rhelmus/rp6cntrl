#include "rp6simul.h"

#include <signal.h>

#include <QtGui/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CRP6Simulator w;
    w.show();

    return a.exec();
}
