#include <stdint.h>

#include "RP6ControlLib.h"
#include "RP6I2CmasterTWI.h"
#include "../shared/shared.h"
#include "interface.h"
#include "servo.h"

enum { REQUEST_DUMP_BASE_DATA = 0 };

uint8_t slaveData[I2C_MAX_INDEX];
uint16_t lastPing;
RC5data_t lastRC5Data;
uint8_t slaveMode;
uint16_t slaveMicUpdateTime;
SUpdateSlaveData updateSlaveData;

// Replaced macros from servolib to these
uint8_t servoLeftTouch = 46;
uint8_t servoRightTouch = 188;
uint32_t servoTimer1 = 100000;

void sendSerialByte(uint8_t data)
{
    // Adepted from RP6 uart lib
    while (!(UCSRA & (1<<UDRE)))
    {
        // Keep running mandatory tasks
        task_I2CTWI();
        task_SERVO();
    }

    UDR = data;
}

void sendSerialMSGByte(ESerialMessage msg, uint8_t data)
{
    sendSerialByte(SERIAL_MSG_START);
    sendSerialByte(2); // Size
    sendSerialByte(msg);
    sendSerialByte(data);
}

void sendSerialMSGWord(ESerialMessage msg, uint16_t data)
{
    sendSerialByte(SERIAL_MSG_START);
    sendSerialByte(3); // Size
    sendSerialByte(msg);
    sendSerialByte(data); // Low
    sendSerialByte(data >> 8); // High
}

void sendSlaveData(void)
{
    // Update stats through serial

    static uint16_t lastUpdate = 0;

    const uint16_t curtime = getStopwatch4();
    const uint16_t delta = curtime - lastUpdate;

    // TODO: Replace slow modulo operations? (http://embeddedgurus.com/stack-overflow/2011/02/efficient-c-tip-13-use-the-modulus-operator-with-caution/)
    // Evil macro and stuff
#define CheckDelay(d) (d && (delta > (curtime % d)))

    if (CheckDelay(updateSlaveData.LEDDelay))
    {
        sendSerialMSGByte(SERIAL_BASE_LEDS, getBaseLEDs());
        sendSerialMSGByte(SERIAL_M32_LEDS, externalPort.LEDS);
    }

    if (CheckDelay(updateSlaveData.lightDelay))
    {
        sendSerialMSGWord(SERIAL_LIGHT_LEFT, getLeftLightSensor());
        sendSerialMSGWord(SERIAL_LIGHT_RIGHT, getRightLightSensor());
    }

    if (CheckDelay(updateSlaveData.motorDelay))
    {
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
    }

    if (CheckDelay(updateSlaveData.batteryDelay))
        sendSerialMSGWord(SERIAL_BATTERY, getBattery());

    if (CheckDelay(updateSlaveData.ACSDelay))
        sendSerialMSGByte(SERIAL_ACS_POWER, getACSPowerState());

    if (CheckDelay(updateSlaveData.micDelay))
        sendSerialMSGWord(SERIAL_MIC, getMicrophonePeak());

    if (CheckDelay(updateSlaveData.RC5Delay))
        sendSerialMSGWord(SERIAL_LASTRC5, getLastRC5().data);

    if (CheckDelay(updateSlaveData.sharpIRDelay))
        sendSerialMSGByte(SERIAL_SHARPIR, getSharpIRDistance());

    if (getStopwatch4() >= SLAVE_ROLLBACK_TIME)
    {
        setStopwatch4(SLAVE_ROLLBACK_TIME - lastUpdate);
        lastUpdate = 0;
    }
    else
        lastUpdate = curtime;
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
            sendSlaveData();
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
    beep(120, 80);
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

void setServoRange(uint8_t left, uint8_t right)
{
    servoLeftTouch = left;
    servoRightTouch = right;
}

void setServoTimer1(uint32_t timer)
{
    servoTimer1 = timer;
    // From initSERVO()
    OCR1A = ((F_CPU/8/timer)-1);   // 19 at 100kHz
}

uint8_t getSharpIRDistance(void)
{
    uint16_t adc = readADC(ADC_2);

    // UNDONE: Avoid float calculations?

    // ADC to volt (assuming 5v supply))
    float volt = (float)adc * 5.0 / 1023.0;

    // From http://www.robotshop.ca/PDF/Sharp_GP2Y0A02YK_Ranger.pdf
    const float A = 0.008271, B = 939.6, C = -3.398, D = 17.339;

    return (uint8_t)((A + B * volt) / (1.0 + C * volt + D * volt * volt));
}

void updateInterface(void)
{
    if (!isStopwatch3Running())
        startStopwatch3();

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
        else if (getStopwatch3() > 50)
        {
            I2CTWI_requestRegisterFromDevice(I2C_SLAVEADDRESS, REQUEST_DUMP_BASE_DATA,
                                             I2C_LEDS, I2C_MAX_INDEX-I2C_LEDS);
            setStopwatch3(0);
            startStopwatch2();
        }
    }
}
