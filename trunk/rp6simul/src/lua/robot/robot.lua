local ret = simulator(...)

driverList =
{
    { name = "acs", default  = true },
    { name = "adc", default  = true },
    { name = "battery", default  = true },
    { name = "bumper", default  = true },
    { name = "extint1", default  = true },
    { name = "ircomm", default  = true },
    { name = "led", default  = true },
    { name = "light", default  = true },
    { name = "motor", default  = true },
    { name = "portlog", default  = false },
    { name = "timer0", default  = true },
    { name = "timer1", default  = false },
    { name = "timer2", default  = true },
    { name = "twi", default  = true },
    { name = "uart", default  = true },
}

-- Override for convenience
local oldUpdateRobotStatus = updateRobotStatus
function updateRobotStatus(...)
    oldUpdateRobotStatus("robot", ...)
end


-- Used by ADC driver
function getADCPortNames()
    return { "ADC0", "ADC1", "LS_R", "LS_L", "E_INT1", "MCURRENT_R",
             "MCURRENT_L", "UBAT" }
end


function init()
    clock.setTargetSpeed(properties.clockSpeed)
    setCmPerPixel(properties.cmPerPixel)
    setRobotLength(properties.robotLength)
    setRobotM32Scale(properties.m32Scale)
    setRobotM32Slot("front", properties.m32Front.pos, properties.m32Front.rotation)
    setRobotM32Slot("back", properties.m32Back.pos, properties.m32Back.rotation)
end

function closePlugin()
end


dofile(getLuaSrcPath("robot/properties.lua"))


return ret
