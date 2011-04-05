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
    main();
}

EXPORT void setIORegisterCallbacks(NRP6SimulGlue::CIORegister::TSetCallback s,
                                  NRP6SimulGlue::CIORegister::TGetCallback g)
{
    NRP6SimulGlue::CIORegister::setCallback = s;
    NRP6SimulGlue::CIORegister::getCallback = g;
}
