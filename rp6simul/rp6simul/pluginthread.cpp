#include "pluginthread.h"

#include <signal.h>

namespace {

void threadCancelHandler(int)
{
    int old;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old);
}

}

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

    // SIGUSR1 was blocked by main thread (see main.cpp)
    sigset_t sset;
    sigemptyset(&sset);
    sigaddset(&sset, SIGUSR1);
    pthread_sigmask(SIG_UNBLOCK, &sset, 0);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = threadCancelHandler;
    sigaction(SIGUSR1, &sa, 0);

    mainFunc();
}
