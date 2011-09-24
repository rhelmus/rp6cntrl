#include "glue.h"

namespace {
NRP6SimulGlue::TEnableISRsCB enableISRsCallback = 0;
}

namespace NRP6SimulGlue {
void *callBackData;
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
