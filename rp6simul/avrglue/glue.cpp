#include "glue.h"

#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

namespace NRP6SimulGlue {

void debugOut(const char *s...)
{
    char *txt;
    va_list v;

    va_start(v, s);
    vasprintf(&txt, s, v);
    va_end(v);

    printf("DEBUG: %s\n", txt);
    fflush(stdout);

    free(txt);
}

}

#include <unistd.h>
#include <pthread.h>
#include <sched.h>

namespace {

void timerSigHandler(int)
{
    timespec ts = { 0, 100 };
//    nanosleep(&ts, 0);
    pthread_yield();
}

}

// Exported functions

extern "C" int main();
EXPORT void callAvrMain(void)
{
    // SIGRTMIN was blocked by main thread (see rp6simul/main.cpp)
/*    sigset_t sset;
    sigemptyset(&sset);
    sigaddset(&sset, SIGUSR1);
    pthread_sigmask(SIG_UNBLOCK, &sset, 0);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = timerSigHandler;
    sigaction(SIGUSR1, &sa, 0);

    timer_t timer_id;
    sigevent se;
    se.sigev_notify = SIGEV_SIGNAL;
    se.sigev_signo = SIGUSR1;
    timer_create(CLOCK_MONOTONIC, &se, &timer_id);

    struct itimerspec its;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 1000;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 1000;
    timer_settime(timer_id, 0, &its, 0);*/

    main();
}

EXPORT void setGeneralIOCallbacks(NRP6SimulGlue::CGeneralIO::TSetCallback s,
                                  NRP6SimulGlue::CGeneralIO::TGetCallback g)
{
    NRP6SimulGlue::CGeneralIO::setCallback = s;
    NRP6SimulGlue::CGeneralIO::getCallback = g;
}
