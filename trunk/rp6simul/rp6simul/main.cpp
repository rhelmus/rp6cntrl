#include "rp6simul.h"

#include <signal.h>

#include <QtGui/QApplication>

int main(int argc, char *argv[])
{
    // Restrict SIGUSR1 to 'main AVR thread'
    sigset_t sset;
    sigemptyset(&sset);
    sigaddset(&sset, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &sset, 0);

    QApplication::setGraphicsSystem("raster"); // UNDONE
    QApplication a(argc, argv);
    CRP6Simulator w;
    w.show();

    return a.exec();
}
