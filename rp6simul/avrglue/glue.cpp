#include "glue.h"

#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

namespace {

typedef void (*TEnableISRsCallback)(bool);
TEnableISRsCallback enableISRsCallback = 0;
}

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

void cli(void)
{
    if (enableISRsCallback)
        enableISRsCallback(false);
}

void sei(void)
{
    if (enableISRsCallback)
        enableISRsCallback(true);
}

// Exported functions

extern "C" int main();
EXPORT void callAvrMain(void)
{
    main();
}

EXPORT void setPluginCallbacks(NRP6SimulGlue::CIORegister::TSetCallback s,
                               NRP6SimulGlue::CIORegister::TGetCallback g,
                               TEnableISRsCallback i)
{
    NRP6SimulGlue::CIORegister::setCallback = s;
    NRP6SimulGlue::CIORegister::getCallback = g;
    enableISRsCallback = i;
}
