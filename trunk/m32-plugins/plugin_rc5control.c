#include "RP6ControlLib.h"
#include "interface.h"

// Key codes from a Samsung remote controller.
enum
{
    KEY_CHUP = 0x10,
    KEY_CHDOWN = 0x11,
    KEY_VOLUP = 0x16,
    KEY_VOLDOWN = 0x15,
    KEY_POWER = 0x0C,
    KEY_1 = 0x01,
    KEY_2 = 0x02,
    KEY_3 = 0x03,
    KEY_4 = 0x04,
    KEY_5 = 0x05,
    KEY_6 = 0x06,
    KEY_7 = 0x07,
    KEY_8 = 0x08,
    KEY_9 = 0x09,
    KEY_COLOR_1 = 0x2B,
    KEY_COLOR_2 = 0x2C,
    KEY_COLOR_3 = 0x2D,
    KEY_COLOR_4 = 0x2E,
    KEY_TV = 0x3F,
    KEY_VIDEO = 0x38,
    KEY_CHSCAN = 0x1E,
    KEY_PRECH = 0x31,
    KEY_MIN = 0x0A,
    KEY_HELP = 0x2F,
    KEY_DISPLAY = 0x0F,
    KEY_MENU = 0x12,

    // Some generic settings and stuff :)
    KEY_SETTINGS_1 = 0x28,
    KEY_SETTINGS_2 = 0x2F, // Double with KEY_HELP
    KEY_SETTINGS_3 = 0x2B, // Double with KEY_COLOR_1
    KEY_SETTINGS_4 = 0x3C,
    KEY_SETTINGS_5 = 0x3A,
    KEY_SETTINGS_6 = 0x2D, // Double with KEY_COLOR_3
    KEY_SETTINGS_7 = 0x29,
    KEY_SETTINGS_8 = 0x2A,
    KEY_SETTINGS_9 = 0x0E,
    KEY_SETTINGS_10 = 0x26,
    KEY_SETTINGS_11 = 0x0D,
    KEY_SETTINGS_12 = 0x16, // Double with KEY_VOLUP
};

enum
{
    START_SPEED = 60,
    MAX_SPEED = 160,
    SPEED_CHANGE = 5, // Speed change on button press
    SPEED_MIN = 30, // Minimal speed before changing direction
};

typedef enum
{
    LEDS_OFF,
    LEDS_BASE_TOGGLED,
    LEDS_CNTRL_TOGGLED,
    LEDS_RUNNING,
} ELEDState;

static int16_t controlForwardSpeed, controlRotateSpeed;
static uint8_t controlRotate;
static ELEDState LEDState;

void modifySpeedVar(int16_t *var, int8_t delta)
{
    *var += delta;

    if (delta > 0)
    {
        if ((*var < 0) && (-(*var) < SPEED_MIN))
            *var = -(*var);
        else if ((*var > 0) && (*var < SPEED_MIN))
            *var = SPEED_MIN;
    }
    else
    {
        if ((*var > 0) && (*var < SPEED_MIN))
            *var = -(*var);
        else if ((*var < 0) && (-(*var) < SPEED_MIN))
            *var = -SPEED_MIN;
    }

    if (*var < -MAX_SPEED)
        *var = -MAX_SPEED;
    else if (*var > MAX_SPEED)
        *var = MAX_SPEED;
}

void updateSpeed(uint8_t dir)
{
    if (getStateSensors().bumperLeft || getStateSensors().bumperRight)
        return;

    // Change from left/right to up/down?   
    if (((dir == FWD) || (dir == BWD)) && controlRotate)
        controlRotateSpeed = 0;
    
    if (dir == FWD)
        modifySpeedVar(&controlForwardSpeed, SPEED_CHANGE);
    else if (dir == BWD)
        modifySpeedVar(&controlForwardSpeed, -SPEED_CHANGE);
    else if (dir == RIGHT)
        modifySpeedVar(&controlRotateSpeed, SPEED_CHANGE);
    else if (dir == LEFT)
        modifySpeedVar(&controlRotateSpeed, -SPEED_CHANGE);
   
    controlRotate = ((dir == LEFT) || (dir == RIGHT));
    
    if (controlForwardSpeed < 0)
        setMoveDirection(BWD);
    else
        setMoveDirection(FWD);
    

    if (controlRotate)
    {
        const uint8_t speed = abs(controlForwardSpeed);
        uint8_t left, right;

        if (controlRotateSpeed < 0) // Left
        {
             left = speed - ((speed < -controlRotateSpeed) ? speed : -controlRotateSpeed);
             right = speed;
        }
        else // right
        {
            left = speed;
            right = speed - ((speed < controlRotateSpeed) ? speed : controlRotateSpeed);
        }

        setMoveSpeed(left, right);
    }
    else
    {
        const uint8_t speed = abs(controlForwardSpeed);
        setMoveSpeed(speed, speed);
    }
    
    writeString_P("Updated speed: ");
    writeInteger(controlForwardSpeed, DEC);
    writeString_P(", ");
    writeInteger(controlRotateSpeed, DEC);
    writeChar('\n');
}

void doRotate(int16_t angle)
{
    // Reset manual speed settings
    controlForwardSpeed = START_SPEED;
    controlRotateSpeed = 0;
    controlRotate = false;

    rotate(80, (angle < 0) ? LEFT : RIGHT, abs(angle));
}

void updateLEDs(ELEDState state)
{
    if (state == LEDState) // Toggle
    {
        LEDState = LEDS_OFF;
        setLEDs(0);
        setBaseLEDs(0);
        stopStopwatch4();
        setStopwatch4(0);
    }
    else
    {
        LEDState = state;

        if (state == LEDS_BASE_TOGGLED)
            setBaseLEDs(255);
        else if (state == LEDS_CNTRL_TOGGLED)
            setLEDs(255);
        else if (state == LEDS_RUNNING)
        {
            setLEDs(0);
            setBaseLEDs(0);
            startStopwatch4();
        }
    }
}

void parseRC5Key(uint8_t key)
{
    switch (key)
    {
        case KEY_CHUP: updateSpeed(FWD); break;
        case KEY_CHDOWN: updateSpeed(BWD); break;
        case KEY_VOLUP: updateSpeed(RIGHT); break;
        case KEY_VOLDOWN: updateSpeed(LEFT); break;
        case KEY_POWER: stopMovement(); break;
        
        case KEY_1: beep(50, 200); break;
        case KEY_2: beep(75, 200); break;
        case KEY_3: beep(100, 200); break;
        case KEY_4: beep(125, 200); break;
        case KEY_5: beep(150, 200); break;
        case KEY_6: beep(175, 200); break;
        case KEY_7: beep(200, 200); break;
        case KEY_8: beep(225, 200); break;
        case KEY_9: beep(255, 200); break;

        case KEY_COLOR_1: doRotate(-45); break;
        case KEY_COLOR_2: doRotate(-90); break;
        case KEY_COLOR_3: doRotate(90); break;
        case KEY_COLOR_4: doRotate(45); break;

        case KEY_TV: updateLEDs(LEDS_BASE_TOGGLED); break;
        case KEY_VIDEO: updateLEDs(LEDS_CNTRL_TOGGLED); break;
        case KEY_CHSCAN: updateLEDs(LEDS_RUNNING); break;
    }
}

void rc5controlStart(void)
{
    setBasePower(true);

    mSleep(15); // Let bot power up
    
    setBaseACS(ACS_POWER_HIGH);
    controlForwardSpeed = START_SPEED;
    controlRotateSpeed = 0;
    controlRotate = false;
    LEDState = LEDS_OFF;

    // RC5 poll timer
    setStopwatch3(0);
    startStopwatch3();

    // LEDs update timer
    setStopwatch4(0);
    
    writeString_P("Started RC5 Control plugin!\n");
}

void rc5controlStop(void)
{
    stopMovement();
    setBasePower(false);
    setBaseACS(ACS_POWER_OFF);
    setBaseLEDs(0);
    setLEDs(0);
    LEDState = LEDS_OFF;

    stopStopwatch3();
    setStopwatch3(0);
    stopStopwatch4();
    setStopwatch4(0);
    
    writeString_P("Stopped RC5 Control plugin.\n");
}

void rc5controlThink(void)
{
    if (getStopwatch3() > 50)
    {
        RC5data_t rc5 = getLastRC5();
        if (rc5.key_code)
        {
            writeString_P("Received RC5 (device, toggle, key): ");
            writeInteger(rc5.device, HEX);
            writeString_P(", ");
            writeInteger(rc5.toggle_bit, DEC);
            writeString_P(", ");
            writeInteger(rc5.key_code, HEX);
            writeChar('\n');
            
            parseRC5Key(rc5.key_code);
            resetLastRC5();
        }

        setStopwatch3(0);
    }

    if ((LEDState == LEDS_RUNNING) && (getStopwatch4() > 150))
    {
        if (externalPort.byte)
        {
            externalPort.byte <<= 1;
            if (externalPort.byte > 8)
            {
                externalPort.byte = 0;
                setBaseLEDs(1);
            }
            outputExt();
        }
        else
        {
            uint8_t baseleds = getBaseLEDs();
            if (!baseleds)
                baseleds = 1;
            else
            {
                baseleds <<= 1;
                if (baseleds > 32)
                {
                    baseleds = 0;
                    setLEDs(1);
                }
            }

            setBaseLEDs(baseleds);
        }

        setStopwatch4(0);
    }
}
