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


function init()
    clock.setTargetSpeed(robotProperties.clockSpeed)
    setCmPerPixel(robotProperties.cmPerPixel)
    setRobotLength(robotProperties.robotLength)
end

function closePlugin()
end

dofile("lua/robot/properties.lua")


return ret
