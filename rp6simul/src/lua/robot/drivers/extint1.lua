local ret = driver(...)

description = "Driver for external interrupt 1. Signals m32 if present."

handledIORegisters = {
    avr.IO_PORTA,
    avr.IO_DDRA,
}


local curEnabled = false


function handleIOData(type, data)
    local e = bit.isSet(data, avr.PINA4)
    if e ~= curEnabled then
        curEnabled = e
        setExtInt1Enabled(e)
    end
end

return ret
