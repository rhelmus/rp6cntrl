#include <string.h>

#include "RP6ControlLib.h"
#include "../shared/shared.h"
#include "command.h"
#include "interface.h"
#include "plugin.h"
#include "servo.h"

static const char usageStr[] PROGMEM =
    "Usage:\n\n"
    "set\t\tChange settings. See set help.\n"
    "dump\t\tDumps settings. See dump help.\n"
    "plugin\t\tPlugin commands. See plugin help.\n"
    "move\t\tMove the robot. Usage: dist <speed> <dir>\n"
    "rotate\t\tRotates the robot. Usage: angle <speed> <dir>\n"
    "irsend\t\tSend RC5 code. Usage: adr key\n"
    "stop\t\tStops all movement\n"
    "beep\t\tBeeps. Usage: pitch duration\n"
    "sound\t\tBeeps (blocking). Usage: pitch duration delay\n"
    "eeprom\t\tReads EEPROM page. Usage: page.\n";

static const char setUsageStr[] PROGMEM =
    "set usage:\n\n"
    "power\t\tPowers sensors on/off. Usage: boolean\n"
    "leds1\t\tSets leds from base. Usage: binary\n"
    "leds2\t\tSets leds from m32. Usage: binary\n"
    "acs\t\tSets ACS power. Usage: off, low, med or high\n"
    "acsup\t\tSets ACS update interval. Usage: interval\n"
    "acsir\t\tSets ACS IRCOMM delay. Usage: delay\n"
    "acssend\t\tSets ACS pulses send. Usage: pulses\n"
    "acsrec\t\tSets ACS minimal pulses received. Usage: pulses\n"
    "acsrect\t\tSets ACS minimal pulses received threshold. Usage: pulses\n"
    "acsto\t\tSets ACS pulses timeout. Usage: timeout\n"
    "speed\t\tSets move speed. Usage: left-speed right-speed\n"
    "dir\t\tSets move direction. Usage: fwd, bwd, left or right\n"
    "rotf\t\tSets rotation error factor. Usage: factor\n"
    "beep\t\tSets beeper pitch. Usage: pitch\n"
    "slave\t\tEnables or disables slave mode. Usage: boolean.\n"
    "slavemic\t\tSlave mic update time. Usage: time in msec.\n"
    "servo\t\tServo position. Usage: 0..180.\n"
    "srange\t\tServo touch range. Usage: left, right\n"
    "stimer\t\tServo timer1. Usage: timer\n";

static const char dumpUsageStr[] PROGMEM =
    "dump usage:\n\n"
    "<nothing>, all\tDumps everything\n"
    "state\t\tDumps state sensors\n"
    "leds\t\tDumps led states as binary\n"
    "battery\t\tDumps battery state\n"
    "light\t\tDumps light sensors\n"
    "motor\t\tDumps motor info\n"
    "acs\t\tDumps ACS power state\n"
    "acsset\t\tDumps ACS tweaks\n"
    "rc5\t\tDumps last received ir (rc5) info\n"
    "ping\t\tDumps last ping time\n"
    "mic\t\tDumps last mic value\n"
    "key\t\tDumps pressed key\n"
    "sharp\t\tDumps distance from Sharp IR sensor\n";

static const char pluginUsageStr[] PROGMEM =
    "plugin usage:\n\n"
    "load\t\tLoads and starts a plugin. Usage: plugin-name\n"
    "stop\t\tStops and unloads current plugin.\n"
    "list\t\tList all available plugins.\n"
    "bind\t\tBinds a plugin to a key. Usage: key-number plugin-name\n"
    "unbind\t\tUnbinds a key. Usage: key-number\n";
    
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
    if (count && !strcmp_P(cmd[0], PSTR("help")))
    {
        writeNStringP(setUsageStr);
        return;
    }
    
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
    else if (!strcmp_P(cmd[0], PSTR("acs")))
    {
        EACSPowerState state;
        if (!strcmp_P(cmd[1], PSTR("off")))
            state = ACS_POWER_OFF;
        else if (!strcmp_P(cmd[1], PSTR("low")))
            state = ACS_POWER_LOW;
        else if (!strcmp_P(cmd[1], PSTR("med")))
            state = ACS_POWER_MED;
        else if (!strcmp_P(cmd[1], PSTR("high")))
            state = ACS_POWER_HIGH;
        else
        {
            writeString_P("Unknown ACS state\n");
            return;
        }
        setBaseACS(state);
    }
    else if (!strcmp_P(cmd[0], PSTR("acsup")))
        setACSUpdateInterval(atoi(cmd[1]));
    else if (!strcmp_P(cmd[0], PSTR("acsir")))
        setACSIRCOMMWait(atoi(cmd[1]));
    else if (!strcmp_P(cmd[0], PSTR("acssend")))
        setACSPulsesSend(atoi(cmd[1]));
    else if (!strcmp_P(cmd[0], PSTR("acsrec")))
        setACSPulsesRec(atoi(cmd[1]));
    else if (!strcmp_P(cmd[0], PSTR("acsrect")))
        setACSPulsesRecThreshold(atoi(cmd[1]));
    else if (!strcmp_P(cmd[0], PSTR("acsto")))
        setACSPulsesTimeout(atoi(cmd[1]));    
    else if (!strcmp_P(cmd[0], PSTR("speed")))
    {
        if (count < 3)
            writeString_P("Error: need atleast 3 arguments\n");
        else
            setMoveSpeed(atoi(cmd[1]), atoi(cmd[2]));
    }
    else if (!strcmp_P(cmd[0], PSTR("dir")))
        setMoveDirection(getDir(cmd[1]));
    else if (!strcmp_P(cmd[0], PSTR("rotf")))
        setRotationFactor(atoi(cmd[1]));
    else if (!strcmp_P(cmd[0], PSTR("beep")))
        setBeeperPitch(atoi(cmd[1]));
    else if (!strcmp_P(cmd[0], PSTR("slave")))
    {
        extern uint8_t slaveMode;
        slaveMode = cmd[1][0] == '1';
    }
    else if (!strcmp_P(cmd[0], PSTR("slavemic")))
    {
        extern uint16_t slaveMicUpdateTime;
        slaveMicUpdateTime = atoi(cmd[1]);
        setStopwatch4(0);
        if (slaveMicUpdateTime)
            startStopwatch4();
        else
            stopStopwatch4();
    }
    else if (!strcmp_P(cmd[0], PSTR("servo")))
    {
        uint8_t pos = atoi(cmd[1]);
        if (pos > 180)
            pos = 180;

        // Degree --> servo pos
        setServo((uint16_t)pos * RIGHT_TOUCH / 180);
    }
    else if (!strcmp_P(cmd[0], PSTR("srange")))
    {
        if (count < 3)
            writeString_P("Error: need atleast 3 arguments\n");
        else
            setServoRange(atoi(cmd[1]), atoi(cmd[2]));
    }
    else if (!strcmp_P(cmd[0], PSTR("stimer")))
    {
        setServoTimer1(atol(cmd[1]));
    }
}

void handleDumpCommand(const char **cmd, uint8_t count)
{
    if (count && !strcmp_P(cmd[0], PSTR("help")))
    {
        writeNStringP(dumpUsageStr);
        return;
    }
    
    const uint8_t all = ((count < 1) || !strcmp_P(cmd[0], PSTR("all")));
    
    if (all || !strcmp_P(cmd[0], PSTR("state")))
    {
        SStateSensors state = getStateSensors();
        dumpVar_P("Left bumper: ", state.bumperLeft, DEC);
        dumpVar_P("Right bumper: ", state.bumperRight, DEC);
        dumpVar_P("Left ACS: ", state.ACSLeft, DEC);
        dumpVar_P("Right ACS: ", state.ACSRight, DEC);
        dumpVar_P("Movement complete: ", state.movementComplete, DEC);
        dumpVar_P("ACS State: ", state.ACSState, DEC);
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
        dumpVar_P("Motor left distance: ", getLeftMotorDist(), DEC);
        dumpVar_P("Motor right distance: ", getRightMotorDist(), DEC);
        dumpVar_P("Motor left destination distance: ", getLeftMotorDestDist(), DEC);
        dumpVar_P("Motor right destination distance: ", getRightMotorDestDist(), DEC);        
        dumpVar_P("Rotation factor error: ", getRotationFactor(), DEC);
    }
    
    if (all || !strcmp_P(cmd[0], PSTR("acs")))
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

    if (all || !strcmp_P(cmd[0], PSTR("acsset")))
    {
        writeString_P("ACS tweaks:\n");
        dumpVar_P("ACS update interval: ", getACSUpdateInterval(), DEC);
        dumpVar_P("ACS IRCOMM wait: ", getACSIRCOMMWait(), DEC);
        dumpVar_P("ACS pulses send: ", getACSPulsesSend(), DEC);
        dumpVar_P("ACS pulses received: ", getACSPulsesRec(), DEC);
        dumpVar_P("ACS pulses received threshold: ", getACSPulsesRecThreshold(), DEC);
        dumpVar_P("ACS pulses timeout: ", getACSPulsesTimeout(), DEC);
        writeChar('\n');
    }
    
    if (all || !strcmp_P(cmd[0], PSTR("rc5")))
    {
        RC5data_t rc5 = getLastRC5();
        writeString_P("Last received RC5 data:\nDevice: ");
        writeInteger(rc5.device, DEC);
        writeString_P("\nToggle: ");
        writeInteger(rc5.toggle_bit, DEC);
        writeString_P("\nKey code: ");
        writeInteger(rc5.key_code, DEC);
        writeChar('\n');
    }
    
    if (all || !strcmp_P(cmd[0], PSTR("ping")))
        dumpVar_P("Last ping: ", getLastPing(), DEC);
    
    if (all || !strcmp_P(cmd[0], PSTR("mic")))
        dumpVar_P("Last microphone peak: ", getMicrophonePeak(), DEC);
    
    if (all || !strcmp_P(cmd[0], PSTR("key")))
        dumpVar_P("Pressed key: ", getPressedKeyNumber(), DEC);

    if (all || !strcmp_P(cmd[0], PSTR("sharp")))
        dumpVar_P("Sharp IR: ", getSharpIRDistance(), DEC);
}

void handlePluginCommand(const char **cmd, uint8_t count)
{
    if (count && !strcmp_P(cmd[0], PSTR("help")))
    {
        writeNStringP(pluginUsageStr);
        return;
    }
    
    if (count < 1)
    {
        writeString_P("Error: dump needs atleast 1 arg.\n");
        return;
    }
    
    if (!strcmp_P(cmd[0], PSTR("load")))
    {
        if (count > 1)
            startPlugin(cmd[1]);
        else
            writeString_P("Error: need 2 arguments\n");
    }
    else if (!strcmp_P(cmd[0], PSTR("stop")))
        stopPlugin();
    else if (!strcmp_P(cmd[0], PSTR("list")))
        listPlugins();
    else if (!strcmp_P(cmd[0], PSTR("bind")))
    {
        if (count > 2)
            bindPlugin(atoi(cmd[1]), cmd[2]);
        else
            writeString_P("Error: need 3 arguments\n");
    }
    else if (!strcmp_P(cmd[0], PSTR("unbind")))
    {
        if (count > 1)
            unBindKey(atoi(cmd[1]));
        else
            writeString_P("Error: need 2 arguments\n");
    }
}

// cmd[0] = command
// cmd[1-n] = arg
void handleCommand(const char **cmd, uint8_t count)
{
    if (!strcmp_P(cmd[0], PSTR("set")))
        handleSetCommand(&cmd[1], count-1);
    else if (!strcmp_P(cmd[0], PSTR("dump")))
        handleDumpCommand(&cmd[1], count-1);
    else if (!strcmp_P(cmd[0], PSTR("plugin")))
        handlePluginCommand(&cmd[1], count-1);
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
    else if (!strcmp_P(cmd[0], PSTR("irsend")))
    {
        if (count < 3)
            writeString_P("irsend needs atleast 2 arguments\n");
        else
            sendRC5(atoi(cmd[1]), atoi(cmd[2]));
    }
    else if (!strcmp_P(cmd[0], PSTR("stop")))
        stopMovement();
    else if (!strcmp_P(cmd[0], PSTR("beep")))
    {
        if (count < 3)
            writeString_P("beep needs atleast 2 arguments\n");
        else
            beep(atoi(cmd[1]), atoi(cmd[2]));
    }
    else if (!strcmp_P(cmd[0], PSTR("sound")))
    {
        if (count < 4)
            writeString_P("sound needs atleast 3 arguments\n");
        else
            sound(atoi(cmd[1]), atoi(cmd[2]), atoi(cmd[3]));
    }
    else if (!strcmp_P(cmd[0], PSTR("eeprom")))
    {
        if (count < 2)
            writeString_P("eeprom needs atleast 1 argument\n");
        else
        {
            const uint16_t page = atoi(cmd[1]);
            uint8_t buffer[64];
            
            SPI_EEPROM_readBytes(page * 64, buffer, 64);

            writeString_P("EEPROM page contents:\n");
            
            uint8_t i;
            for (i=0; i<64; ++i)
            {
                writeInteger(buffer[i], DEC);
                writeChar(',');
            }
            writeChar('\n');
        }
    }
    else if (!strcmp_P(cmd[0], PSTR("help")))
        writeNStringP(usageStr);
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
