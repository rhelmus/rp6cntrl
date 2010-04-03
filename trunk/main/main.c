#include "RP6RobotBaseLib.h"
#include "RP6I2CslaveTWI.h" 
#include "../shared/shared.h"

SStateSensors stateSensors;
EACSPowerState ACSPowerState;
RC5data_t lastRC5Data;

// Modified lib source so that ROTATION_FACTOR points to this var
uint16_t rotationFactor = 750; // Originally 688

// ACS Settings. Modified lib header to use following variables.
uint8_t ACSUpdateInterval = 2;
uint8_t ACSIRCOMMWaitTime = 20;

uint8_t ACSSendPulsesLeft = 40;
uint8_t ACSRecPulsesLeft = 6;
uint8_t ACSRecPulsesLeftThreshold = 2;
uint8_t ACSTimeOutLeft = 14;

uint8_t ACSSendPulsesRight = 40;
uint8_t ACSRecPulsesRight = 6;
uint8_t ACSRecPulsesRightThreshold = 2;
uint8_t ACSTimeOutRight = 14;

// From lib
extern uint16_t distanceToMove_L;
extern uint16_t distanceToMove_R;
extern uint8_t acs_state;


void FlashLEDs(void)
{   
    for (statusLEDs.byte=1; statusLEDs.byte<=32; statusLEDs.byte<<=1)
    {
        updateStatusLEDs();
        mSleep(300);
    }
    
    statusLEDs.byte = 0;
    updateStatusLEDs();
}

void setACSState(EACSPowerState state)
{
    ACSPowerState = state;
    switch (state)
    {
        case ACS_POWER_OFF:
            disableACS();
            setACSPwrOff();
            break;
        case ACS_POWER_LOW:
            enableACS();
            setACSPwrLow();
            break;
        case ACS_POWER_MED:
            enableACS();
            setACSPwrMed();
            break;
        case ACS_POWER_HIGH:
            enableACS();
            setACSPwrHigh();
            break;
        default: break;
    }
}

void updateStateSensors(void)
{
    SStateSensors temp;
    temp.bumperLeft = bumper_left;
    temp.bumperRight = (bumper_right != 0); // bumper_right can be >1!?
    temp.ACSLeft = obstacle_left;
    temp.ACSRight = obstacle_right;
    temp.movementComplete = isMovementComplete();
    temp.ACSState = acs_state;

    const uint8_t signal = (temp.byte != stateSensors.byte);
    stateSensors.byte = temp.byte;

    if (signal)
        extIntON(); // Signal external interrupt to notify change
}

void updateI2CReadRegisters(void)
{
    uint8_t i = 0;
    for (;((i<I2C_MAX_INDEX) && !I2CTWI_readBusy); ++i)
    {
        switch (i)
        {
            case I2C_STATE_SENSORS: I2CTWI_readRegisters[i] = stateSensors.byte; break;
            
            case I2C_LEDS: I2CTWI_readRegisters[i] = statusLEDs.byte; break;
            
            case I2C_LIGHT_LEFT_LOW: I2CTWI_readRegisters[i] = adcLSL; break;
            case I2C_LIGHT_LEFT_HIGH: I2CTWI_readRegisters[i] = adcLSL>>8; break;
            case I2C_LIGHT_RIGHT_LOW: I2CTWI_readRegisters[i] = adcLSR; break;
            case I2C_LIGHT_RIGHT_HIGH: I2CTWI_readRegisters[i] = adcLSR>>8; break;
            
            case I2C_MOTOR_SPEED_LEFT:
                I2CTWI_readRegisters[i] = getLeftSpeed();
                break;
            case I2C_MOTOR_SPEED_RIGHT:
                I2CTWI_readRegisters[i] = getRightSpeed();
                break;
            case I2C_MOTOR_DESTSPEED_LEFT:
                I2CTWI_readRegisters[i] = getDesSpeedLeft();
                break;
            case I2C_MOTOR_DESTSPEED_RIGHT:
                I2CTWI_readRegisters[i] = getDesSpeedRight();
                break;
            
            case I2C_MOTOR_CURRENT_LEFT_LOW:
                I2CTWI_readRegisters[i] = adcMotorCurrentLeft;
                break;
            case I2C_MOTOR_CURRENT_LEFT_HIGH:
                I2CTWI_readRegisters[i] = adcMotorCurrentLeft >> 8;
                break;
            case I2C_MOTOR_CURRENT_RIGHT_LOW:
                I2CTWI_readRegisters[i] = adcMotorCurrentRight;
                break;
            case I2C_MOTOR_CURRENT_RIGHT_HIGH:
                I2CTWI_readRegisters[i] = adcMotorCurrentRight >> 8;
                break;

            case I2C_MOTOR_LEFT_DIST_LOW:
                I2CTWI_readRegisters[i] = getLeftDistance();
                break;
            case I2C_MOTOR_LEFT_DIST_HIGH:
                I2CTWI_readRegisters[i] = getLeftDistance() >> 8;
                break;
            case I2C_MOTOR_RIGHT_DIST_LOW:
                I2CTWI_readRegisters[i] = getRightDistance();
                break;
            case I2C_MOTOR_RIGHT_DIST_HIGH:
                I2CTWI_readRegisters[i] = getRightDistance() >> 8;
                break;

            case I2C_MOTOR_LEFT_DESTDIST_LOW:
                I2CTWI_readRegisters[i] = distanceToMove_L;
                break;
            case I2C_MOTOR_LEFT_DESTDIST_HIGH:
                I2CTWI_readRegisters[i] = distanceToMove_L >> 8;
                break;
            case I2C_MOTOR_RIGHT_DESTDIST_LOW:
                I2CTWI_readRegisters[i] = distanceToMove_R;
                break;
            case I2C_MOTOR_RIGHT_DESTDIST_HIGH:
                I2CTWI_readRegisters[i] = distanceToMove_R >> 8;
                break;
                
            case I2C_ROTATE_FACTOR_LOW:
                I2CTWI_readRegisters[i] = rotationFactor;
                break;
            case I2C_ROTATE_FACTOR_HIGH:
                I2CTWI_readRegisters[i] = rotationFactor >> 8;
                break;
                
            case I2C_BATTERY_LOW: I2CTWI_readRegisters[i] = adcBat; break;
            case I2C_BATTERY_HIGH: I2CTWI_readRegisters[i] = adcBat>>8; break;

            case I2C_ACS_POWER: I2CTWI_readRegisters[i] = (uint8_t)ACSPowerState; break;
            
            case I2C_ACS_UPDATE_INTERVAL:
                I2CTWI_readRegisters[i] = ACSUpdateInterval;
                break;
            case I2C_ACS_IRCOMM_WAIT:
                I2CTWI_readRegisters[i] = ACSIRCOMMWaitTime;
                break;
            case I2C_ACS_PULSES_SEND:
                I2CTWI_readRegisters[i] = ACSSendPulsesLeft; // Assume left == right
                break;
            case I2C_ACS_PULSES_REC:
                I2CTWI_readRegisters[i] = ACSRecPulsesLeft;
                break;
            case I2C_ACS_PULSES_REC_THRESHOLD:
                I2CTWI_readRegisters[i] = ACSRecPulsesLeftThreshold;
                break;
            case I2C_ACS_PULSES_TIMEOUT:
                I2CTWI_readRegisters[i] = ACSTimeOutLeft;
                break;
                
            case I2C_LASTRC5_ADR:
                I2CTWI_readRegisters[i] = lastRC5Data.device;
                if (lastRC5Data.toggle_bit)
                    I2CTWI_readRegisters[i] |= TOGGLEBIT;
                break;                
            case I2C_LASTRC5_KEY:
                I2CTWI_readRegisters[i] = lastRC5Data.key_code;
                break;
        }
    }
}

void handleCommands(void)
{
    if (I2CTWI_writeRegisters[I2C_CMD_REGISTER] && !I2CTWI_writeBusy)
    {
        uint8_t cmd = I2CTWI_writeRegisters[I2C_CMD_REGISTER];
        I2CTWI_writeRegisters[I2C_CMD_REGISTER] = 0;
        
        uint8_t args[I2C_CMD_MAX_ARGS], i;
        for (i=0; i<I2C_CMD_MAX_ARGS; ++i)
            args[i] = I2CTWI_writeRegisters[i+1];
        
        switch (cmd)
        {
            case I2C_CMD_STATEACK:                
            case I2C_CMD_ACK:
                if (!isStopwatch1Running())
                    startStopwatch1();
                setStopwatch1(0);
                
                // Reset stuff
                if (cmd == I2C_CMD_STATEACK)
                    extIntOFF();
                else
                    lastRC5Data.data = 0;
                break;
            case I2C_CMD_SETPOWER:
                if (args[0])
                    powerON();
                else
                    powerOFF();
                break;
            case I2C_CMD_SETACS:
                setACSState(args[0]);
                break;
            case I2C_CMD_ACS_UPDATE_INTERVAL:
                ACSUpdateInterval = args[0];
                break;
            case I2C_CMD_ACS_IRCOMM_WAIT:
                ACSIRCOMMWaitTime = args[0];
                break;
            case I2C_CMD_ACS_PULSES_SEND:
                ACSSendPulsesLeft = ACSSendPulsesRight = args[0];
                break;
            case I2C_CMD_ACS_PULSES_REC:
                ACSRecPulsesLeft = ACSRecPulsesRight = args[0];
                break;
            case I2C_CMD_ACS_PULSES_REC_THRESHOLD:
                ACSRecPulsesLeftThreshold = ACSRecPulsesRightThreshold = args[0];
                break;
            case I2C_CMD_ACS_PULSES_TIMEOUT:
                ACSTimeOutLeft = ACSTimeOutRight = args[0];
                break;
            case I2C_CMD_SETLEDS:
                statusLEDs.byte = args[0];
                updateStatusLEDs();
                break;
            case I2C_CMD_SENDRC5:
                IRCOMM_sendRC5(args[0], args[1]);
                break;
            case I2C_CMD_SETSPEED:
                moveAtSpeed(args[0], args[1]);
                break;
            case I2C_CMD_SETDIRECTION:
                changeDirection(args[0]);
                break;
            case I2C_CMD_STOP:
                stop();
                break;
            case I2C_CMD_MOVE:
                move(args[0], args[1], DIST_MM((uint16_t)args[2] + ((uint16_t)args[3] << 8)), false);
                break;
            case I2C_CMD_ROTATE:
                rotate(args[0], args[1], args[2] + (args[3] << 8), false);
                break;
            case I2C_CMD_ROTATE_FACTOR:
                rotationFactor = (uint16_t)args[0] + ((uint16_t)args[1] << 8);
                break;
        }
    }
}

void fullStop(void)
{
    // UNDONE
    stop();
}

void bumperHandler(void)
{
    if (bumper_left || bumper_right)
        stop(); // Safety: in case master responds too late
}

void RC5Handler(RC5data_t rc5data)
{
    lastRC5Data = rc5data;
}

int main(void)
{
    initRobotBase();
    I2CTWI_initSlave(I2C_SLAVEADDRESS);
    
    //powerON(); // UNDONE
    
    BUMPERS_setStateChangedHandler(bumperHandler);
    IRCOMM_setRC5DataReadyHandler(RC5Handler);
    
    setACSState(ACS_POWER_OFF);
   
    FlashLEDs();
    
    stateSensors.byte = 0;
    lastRC5Data.data = 0;
    
    startStopwatch1(); // Ping stopwatch

    for (;;)
    {
        if (getStopwatch1() > 1500)
        {
            fullStop();
            stopStopwatch1();
            setStopwatch1(0);
        }
        
        updateStateSensors();
        updateI2CReadRegisters();
        handleCommands();        
        task_RP6System();
    }
    
    return 0;
} 
