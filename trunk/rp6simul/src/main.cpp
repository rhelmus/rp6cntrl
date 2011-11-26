#include "rp6simul.h"

#include <signal.h>

#include <QtGui/QApplication>

#ifdef Q_OS_WIN // SDL fix
extern "C" int SDL_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
#ifndef Q_OS_WIN
    // Restrict SIGUSR1 to 'main AVR thread'
    sigset_t sset;
    sigemptyset(&sset);
    sigaddset(&sset, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &sset, 0);
#endif

    QApplication::setGraphicsSystem("raster"); // UNDONE: Required on Windows?

    QCoreApplication::setOrganizationName("rp6simul");
    // UNDONE
    QCoreApplication::setOrganizationDomain("code.google.com/p/rp6simul");
    QCoreApplication::setApplicationName("rp6simul");
    QCoreApplication::setApplicationVersion("1.0");

    QApplication a(argc, argv);
    CRP6Simulator w;
    w.show();

    return a.exec();
}

#ifdef Q_OS_WIN
int qMain(int argc, char *argv[]) { return SDL_main(argc, argv); }
#endif
