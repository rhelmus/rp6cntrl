#include "glue.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

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

// Exported functions

extern "C" int main();
EXPORT void callAvrMain(void)
{
    main();
}

EXPORT void setGeneralIOCallbacks(NRP6SimulGlue::CGeneralIO::TSetCallback s,
                                  NRP6SimulGlue::CGeneralIO::TGetCallback g)
{
    NRP6SimulGlue::CGeneralIO::setCallback = s;
    NRP6SimulGlue::CGeneralIO::getCallback = g;
}
