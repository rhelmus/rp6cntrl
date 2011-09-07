local ret = driver(...)

description = "Driver for the two-wire interface (TWI). Both slave and master mode."

handledIORegisters = {
    avr.IO_TWBR,
    avr.IO_TWSR,
    avr.IO_TWCR,
}


local TWIInfo =
{
    state = "idle",
}


local function handleMasterEvent(data)
    if not bit.isSet(data, avr.TWEN) then
        return
    end

    -- UNDONE

    -- Master start condition?
    if bit.isSet(data, avr.TWINT, avr.TWSTA) and
       not bit.isSet(data, avr.TWSTO)
        if TWIInfo.state ~= "idle" then
            log("Master restart condition\n")
        else
            log("Master start condition\n")
        end
        if doint then

        TWIInfo.state = "mstart"
    -- SLA+W / SLA+R?
    elseif bit.isSet(data, avr.TWINT) and
           not bit.isSet(data, avr.TWSTA, avr.TWSTO)
        if TWIInfo.state == "idle" then
            local d = avr.getIORegister(avr.IO_TWDR)
            if bit.isSet(d, 1) then -- MR read
                TWIInfo.state = "mread"
                d = bit.unSet(d, 1)
                log(string.format("SLA+R: %d\n", d))
            else -- MR write
                TWIInfo.state = "mwrite"
                log(string.format("SLA+W: %d\n", d))
            end
        else
            warning("Ignoring data: SLA+R/W received without master start condition!\n")
            return
        end
    -- Master stop condition?
    elseif bit.isSet(data, avr.TWINT, avr.TWSTO) and
           not bit.isSet(data, avr.TWSTA)
        log("Master stop condition\n")
    else
        warning("Ignoring unknown master event\n")
        return
    end

    if bit.isSet(data, avr.TWIE) then
        avr.execISR(avr.ISR_TWI_vect)
    end
end


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
    elseif type == avr.IO_TWCR then
        if bit.isSet(data, avr.TWINT) then
            handleMasterEvent(data)
        end
    end
end


return ret
