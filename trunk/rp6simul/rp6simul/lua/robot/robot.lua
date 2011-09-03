local ret = simulator(...)

driverList =
{
    { name = "acs", default  = true },
    { name = "adc", default  = true },
    { name = "bumper", default  = true },
    { name = "ircomm", default  = true },
    { name = "led", default  = true },
    { name = "light", default  = true },
    { name = "motor", default  = true },
    { name = "portlog", default  = false },
    { name = "timer0", default  = true },
    { name = "timer1", default  = false },
    { name = "timer2", default  = true },
    { name = "uart", default  = true },
}

-- Override for convenience
local oldUpdateRobotStatus = updateRobotStatus
function updateRobotStatus(...)
    oldUpdateRobotStatus("robot", ...)
end

function init()
    clock.setTargetSpeed(properties.clockSpeed)
    setCmPerPixel(properties.cmPerPixel)
    setRobotLength(properties.robotLength)
end

function closePlugin()
end

dofile("lua/robot/properties.lua")


return ret
