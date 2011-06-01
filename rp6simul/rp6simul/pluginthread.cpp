#include "pluginthread.h"

#include <signal.h>

#ifndef Q_OS_WIN32
namespace {

void threadCancelHandler(int)
{
    int old;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old);
}

}
#endif

CCallPluginMainThread::CCallPluginMainThread(TCallPluginMainFunc func,
                                             QObject *parent) :
    QThread(parent), mainFunc(func), threadID(0)
{
}

void CCallPluginMainThread::run()
{
    threadID = QThread::currentThreadId();

    Q_ASSERT(mainFunc != 0);
    setTerminationEnabled(true);

#ifndef Q_OS_WIN32
    // SIGUSR1 was blocked by main thread (see main.cpp)
    sigset_t sset;
    sigemptyset(&sset);
    sigaddset(&sset, SIGUSR1);
    pthread_sigmask(SIG_UNBLOCK, &sset, 0);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = threadCancelHandler;
    sigaction(SIGUSR1, &sa, 0);
#endif

    mainFunc();
}
