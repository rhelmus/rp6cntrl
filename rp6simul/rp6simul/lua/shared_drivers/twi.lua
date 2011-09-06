local ret = driver(...)

description = "Driver for the two-wire interface (TWI). Both slave and master mode."

handledIORegisters = {
    avr.IO_TWBR,
    avr.IO_TWSR,
}


function initPlugin()
    -- Initialize TWI IO registers
    -- UNDONE: Not here
    avr.setIORegister(avr.IO_TWSR, tonumber(11111000, 2))
    avr.setIORegister(avr.IO_TWDR, tonumber(11111111, 2))
    avr.setIORegister(avr.IO_TWAR, tonumber(11111110, 2))
end

function handleIOData(type, data)
    if type == avr.IO_TWBR then -- Bitrate
        -- From atmega32 docs: freq=CPU_clock/(16+2*TWBR*4^prescaler)
        -- Note that the driver currently does not handle any prescalers
        local freq = properties.clockSpeed / (16+2*data)
        log(string.format("Setting bit rate to %d (%d Hz)\n", data, freq))
        updateRobotStatus("TWI", "bitrate", data)
        updateRobotStatus("TWI", "frequency (Hz)", freq)
    elseif type == avr.IO_TWSR then -- Prescaler (on write access)
        if data ~= 0 then
            errorLog("TWI driver does not support any prescaler!")
        end
    end
end


return ret
