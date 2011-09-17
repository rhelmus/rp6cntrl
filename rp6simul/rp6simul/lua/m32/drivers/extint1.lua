local ret = driver(...)

description = "Driver for external interrupt 1. You will also need the robot driver counterpart."

handledIORegisters = {
    avr.IO_DDRD,
}

local driverEnabled = false

local function handleExtInt1(e)
    if not driverEnabled then
        warning("Driver disabled\n")
        return
    end

    -- UNDONE: Call interrupt?

    local d = avr.getIORegister(avr.IO_PIND)
    log("extint1:", d, "\n")
    if e then
        avr.setIORegister(avr.IO_PIND, bit.set(d, avr.PIND2))
    else
        avr.setIORegister(avr.IO_PIND, bit.unSet(d, avr.PIND2))
    end
end


function initPlugin()
    setExtInt1Handler(handleExtInt1)
end

function handleIOData(type, data)
    -- Verify data direction
    driverEnabled = (not bit.isSet(data, avr.DDD2))
end


return ret
