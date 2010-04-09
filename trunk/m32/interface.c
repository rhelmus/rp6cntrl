#include "RP6ControlLib.h"
#include "RP6I2CmasterTWI.h"
#include "../shared/shared.h"
#include "interface.h"

enum { REQUEST_DUMP_BASE_DATA = 0 };

uint8_t slaveData[I2C_MAX_INDEX];
uint16_t lastPing;
RC5data_t lastRC5Data;
uint8_t slaveMode;
uint16_t slaveMicUpdateTime;

void sendSerialMSGByte(ESerialMessage msg, uint8_t data)
{
    writeChar(SERIAL_MSG_START);
    writeChar(2); // Size
    writeChar(msg);
    writeChar(data);
}

void sendSerialMSGWord(ESerialMessage msg, uint16_t data)
{
    writeChar(SERIAL_MSG_START);
    writeChar(3); // Size
    writeChar(msg);
    writeChar(data); // Low
    writeChar(data >> 8); // High
}

void I2CError(uint8_t error)
{
    writeString_P("\nI2C ERROR - TWI STATE: 0x");
    writeInteger(error, HEX);
    writeChar('\n');
    
    stopStopwatch2();
    setStopwatch2(0);
}

void I2CRequestReady(uint8_t id)
{
    if (id == REQUEST_DUMP_BASE_DATA)
    {
        I2CTWI_getReceivedData(&slaveData[I2C_LEDS], I2C_MAX_INDEX-I2C_LEDS);
        lastPing = getStopwatch2();
        stopStopwatch2();
        setStopwatch2(0);

        if (slaveData[I2C_LASTRC5_KEY]) // Got new RC5 data?
        {
            // Store it, as it won't be there anymore in the next update
            lastRC5Data.device = (slaveData[I2C_LASTRC5_ADR] & ~TOGGLEBIT);
            lastRC5Data.toggle_bit = ((slaveData[I2C_LASTRC5_ADR] & TOGGLEBIT) ==
                                      TOGGLEBIT);
            lastRC5Data.key_code = slaveData[I2C_LASTRC5_KEY];
        }

        // Send ACK. Neater would be to integrate this in the I2C lib code
        I2CTWI_transmit2Bytes(I2C_SLAVEADDRESS, I2C_CMD_REGISTER, I2C_CMD_ACK);

        if (slaveMode)
        {
            // Update stats through serial
            sendSerialMSGByte(SERIAL_BASE_LEDS, getBaseLEDs());
            sendSerialMSGByte(SERIAL_M32_LEDS, externalPort.LEDS);
            
            sendSerialMSGWord(SERIAL_LIGHT_LEFT, getLeftLightSensor());
            sendSerialMSGWord(SERIAL_LIGHT_RIGHT, getRightLightSensor());
            
            sendSerialMSGByte(SERIAL_MOTOR_SPEED_LEFT, getLeftMotorSpeed());
            sendSerialMSGByte(SERIAL_MOTOR_SPEED_RIGHT, getRightMotorSpeed());
            sendSerialMSGByte(SERIAL_MOTOR_DESTSPEED_LEFT, getLeftDestMotorSpeed());
            sendSerialMSGByte(SERIAL_MOTOR_DESTSPEED_RIGHT, getRightDestMotorSpeed());
            sendSerialMSGWord(SERIAL_MOTOR_DIST_LEFT, getLeftMotorDist());
            sendSerialMSGWord(SERIAL_MOTOR_DIST_RIGHT, getRightMotorDist());
            sendSerialMSGWord(SERIAL_MOTOR_DESTDIST_LEFT, getLeftMotorDestDist());
            sendSerialMSGWord(SERIAL_MOTOR_DESTDIST_RIGHT, getRightMotorDestDist());
            sendSerialMSGWord(SERIAL_MOTOR_CURRENT_LEFT, getLeftMotorCurrent());
            sendSerialMSGWord(SERIAL_MOTOR_CURRENT_RIGHT, getRightMotorCurrent());
            sendSerialMSGByte(SERIAL_MOTOR_DIRECTIONS, getMotorDirections().byte);

            sendSerialMSGWord(SERIAL_BATTERY, getBattery());

            sendSerialMSGByte(SERIAL_ACS_POWER, getACSPowerState());

            if (slaveMicUpdateTime && (getStopwatch3() >= slaveMicUpdateTime))
            {
                sendSerialMSGWord(SERIAL_MIC, getMicrophonePeak());
                setStopwatch3(0);
            }

            sendSerialMSGWord(SERIAL_LASTRC5, getLastRC5().data);
        }
    }
}

void initI2C(void)
{
    memset(slaveData, 0, I2C_MAX_INDEX);
    I2CTWI_initMaster(200);
    I2CTWI_setTransmissionErrorHandler(I2CError);
    I2CTWI_setRequestedDataReadyHandler(I2CRequestReady);
}

void setBasePower(uint8_t enable)
{
    I2CTWI_transmit3Bytes(I2C_SLAVEADDRESS, I2C_CMD_REGISTER,
                          I2C_CMD_SETPOWER, enable);                          
}

void setBaseACS(EACSPowerState state)
{
    I2CTWI_transmit3Bytes(I2C_SLAVEADDRESS, I2C_CMD_REGISTER,
                          I2C_CMD_SETACS, state);                          
}

void setACSUpdateInterval(uint8_t interval)
{
    I2CTWI_transmit3Bytes(I2C_SLAVEADDRESS, I2C_CMD_REGISTER,
                          I2C_CMD_ACS_UPDATE_INTERVAL, interval);
}

void setACSIRCOMMWait(uint8_t time)
{
    I2CTWI_transmit3Bytes(I2C_SLAVEADDRESS, I2C_CMD_REGISTER,
                          I2C_CMD_ACS_IRCOMM_WAIT, time);
}

void setACSPulsesSend(uint8_t pulses)
{
    I2CTWI_transmit3Bytes(I2C_SLAVEADDRESS, I2C_CMD_REGISTER,
                          I2C_CMD_ACS_PULSES_SEND, pulses);
}

void setACSPulsesRec(uint8_t pulses)
{
    I2CTWI_transmit3Bytes(I2C_SLAVEADDRESS, I2C_CMD_REGISTER,
                          I2C_CMD_ACS_PULSES_REC, pulses);
}

void setACSPulsesRecThreshold(uint8_t pulses)
{
    I2CTWI_transmit3Bytes(I2C_SLAVEADDRESS, I2C_CMD_REGISTER,
                          I2C_CMD_ACS_PULSES_REC_THRESHOLD, pulses);
}

void setACSPulsesTimeout(uint8_t time)
{
    I2CTWI_transmit3Bytes(I2C_SLAVEADDRESS, I2C_CMD_REGISTER,
                          I2C_CMD_ACS_PULSES_TIMEOUT, time);
}

void setBaseLEDs(uint8_t leds)
{
    I2CTWI_transmit3Bytes(I2C_SLAVEADDRESS, I2C_CMD_REGISTER,
                          I2C_CMD_SETLEDS, leds);
}

void sendRC5(uint8_t adr, uint8_t data)
{
    I2CTWI_transmit4Bytes(I2C_SLAVEADDRESS, I2C_CMD_REGISTER,
                          I2C_CMD_SENDRC5, adr, data);
}

void setMoveSpeed(uint8_t left, uint8_t right)
{
    I2CTWI_transmit4Bytes(I2C_SLAVEADDRESS, I2C_CMD_REGISTER,
                          I2C_CMD_SETSPEED, left, right);
}

void setMoveDirection(uint8_t dir)
{
    I2CTWI_transmit3Bytes(I2C_SLAVEADDRESS, I2C_CMD_REGISTER,
                          I2C_CMD_SETDIRECTION, dir);
}

void stopMovement(void)
{
    I2CTWI_transmit2Bytes(I2C_SLAVEADDRESS, I2C_CMD_REGISTER, I2C_CMD_STOP);
}

void move(uint8_t speed, uint8_t dir, uint16_t dist)
{
    uint8_t buf[6] = { I2C_CMD_REGISTER, I2C_CMD_MOVE, speed, dir, dist, (dist >> 8) };
    I2CTWI_transmitBytes(I2C_SLAVEADDRESS, buf, 6);
}

void rotate(uint8_t speed, uint8_t dir, uint16_t angle)
{
    uint8_t buf[6] = { I2C_CMD_REGISTER, I2C_CMD_ROTATE, speed, dir,
                       angle, (angle >> 8) };
    I2CTWI_transmitBytes(I2C_SLAVEADDRESS, buf, 6);
}

void setRotationFactor(uint16_t factor)
{
    I2CTWI_transmit4Bytes(I2C_SLAVEADDRESS, I2C_CMD_REGISTER,
                          I2C_CMD_ROTATE_FACTOR, factor, (factor >> 8));
}

void updateInterface(void)
{
    if (!isStopwatch1Running())
        startStopwatch1();

    if (!I2CTWI_isBusy())
    {
        if (PIND & EINT1)
        {
            I2CTWI_transmitByte(I2C_SLAVEADDRESS, I2C_STATE_SENSORS);
            slaveData[I2C_STATE_SENSORS] = I2CTWI_readByte(I2C_SLAVEADDRESS);
            I2CTWI_transmit2Bytes(I2C_SLAVEADDRESS, I2C_CMD_REGISTER, I2C_CMD_STATEACK);

            if (slaveMode)
                sendSerialMSGByte(SERIAL_STATE_SENSORS, slaveData[I2C_STATE_SENSORS]);
        }
        else if (getStopwatch1() > 100)
        {
            I2CTWI_requestRegisterFromDevice(I2C_SLAVEADDRESS, REQUEST_DUMP_BASE_DATA,
                                             I2C_LEDS, I2C_MAX_INDEX-I2C_LEDS);
            setStopwatch1(0);
            startStopwatch2();
        }
    }
}
