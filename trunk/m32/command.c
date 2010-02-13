#include "RP6ControlLib.h"
#include "../shared/shared.h"
#include "command.h"
#include "interface.h"

// var MUST be in ROM!
void dumpVarP(const char *var, uint16_t val, uint8_t base)
{
    writeNStringP(var);
    writeString_P(": ");
    writeInteger(val, base);
    writeChar('\n');
}

#define dumpVar_P(var, val, base) dumpVarP((PSTR(var)), val, base)

static uint8_t getByte(const char *s, uint8_t bits)
{
    uint8_t ret = 0, i;
    const uint8_t len = strlen(s);
    
    for (i=0; (i < bits) && s[i]; ++i)
    {
        if (s[i] == '1')
            ret |= (1<<(len-1-i));
    }
    
    return ret;
}

static uint8_t getDir(const char *s)
{
    uint8_t ret = FWD; // default
    
    if (!strcmp_P(s, PSTR("fwd")))
        ret = FWD;
    else if (!strcmp_P(s, PSTR("bwd")))
        ret = BWD;
    else if (!strcmp_P(s, PSTR("left")))
        ret = LEFT;
    else if (!strcmp_P(s, PSTR("right")))
        ret = RIGHT;
    
    return ret;
}

void handleSetCommand(const char **cmd, uint8_t count)
{
    if (count < 2)
    {
        writeString_P("Error: set needs atleast 2 args.\n");
        return;
    }
    
    if (!strcmp_P(cmd[0], PSTR("power")))
        setBasePower(cmd[1][0] == '1');
    else if (!strcmp_P(cmd[0], PSTR("leds1")))
        setBaseLEDs(getByte(cmd[1], 6));
    else if (!strcmp_P(cmd[0], PSTR("leds2")))
    {
        externalPort.LEDS = getByte(cmd[1], 4);
        outputExt();
    }
}

void handleDumpCommand(const char **cmd, uint8_t count)
{
    const uint8_t all = ((count < 1) || !strcmp_P(cmd[0], "all"));
    
    if (all || !strcmp_P(cmd[0], PSTR("state")))
    {
        EStateSensors state = getStateSensors();
        dumpVar_P("Left bumper: ", state.bumperLeft, DEC);
        dumpVar_P("Right bumper: ", state.bumperRight, DEC);
        dumpVar_P("Left ACS: ", state.ACSLeft, DEC);
        dumpVar_P("Right ACS: ", state.ACSRight, DEC);
        dumpVar_P("Movement complete: ", state.movementComplete, DEC);
    }
    
    if (all || !strcmp_P(cmd[0], PSTR("leds")))
    {
        dumpVar_P("base leds: ", getBaseLEDs(), BIN);
        dumpVar_P("m32 leds: ", externalPort.LEDS, BIN);
    }
    
    if (all || !strcmp_P(cmd[0], PSTR("battery")))
        dumpVar_P("Battery: ", getBattery(), DEC);
    
    if (all || !strcmp_P(cmd[0], PSTR("light")))
    {
        dumpVar_P("Left light: ", getLeftLightSensor(), DEC);
        dumpVar_P("Right light: ", getRightLightSensor(), DEC);
    }
    
    if (all || !strcmp_P(cmd[0], PSTR("motor")))
    {
        dumpVar_P("Motor left: ", getLeftMotorSpeed(), DEC);
        dumpVar_P("Motor right: ", getRightMotorSpeed(), DEC);
        dumpVar_P("Motor dest left: ", getLeftDestMotorSpeed(), DEC);
        dumpVar_P("Motor dest right: ", getRightDestMotorSpeed(), DEC);
        dumpVar_P("Motor left current: ", getLeftMotorCurrent(), DEC);
        dumpVar_P("Motor right current: ", getRightMotorCurrent(), DEC);
    }
    
    if (all || !strcmp_P(cmd[0], PSTR("ACS")))
    {
        writeString_P("ACS: ");
        switch (getACSPowerState())
        {
            case ACS_POWER_OFF: writeString_P("off"); break;
            case ACS_POWER_LOW: writeString_P("low"); break;
            case ACS_POWER_MED: writeString_P("medium"); break;
            case ACS_POWER_HIGH: writeString_P("high"); break;
        }
        writeChar('\n');
    }
    
    if (all || !strcmp_P(cmd[0], PSTR("RC5")))
        dumpVar_P("Last received RC5 key: ", getLastRC5Key(), DEC);
    
    if (all || !strcmp_P(cmd[0], PSTR("ping")))
        dumpVar_P("Last ping: ", getLastPing(), DEC);
}

// cmd[0] = command
// cmd[1-n] = arg
void handleCommand(const char **cmd, uint8_t count)
{
    if (!strcmp_P(cmd[0], PSTR("set")))
        handleSetCommand(&cmd[1], count-1);
    else if (!strcmp_P(cmd[0], PSTR("dump")))
        handleDumpCommand(&cmd[1], count-1);
    else if (!strcmp_P(cmd[0], PSTR("move")))
    {
        if (count < 2)
        {
            writeString_P("move needs atleast 1 argument\n");
            return;
        }
        
        uint8_t speed = 80, dir = FWD;
        uint16_t dist = atoi(cmd[1]);
        
        if (count > 2)
            speed = atoi(cmd[2]);
        
        if (count > 3)
            dir = getDir(cmd[3]);
        
        move(speed, dir, dist);
    }
    else if (!strcmp_P(cmd[0], PSTR("rotate")))
    {
        if (count < 2)
        {
            writeString_P("rotate needs atleast 1 argument\n");
            return;
        }
        
        uint8_t speed = 80, dir = RIGHT;
        uint16_t angle = atoi(cmd[1]);
        
        if (count > 2)
            speed = atoi(cmd[2]);
        
        if (count > 3)
            dir = getDir(cmd[3]);
        
        rotate(speed, dir, angle);
    }
}

void checkCommands(void)
{
    #define MAXBUFFER 32
    static char buffer[MAXBUFFER] = { 0 };
    static uint8_t bufindex = 0;
    
    while (getBufferLength())
    {
        if ((bufindex + 2) >= MAXBUFFER) //+2: space for '0'
        {
            // Just discard the rest and fake a newline
            buffer[MAXBUFFER-2] = '\n';
            bufindex = MAXBUFFER-2;
            break;
        }
        
        buffer[bufindex] = readChar();
        
        if (buffer[bufindex] == '\n')
            break;
        
        bufindex++;
    }
    
    if (buffer[bufindex] == '\n')
    {
        buffer[bufindex+1] = 0;
        bufindex = 0;
        
        #define MAX_ARGS 6
        static const char *commands[MAX_ARGS];
        char *word, *toks = 0;
        uint8_t count = 0;
        
        word = strtok_r(buffer, " \n", &toks);
        while (word && (count < MAX_ARGS))
        {
            commands[count] = word;
            count++;
            word = strtok_r(NULL, " \n", &toks);
        }
        
        if (count)
            handleCommand(commands, count);
        
        buffer[0] = 0;
    }
}
