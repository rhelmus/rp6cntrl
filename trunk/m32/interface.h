#ifndef INTERFACE_H
#define INTERFACE_H

#include "shared.h"

// From RP6RobotBaseLib.h. Keep this in sync!

#define TOGGLEBIT 32

// --- End RP6 base code


void initI2C(void);
void setBasePower(uint8_t enable);
void setBaseACS(EACSPowerState state);
void setACSUpdateInterval(uint8_t interval);
void setACSIRCOMMWait(uint8_t time);
void setACSPulsesSend(uint8_t pulses);
void setACSPulsesRec(uint8_t pulses);
void setACSPulsesRecThreshold(uint8_t pulses);
void setACSPulsesTimeout(uint8_t time);
void setBaseLEDs(uint8_t leds);
void sendRC5(uint8_t adr, uint8_t data);
void setMoveSpeed(uint8_t left, uint8_t right);
void setMoveDirection(uint8_t dir);
void stopMovement(void);
void move(uint8_t speed, uint8_t dir, uint16_t dist);
void rotate(uint8_t speed, uint8_t dir, uint16_t angle);
void setRotationFactor(uint16_t factor);

extern uint8_t slaveData[I2C_MAX_INDEX];

inline SStateSensors getStateSensors(void)
{ return (SStateSensors)slaveData[I2C_STATE_SENSORS]; }
inline uint8_t getBaseLEDs(void) { return slaveData[I2C_LEDS]; }

inline uint16_t getLeftLightSensor(void)
{ return slaveData[I2C_LIGHT_LEFT_LOW] + (slaveData[I2C_LIGHT_LEFT_HIGH] << 8); }
inline uint16_t getRightLightSensor(void)
{ return slaveData[I2C_LIGHT_RIGHT_LOW] + (slaveData[I2C_LIGHT_RIGHT_HIGH] << 8); }

inline uint8_t getLeftMotorSpeed(void) { return slaveData[I2C_MOTOR_SPEED_LEFT]; }
inline uint8_t getRightMotorSpeed(void) { return slaveData[I2C_MOTOR_SPEED_RIGHT]; }

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

inline uint16_t getLeftMotorDist(void)
{ return slaveData[I2C_MOTOR_LEFT_DIST_LOW] +
         (slaveData[I2C_MOTOR_LEFT_DIST_HIGH] << 8); }
inline uint16_t getRightMotorDist(void)
{ return slaveData[I2C_MOTOR_RIGHT_DIST_LOW] +
         (slaveData[I2C_MOTOR_RIGHT_DIST_HIGH] << 8); }

inline uint16_t getLeftMotorDestDist(void)
{ return slaveData[I2C_MOTOR_LEFT_DESTDIST_LOW] +
         (slaveData[I2C_MOTOR_LEFT_DESTDIST_HIGH] << 8); }
inline uint16_t getRightMotorDestDist(void)
{ return slaveData[I2C_MOTOR_RIGHT_DESTDIST_LOW] +
         (slaveData[I2C_MOTOR_RIGHT_DESTDIST_HIGH] << 8); }

inline SMotorDirections getMotorDirections(void)
{ return (SMotorDirections)slaveData[I2C_MOTOR_DIRECTIONS]; }

inline uint16_t getRotationFactor(void)
{ return slaveData[I2C_ROTATE_FACTOR_LOW] + (slaveData[I2C_ROTATE_FACTOR_HIGH] << 8); }
         
inline uint16_t getBattery(void)
{ return slaveData[I2C_BATTERY_LOW] + (slaveData[I2C_BATTERY_HIGH] << 8); }

inline EACSPowerState getACSPowerState(void) { return slaveData[I2C_ACS_POWER]; }
inline uint8_t getACSUpdateInterval(void) { return slaveData[I2C_ACS_UPDATE_INTERVAL]; }
inline uint8_t getACSIRCOMMWait(void) { return slaveData[I2C_ACS_IRCOMM_WAIT]; }
inline uint8_t getACSPulsesSend(void) { return slaveData[I2C_ACS_PULSES_SEND]; }
inline uint8_t getACSPulsesRec(void) { return slaveData[I2C_ACS_PULSES_REC]; }
inline uint8_t getACSPulsesRecThreshold(void)
{ return slaveData[I2C_ACS_PULSES_REC_THRESHOLD]; }
inline uint8_t getACSPulsesTimeout(void) { return slaveData[I2C_ACS_PULSES_TIMEOUT]; }

extern uint16_t lastPing;
inline uint16_t getLastPing(void) { return lastPing; }

extern RC5data_t lastRC5Data;
inline RC5data_t getLastRC5(void) { return lastRC5Data; }
inline void resetLastRC5(void) { lastRC5Data.data = 0; }

void setServoRange(uint8_t left, uint8_t right);
void setServoTimer1(uint32_t timer);
uint8_t getSharpIRDistance(void); 

void updateInterface(void);

#endif

