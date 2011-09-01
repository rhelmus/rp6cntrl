#include "glue.h"

#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

namespace {
NRP6SimulGlue::TEnableISRsCB enableISRsCallback = 0;
}

namespace NRP6SimulGlue {

void *callBackData;

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
        enableISRsCallback(false, NRP6SimulGlue::callBackData);
}

void sei(void)
{
    if (enableISRsCallback)
        enableISRsCallback(true, NRP6SimulGlue::callBackData);
}

// Exported functions

extern "C" int main();
EXPORT void callAvrMain(void)
{
    main();
}

EXPORT void setPluginCallbacks(NRP6SimulGlue::TIORegisterSetCB s,
                               NRP6SimulGlue::TIORegisterGetCB g,
                               NRP6SimulGlue::TEnableISRsCB i, void *d)
{
    NRP6SimulGlue::CIORegister::setCallback = s;
    NRP6SimulGlue::CIORegister::getCallback = g;
    enableISRsCallback = i;
    NRP6SimulGlue::callBackData = d;
}
