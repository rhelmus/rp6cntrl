local ret = simulator(...)

-- UNDONE: defaults
driverList =
{
    { name = "adc", default = true },
--    { name = "led", default  = true },
--    { name = "portlog", default  = false },
    { name = "timer0", default  = true },
    { name = "timer1", default  = false },
    { name = "timer2", default  = false },
    { name = "uart", default  = true },
}

-- Override for convenience
local oldUpdateRobotStatus = updateRobotStatus
function updateRobotStatus(...)
    oldUpdateRobotStatus("m32", ...)
end


-- Used by ADC driver
function getADCPortNames()
    return { "MIC", "KEYPAD", "ADC2", "ADC3", "ADC4", "ADC5", "ADC6", "ADC7" }
end


function init()
    clock.setTargetSpeed(properties.clockSpeed)
end

function initPlugin()
end

function closePlugin()
end

dofile("lua/m32/properties.lua")


return ret