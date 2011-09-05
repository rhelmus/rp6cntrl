local ret = driver(...)

description = "M32 SPI driver. You will need this."

handledIORegisters = {
    avr.IO_SPSR,
}


function initPlugin()
    -- Init SPSR register: we're always ready
    -- UNDONE: Not here
    avr.setIORegister(avr.IO_SPSR, bit.set(0, avr.SPIF))
end

function handleIOData(type, data)
    -- We're always ready
    avr.setIORegister(avr.IO_SPSR, bit.set(data, avr.SPIF))
end


return ret
