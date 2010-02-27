#include "RP6ControlLib.h"
#include "interface.h"

static uint8_t rotating;

void acstestStart(void)
{
    setBasePower(true);

    mSleep(15); // Let bot power up
    
    setBaseACS(ACS_POWER_HIGH);
    startStopwatch3();
    setStopwatch3(0);
    
    writeString_P("Started ACS test plugin!\n");
}

void acstestStop(void)
{
    setBasePower(false);
    setBaseACS(ACS_POWER_OFF);
    setBaseLEDs(0);
    
    stopStopwatch3();
    setStopwatch3(0);

    rotating = false;
    
    writeString_P("Stopped ACS test plugin.\n");
}

void rotateThink(void)
{
    #define MAX_SCAN_SIZE 32
    static uint16_t scanBuffer[MAX_SCAN_SIZE];
    static uint8_t index = 0;
    static uint8_t lastleft = false, lastright = false;

    if (!getStateSensors().movementComplete)
    {
        uint8_t left = getStateSensors().ACSLeft, right = getStateSensors().ACSRight;
        
        if ((lastleft != left) || (lastright != right))
        {
            if (index < MAX_SCAN_SIZE)
            {
                // UNDONE
                scanBuffer[index] = (uint16_t)((uint32_t)getLeftMotorDist() * 360 / getLeftMotorDestDist());
                ++index;
            }
        }

        lastleft = left;
        lastright = right;
    }
    else
    {
        rotating = false;

        writeString_P("Results:\nScans: ");
        writeInteger(index, DEC);
        writeString_P("\nScan table: \n");
        uint8_t i;
        for (i=0; i<index; ++i)
        {
            writeInteger(scanBuffer[i], DEC);
            writeString_P(", ");
        }
        writeChar('\n');

        index = lastleft = lastright = 0;
    }
}

void acstestThink(void)
{
    if (getStopwatch3() > 10)
    {
        const uint8_t acs = getACSPowerState();
        uint8_t leds = 0;
       
        if (acs != ACS_POWER_OFF)
        {
            if (getStateSensors().ACSLeft)
            {
                if (acs == ACS_POWER_LOW)
                    leds |= (1<<5);
                else if (acs == ACS_POWER_LOW)
                    leds |= (1<<4);
                else
                    leds |= (1<<3);
            }

            if (getStateSensors().ACSRight)
            {
                if (acs == ACS_POWER_LOW)
                    leds |= (1<<2);
                else if (acs == ACS_POWER_LOW)
                    leds |= (1<<1);
                else
                    leds |= (1<<0);
            }
        }

        setBaseLEDs(leds);
        setStopwatch3(0);
    }

    if (rotating)
        rotateThink();
    else if (getPressedKeyNumber() == 5)
    {
        rotate(75, RIGHT, 360);
        rotating = true;
    }
}
