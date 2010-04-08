#include "RP6ControlLib.h"
#include "interface.h"

static uint8_t rotating;

void acstestStart(void)
{
    setBasePower(true);

    mSleep(15); // Let bot power up
    
    setBaseACS(ACS_POWER_HIGH);
    startDynStopwatch(0);
    setDynStopwatch(0, 0);
    
    writeString_P("Started ACS test plugin!\n");
}

void acstestStop(void)
{
    setBasePower(false);
    setBaseACS(ACS_POWER_OFF);
    setBaseLEDs(0);
    
    stopDynStopwatch(0);
    setDynStopwatch(0, 0);
    stopDynStopwatch(1);
    setDynStopwatch(1, 0);
    
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
    static uint8_t statecycles = 0, prevacsstate;
    uint8_t acs = getACSPowerState();

    if (getDynStopwatch(0) > 5)
    {
        uint8_t leds = 0;

        if (acs != ACS_POWER_OFF)
        {
            if (getStateSensors().ACSLeft)
            {
                if (acs == ACS_POWER_LOW)
                    leds |= (1<<5);
                else if (acs == ACS_POWER_MED)
                    leds |= (1<<4);
                else
                    leds |= (1<<3);
            }

            if (getStateSensors().ACSRight)
            {
                if (acs == ACS_POWER_LOW)
                    leds |= (1<<2);
                else if (acs == ACS_POWER_MED)
                    leds |= (1<<1);
                else
                    leds |= (1<<0);
            }
        }

        if (getBaseLEDs() != leds)
            setBaseLEDs(leds);
        setDynStopwatch(0, 0);
    }

    if (getDynStopwatch(1) > 100)
    {
        if (!statecycles && (getStateSensors().ACSState < 2))
        {
            if (acs == ACS_POWER_HIGH)
                acs = ACS_POWER_LOW;
            else
                ++acs;
            setBaseACS(acs);
            statecycles = 2; // Amount of ACS cycles required before next switch
            setDynStopwatch(1, 0);
        }
    }

    if (statecycles && (prevacsstate != ACS_STATE_WAIT_RIGHT) &&
        (getStateSensors().ACSState == ACS_STATE_WAIT_RIGHT))
    {
        --statecycles; // Reached another state cycle
        writeString_P("cycle time: ");
        writeInteger(getDynStopwatch(1), DEC);
        writeString_P("ms <= ");
        writeInteger(statecycles, DEC);
        writeChar('\n');
    }
        
    if (rotating)
        rotateThink();
    else if (getPressedKeyNumber() == 5)
    {
        rotate(75, RIGHT, 360);
        rotating = true;
    }
    else if ((getPressedKeyNumber() == 4) && !isDynStopwatchRunning(1))
    {
        startDynStopwatch(1);
        setDynStopwatch(1, 0);
    }

    prevacsstate = getStateSensors().ACSState;
}
