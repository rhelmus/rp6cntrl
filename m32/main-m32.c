#include "RP6ControlLib.h"
#include "RP6I2CmasterTWI.h"
#include "../shared/shared.h"
#include "command.h"
#include "interface.h"
#include "plugin.h"
#include "servo.h"

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

    // Stopwatch 1: Reserved by servo lib
    // Stopwatch 2: ping
    // Stopwatch 3: Refresh delay
    
    dischargePeakDetector();

    // Init IR turret
    initSERVO(SERVO5);
    setServo(MIDDLE_POSITION); // Place servo at middle
    DDRA &= ~ADC2; // Enable sharp IR for readout

    for (;;)
    {
        updateInterface();

        checkCommands();
        pluginThink();
        
        task_I2CTWI();
        task_SERVO();
    }
    
    return 0;
}
