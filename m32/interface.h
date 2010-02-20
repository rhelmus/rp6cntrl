#ifndef INTERFACE_H
#define INTERFACE_H

// From RP6RobotBaseLib.h. Keep this in sync!
#define TOGGLEBIT 32

typedef union {
    uint16_t data;
    struct {
        unsigned key_code:6;
        unsigned device:5;
        unsigned toggle_bit:1;
        unsigned reserved:3;
    };
} RC5data_t;


void initI2C(void);
void pingBase(void);
void requestBaseData(void);
void setBasePower(uint8_t enable);
void setBaseACS(EACSPowerState state);
void setBaseLEDs(uint8_t leds);
void sendRC5(uint8_t adr, uint8_t data);
void setMoveSpeed(uint8_t left, uint8_t right);
void setMoveDirection(uint8_t dir);
void stopMovement(void);
void move(uint8_t speed, uint8_t dir, uint16_t dist);
void rotate(uint8_t speed, uint8_t dir, uint16_t angle);

extern uint8_t slaveData[I2C_MAX_INDEX];

inline EStateSensors getStateSensors(void)
{ return (EStateSensors)slaveData[I2C_STATE_SENSORS]; }
inline uint8_t getBaseLEDs(void) { return slaveData[I2C_LEDS]; }
inline uint16_t getLeftLightSensor(void)
{ return slaveData[I2C_LIGHT_LEFT_LOW] + (slaveData[I2C_LIGHT_LEFT_HIGH] << 8); }
inline uint16_t getRightLightSensor(void)
{ return slaveData[I2C_LIGHT_RIGHT_LOW] + (slaveData[I2C_LIGHT_RIGHT_HIGH] << 8); }
inline uint16_t getLeftMotorSpeed(void) { return slaveData[I2C_MOTOR_SPEED_LEFT]; }
inline uint16_t getRightMotorSpeed(void) { return slaveData[I2C_MOTOR_SPEED_RIGHT]; }
inline uint8_t getLeftDestMotorSpeed(void)
{ return slaveData[I2C_MOTOR_DESTSPEED_LEFT]; }
inline uint8_t getRightDestMotorSpeed(void)
{ return slaveData[I2C_MOTOR_DESTSPEED_RIGHT]; }
inline uint16_t getLeftMotorCurrent(void)
{ return slaveData[I2C_MOTOR_CURRENT_LEFT_LOW] +
         (slaveData[I2C_MOTOR_CURRENT_LEFT_HIGH] << 8); }
inline uint16_t getRightMotorCurrent(void)
{ return slaveData[I2C_MOTOR_CURRENT_RIGHT_LOW] +
         (slaveData[I2C_MOTOR_CURRENT_RIGHT_HIGH] << 8); }
inline uint16_t getBattery(void)
{ return slaveData[I2C_BATTERY_LOW] + (slaveData[I2C_BATTERY_HIGH] << 8); }
inline EACSPowerState getACSPowerState(void) { return slaveData[I2C_ACS_POWER]; }

extern uint16_t lastPing;
inline uint16_t getLastPing(void) { return lastPing; }

RC5data_t getLastRC5(void);
    
#endif

