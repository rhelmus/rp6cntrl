
#ifndef SHARED_H
#define SHARED_H

#define I2C_SLAVEADDRESS 10
#define I2C_CMD_REGISTER 0
#define I2C_CMD_MAX_ARGS 5

enum
{
    I2C_STATE_SENSORS=0, // Bumpers, ACD
    
    I2C_LEDS,
    
    I2C_LIGHT_LEFT_LOW,
    I2C_LIGHT_LEFT_HIGH,
    I2C_LIGHT_RIGHT_LOW,
    I2C_LIGHT_RIGHT_HIGH,
    
    I2C_MOTOR_SPEED_LEFT,
    I2C_MOTOR_SPEED_RIGHT,
    I2C_MOTOR_DESTSPEED_LEFT,
    I2C_MOTOR_DESTSPEED_RIGHT,
    
    I2C_MOTOR_CURRENT_LEFT_LOW,
    I2C_MOTOR_CURRENT_LEFT_HIGH,
    I2C_MOTOR_CURRENT_RIGHT_LOW,
    I2C_MOTOR_CURRENT_RIGHT_HIGH,

    I2C_MOTOR_LEFT_DIST_LOW,
    I2C_MOTOR_LEFT_DIST_HIGH,
    I2C_MOTOR_RIGHT_DIST_LOW,
    I2C_MOTOR_RIGHT_DIST_HIGH,

    I2C_MOTOR_LEFT_DESTDIST_LOW,
    I2C_MOTOR_LEFT_DESTDIST_HIGH,
    I2C_MOTOR_RIGHT_DESTDIST_LOW,
    I2C_MOTOR_RIGHT_DESTDIST_HIGH,

    I2C_ROTATE_FACTOR_LOW,
    I2C_ROTATE_FACTOR_HIGH,

    I2C_BATTERY_LOW,
    I2C_BATTERY_HIGH,

    I2C_ACS_POWER,
    
    // ACS/IRCOMM tweaks
    I2C_ACS_UPDATE_INTERVAL,
    I2C_ACS_IRCOMM_WAIT,
    I2C_ACS_PULSES_SEND,
    I2C_ACS_PULSES_REC,
    I2C_ACS_PULSES_REC_THRESHOLD,
    I2C_ACS_PULSES_TIMEOUT,

    I2C_LASTRC5_ADR,
    I2C_LASTRC5_KEY,
        
    I2C_MAX_INDEX
};

typedef union
{
    uint8_t byte;
    struct
    {
        uint8_t bumperLeft:1;
        uint8_t bumperRight:1;
        uint8_t ACSLeft:1;
        uint8_t ACSRight:1;
        uint8_t movementComplete:1;
        uint8_t ACSState:3;
    };
} SStateSensors;

typedef enum
{
    ACS_POWER_OFF=0,
    ACS_POWER_LOW,
    ACS_POWER_MED,
    ACS_POWER_HIGH,
} EACSPowerState;

enum
{
    I2C_CMD_NONE=0,
    I2C_CMD_STATEACK,
    I2C_CMD_ACK,
    I2C_CMD_SETPOWER,
    I2C_CMD_SETACS,
    I2C_CMD_ACS_UPDATE_INTERVAL,
    I2C_CMD_ACS_IRCOMM_WAIT,
    I2C_CMD_ACS_PULSES_SEND,
    I2C_CMD_ACS_PULSES_REC,
    I2C_CMD_ACS_PULSES_REC_THRESHOLD,
    I2C_CMD_ACS_PULSES_TIMEOUT,
    I2C_CMD_SETLEDS,
    I2C_CMD_SENDRC5,
    I2C_CMD_SETSPEED,
    I2C_CMD_SETDIRECTION,
    I2C_CMD_STOP,
    I2C_CMD_MOVE,
    I2C_CMD_ROTATE,
    I2C_CMD_ROTATE_FACTOR,
    I2C_CMD_MAX_INDEX
};


#endif
