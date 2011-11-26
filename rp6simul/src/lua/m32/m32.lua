local ret = simulator(...)

driverList =
{
    { name = "adc", default = true },
    { name = "exteeprom", default  = true },
    { name = "extint1", default  = true },
    { name = "keypad", default  = true },
    { name = "led", default  = true },
    { name = "mic", default  = true },
    { name = "piezo", default  = true },
    { name = "portlog", default  = false },
    { name = "spi", default  = true },
    { name = "timer0", default  = true },
    { name = "timer1", default  = false },
    { name = "timer2", default  = false },
    { name = "twi", default  = true },
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

dofile(getLuaSrcPath("m32/properties.lua"))


return ret
