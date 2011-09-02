local ret = simulator(...)

driverList =
{
--    { name = "adc", default  = true },
--    { name = "led", default  = true },
--    { name = "portlog", default  = false },
--    { name = "timer0", default  = true },
--    { name = "timer1", default  = false },
--    { name = "timer2", default  = true },
--    { name = "uart", default  = true },
}


function init()
    clock.setTargetSpeed(m32Properties.clockSpeed)
end

local timer
function initPlugin()
    -- UNDONE!
    timer = clock.createTimer()
    timer:setTimeOut(function () end)
    timer:setCompareValue(100000)
    clock.enableTimer(timer, true)
end

function closePlugin()
    timer = nil
end

dofile("lua/m32/properties.lua")


return ret
