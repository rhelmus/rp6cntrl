#include "RP6ControlLib.h"
#include "RP6I2CmasterTWI.h"
#include "../shared/shared.h"

enum { REQUEST_DUMP_BASE_DATA = 0 };

uint8_t slaveData[I2C_MAX_INDEX];
uint16_t lastPing;

void I2CError(uint8_t error)
{
    writeString_P("\nI2C ERROR - TWI STATE: 0x");
    writeInteger(error, HEX);
    writeChar('\n');
}

void I2CRequestReady(uint8_t id)
{
    if (id == REQUEST_DUMP_BASE_DATA)
    {
        I2CTWI_getReceivedData(slaveData, I2C_MAX_INDEX);
        lastPing = getStopwatch2();
        stopStopwatch2();
        setStopwatch2(0);
    }
}

void initI2C(void)
{
    memset(slaveData, 0, I2C_MAX_INDEX);
    I2CTWI_initMaster(200);
    I2CTWI_setTransmissionErrorHandler(I2CError);
    I2CTWI_setRequestedDataReadyHandler(I2CRequestReady);
}

void pingBase(void)
{
    startStopwatch2();
    I2CTWI_transmit2Bytes(I2C_SLAVEADDRESS, I2C_CMD_REGISTER, I2C_CMD_PING);
}

void requestBaseData(void)
{
    I2CTWI_requestRegisterFromDevice(I2C_SLAVEADDRESS, REQUEST_DUMP_BASE_DATA, 0,
                                     I2C_MAX_INDEX);                                     
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
