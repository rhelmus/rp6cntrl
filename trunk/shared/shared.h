
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
    
    I2C_BATTERY_LOW,
    I2C_BATTERY_HIGH,
    
    I2C_ACS_POWER,
    
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
    };
} EStateSensors;

typedef enum
{
    ACS_POWER_OFF=0,
    ACS_POWER_LOW,
    ACS_POWER_MED,
    ACS_POWER_HIGH
} EACSPowerState;

enum
{
    I2C_CMD_NONE=0,
    I2C_CMD_PING,
    I2C_CMD_SETPOWER,
    I2C_CMD_SETACS,
    I2C_CMD_SETLEDS,
    I2C_CMD_SENDRC5,
    I2C_CMD_SETSPEED,
    I2C_CMD_SETDIRECTION,
    I2C_CMD_STOP,
    I2C_CMD_MOVE,
    I2C_CMD_ROTATE,    
    I2C_CMD_MAX_INDEX
};

// Directions for m32 code
#ifndef FWD
#define FWD 0
#endif

#ifndef BWD
#define BWD 1
#endif

#ifndef LEFT
#define LEFT 2
#endif

#ifndef RIGHT
#define RIGHT 3
#endif

#endif

