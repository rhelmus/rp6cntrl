#include "RP6ControlLib.h"
#include "RP6I2CmasterTWI.h"
#include "../shared/shared.h"
#include "command.h"
#include "interface.h"
#include "plugin.h"

void FlashLEDs(void)
{   
    externalPort.LEDS = 1;
    outputExt();
    
    do
    {
        mSleep(300); 
        externalPort.LEDS <<= 1;
        outputExt();
    }
    while (externalPort.LEDS < 8);
    
    externalPort.LEDS = 0;
    outputExt();
}

int main(void)
{   
    initRP6Control();

    initI2C();
    
    sound(180,80,25);
    sound(220,80,25);
    
    FlashLEDs();

    // Stopwatch 1: Refresh delay
    // Stopwatch 2: ping
    
    dischargePeakDetector();
   
    for (;;)
    {
        updateInterface();

        checkCommands();
        pluginThink();
        
        task_I2CTWI();
    }
    
    return 0;
}
