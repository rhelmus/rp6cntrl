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
};

enum
{
    START_SPEED = 60,
    MAX_SPEED = 160,
    SPEED_CHANGE = 5, // Speed change on button press
    SPEED_MIN = 30, // Minimal speed before changing direction
};

static int16_t controlSpeed;
static uint8_t controlRotate;

void updateSpeed(uint8_t dir)
{
    if (getStateSensors().bumperLeft || getStateSensors().bumperRight)
        return;

    // Change from up/down to left/right ?
    if (((dir == FWD) || (dir == BWD)) && controlRotate)
    {
        if (dir == FWD)
            controlSpeed = START_SPEED;
        else
            controlSpeed = -START_SPEED;
    }
    // Change from left/right to up/down?
    else if (((dir == LEFT) || (dir == RIGHT)) && !controlRotate)
    {
        if (dir == RIGHT)
            controlSpeed = START_SPEED;
        else
            controlSpeed = -START_SPEED;  
    }
    else // Increase/Decrease speed
    {
        if ((dir == FWD) || (dir == RIGHT))
        {
            controlSpeed += SPEED_CHANGE;
            if ((controlSpeed < 0) && (-controlSpeed < SPEED_MIN))
                controlSpeed = -controlSpeed;
        }
        else
        {
            controlSpeed -= SPEED_CHANGE;
            if ((controlSpeed > 0) && (controlSpeed < SPEED_MIN))
                controlSpeed = -controlSpeed;
        }
    }

    if (controlSpeed < -MAX_SPEED)
        controlSpeed = -MAX_SPEED;
    else if (controlSpeed > MAX_SPEED)
        controlSpeed = MAX_SPEED;

    controlRotate = ((dir == LEFT) || (dir == RIGHT));

    if (controlRotate)
    {
        if (controlSpeed < 0)
            setMoveDirection(LEFT);
        else
            setMoveDirection(RIGHT);
    }
    else
    {
        if (controlSpeed < 0)
            setMoveDirection(BWD);
        else
            setMoveDirection(FWD);
    }

    const uint8_t speed = abs(controlSpeed);
    setMoveSpeed(speed, speed);
    
    writeString_P("Updated speed: ");
    writeInteger(controlSpeed, DEC);
    writeChar('\n');
}

void parseRC5Key(uint8_t key)
{
    writeString_P("Received RC5 key: ");
    writeInteger(key, HEX);
    writeChar('\n');

    switch (key)
    {
        case KEY_CHUP: updateSpeed(FWD); break;
        case KEY_CHDOWN: updateSpeed(BWD); break;
        case KEY_VOLUP: updateSpeed(RIGHT); break;
        case KEY_VOLDOWN: updateSpeed(LEFT); break;
        case KEY_POWER: stopMovement(); break;
    }
}

void rc5controlStart(void)
{
    setBasePower(true);

    mSleep(15); // Let bot power up
    
    setBaseACS(ACS_POWER_HIGH);
    setStopwatch3(0);
    startStopwatch3();
    controlSpeed = START_SPEED;
    controlRotate = false;
    writeString_P("Started RC5 Control plugin!\n");
}

void rc5controlStop(void)
{
    stopMovement();
    setBasePower(false);
    setBaseACS(ACS_POWER_OFF);
    setStopwatch3(0);
    stopStopwatch3();
    writeString_P("Stopped RC5 Control plugin.\n");
}

void rc5controlThink(void)
{
    if (getStopwatch3() > 50)
    {
        RC5data_t rc5 = getLastRC5();
        if (rc5.key_code)
        {
            parseRC5Key(rc5.key_code);
            resetLastRC5();
        }

        setStopwatch3(0);
    }
}
