#include "glue.h"

#include "RP6Config.h"

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

#ifdef RP6_BUILD

EXPORT float getEncoderResolution()
{
    return ENCODER_RESOLUTION;
}

EXPORT int getRotationFactor()
{
    return ROTATION_FACTOR;
}

EXPORT int getSpeedTimerBase()
{
    return SPEED_TIMER_BASE;
}

EXPORT int getACSSendPulsesLeft()
{
    return ACS_SEND_PULSES_LEFT;
}

EXPORT int getACSRecPulsesLeft()
{
    return ACS_REC_PULSES_LEFT;
}

EXPORT int getACSSendPulsesRight()
{
    return ACS_SEND_PULSES_RIGHT;
}

EXPORT int getACSRecPulsesRight()
{
    return ACS_REC_PULSES_RIGHT;
}

#endif // RP6_BUILD
