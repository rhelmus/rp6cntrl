local ret = driver(...)

description = "Driver for the two-wire interface (TWI). Both slave and master mode."

handledIORegisters = {
    avr.IO_TWAR,
    avr.IO_TWBR,
    avr.IO_TWDR,
    avr.IO_TWSR,
    avr.IO_TWCR,
}


local TWIInfo =
{
    state = "idle",
    slaveAddress = nil, -- nil: not a slave (uninitialized)
    slaveData = nil,
}


local function isMaster()
    return TWIInfo.slaveAddress == nil
end

local function TWIMSGHandler(msg, ...)
    -- UNDONE: general calls

    log("Received TWI MSG:", isMaster(), msg, ..., "\n")
--[[
    local maybeexecisr, accept, ret = false, false, nil
    local TWCR = avr.getIORegister(avr.IO_TWCR)

    if isMaster() then
        if msg == "sendslavedata" then
            assert(TWIInfo.state == "mread")
            ret = bit.isSet(TWCR, avr.TWEA)
            local d = selectOne(1, ...)
            avr.setIORegister(avr.IO_TWDR, d)
            avr.setIORegister(avr.IO_TWSR, (ret and 0x50) or 0x58)
            log("Received master data:", d, "\n")
            maybeexecisr = true
            accept = true
        end
    else
        if msg == "sendslavereadadr" or msg == "sendslavewriteadr" then
            local adr = selectOne(1, ...)
            if adr == TWIInfo.slaveAddress then
                TWIInfo.state = ((msg == "sendslavereadadr") and "sread") or "swrite"
                ret = bit.isSet(TWCR, avr.TWEA)
                if TWIInfo.state == "sread" and ret then
                    avr.setIORegister(avr.IO_TWSR, 0xA8)
                    maybeexecisr = true
                else
                    avr.setIORegister(avr.IO_TWSR, (ret and 0x60) or 0x68)
                    maybeexecisr = true
                end
                accept = true
            end
        elseif msg == "sendslavedata" and TWIInfo.state == "swrite" then
            ret = bit.isSet(TWCR, avr.TWEA)
            local d = selectOne(1, ...)
            avr.setIORegister(avr.IO_TWDR, d)
            avr.setIORegister(avr.IO_TWSR, (ret and 0x80) or 0x88)
            log("Received slave data:", d, ret, "\n")
            maybeexecisr = true
            accept = true
        elseif (msg == "stopslave" or msg == "restartslave") and
               TWIInfo.state ~= "idle" then
            TWIInfo.state = "idle"
            avr.setIORegister(avr.IO_TWSR, 0xA0)
            maybeexecisr = true
        end
    end

    if maybeexecisr and bit.isSet(TWCR, avr.TWIE) then
        avr.execISR(avr.ISR_TWI_vect)
    end

    return accept, ret
    --]]
end

local function handleMasterEvent(data)
    -- UNDONE

    local maybeexecisr = false

    -- Master start condition?
    if bit.isSet(data, avr.TWINT, avr.TWSTA) and
       not bit.isSet(data, avr.TWSTO) then
        if TWIInfo.state ~= "idle" then
            log("Master restart condition\n")
            avr.setIORegister(avr.IO_TWSR, 0x10)
            maybeexecisr = true
            avr.sendTWIMSG("restartslave")
        else
            log("Master start condition\n")
            avr.setIORegister(avr.IO_TWSR, 0x08)
            maybeexecisr = true
        end
        TWIInfo.state = "mstart"
    -- SLA+W / SLA+R?
    elseif bit.isSet(data, avr.TWINT) and
           not bit.isSet(data, avr.TWSTA) and not bit.isSet(data, avr.TWSTO) then
        if TWIInfo.state ~= "idle" then
            local d = avr.getIORegister(avr.IO_TWDR)
            if TWIInfo.state == "mstart" then -- Address?
                if bit.isSet(d, 0) then -- MR read
                    TWIInfo.state = "mread"
                    d = bit.unSet(d, 0)
                    log(string.format("SLA+R: %d\n", d))
                    local ack = avr.sendTWIMSG("sendslavereadadr", d)
                    avr.setIORegister(avr.IO_TWSR, (ack and 0x40) or 0x48)
                    maybeexecisr = true
                else -- MR write
                    TWIInfo.state = "mwrite"
                    log(string.format("SLA+W: %d\n", d))
                    local ack = avr.sendTWIMSG("sendslavewriteadr", d)
                    avr.setIORegister(avr.IO_TWSR, (ack and 0x18) or 0x20)
                    maybeexecisr = true
                end
            elseif TWIInfo.state == "mwrite" then  -- plain data
                local ack = avr.sendTWIMSG("sendslavedata",
                                           avr.getIORegister(avr.IO_TWDR))
                avr.setIORegister(avr.IO_TWSR, (ack and 0x28) or 0x30)
                maybeexecisr = true
            elseif TWIInfo.state == "mread" then
--                if not bit.isSet(data, avr.TWEA) then -- Don't want any more data?
--                    TWIInfo.state = "idle"
--                end
            end
        else
            warning("Ignoring data: SLA+R/W received without master start condition!\n")
            warning("Data:", data, "\n")
        end
    -- Master stop condition?
    elseif bit.isSet(data, avr.TWINT, avr.TWSTO) and
           not bit.isSet(data, avr.TWSTA) then
        log("Master stop condition\n")
        avr.sendTWIMSG("stopslave")
        TWIInfo.state = "idle"
    else
        warning("Ignoring unknown master event\n")
        return
    end

    data = bit.unSet(data, avr.TWINT)
    avr.setIORegister(avr.IO_TWCR, data)

    if maybeexecisr and bit.isSet(data, avr.TWIE) then
        avr.execISR(avr.ISR_TWI_vect)
    end
end

local function handleSlaveEvent(data)
    local maybeexecisr = false

    -- Specs say that TWSTO must be zero when writing
    if not bit.isSet(data, avr.TWSTO) then
        if TWIInfo.state == "sread" and TWIInfo.slaveData ~= nil then -- data set?
            local ack = avr.sendTWIMSG("sendslavedata", TWIInfo.slaveData)
            TWIInfo.slaveData = nil
            if ack then
                if bit.isSet(data, avr.TWEA) then
                    avr.setIORegister(avr.IO_TWSR, 0xB8)
                else
                    avr.setIORegister(avr.IO_TWSR, 0xC8)
                    TWIInfo.state = "idle"
                end
            else
                avr.setIORegister(avr.IO_TWSR, 0xC0)
                TWIInfo.state = "idle"
            end
            maybeexecisr = true
        end
    end

    data = bit.unSet(data, avr.TWINT)
    avr.setIORegister(avr.IO_TWCR, data)

    if maybeexecisr and bit.isSet(data, avr.TWIE) then
        avr.execISR(avr.ISR_TWI_vect)
    end
end


function initPlugin()
    -- Initialize TWI IO registers
    -- UNDONE: Not here
    avr.setIORegister(avr.IO_TWSR, tonumber(11111000, 2))
    avr.setIORegister(avr.IO_TWDR, tonumber(11111111, 2))
    avr.setIORegister(avr.IO_TWAR, tonumber(11111110, 2))

    avr.setTWIMSGHandler(TWIMSGHandler)
end

function handleIOData(type, data)
    if type == avr.IO_TWAR then -- Slave address
        TWIInfo.slaveAddress = data
        log(string.format("Setting slave address to %d\n", data))
        updateRobotStatus("TWI", "slave adr", data)
    elseif type == avr.IO_TWBR and isMaster() then -- Bitrate
        -- From atmega32 docs: freq=CPU_clock/(16+2*TWBR*4^prescaler)
        -- Note that the driver currently does not handle any prescalers
        local freq = properties.clockSpeed / (16+2*data)
        log(string.format("Setting bit rate to %d (%d Hz)\n", data, freq))
        updateRobotStatus("TWI", "bitrate", data)
        updateRobotStatus("TWI", "frequency (Hz)", freq)
    elseif type == avr.IO_TWDR and not isMaster() then
        if bit.isSet(avr.getIORegister(avr.IO_TWCR), avr.TWEN) then
            TWIInfo.slaveData = data
        end
    elseif type == avr.IO_TWSR and isMaster() then -- Prescaler (on write access)
        if data ~= 0 then
            errorLog("TWI driver does not support any prescaler!")
        end
    elseif type == avr.IO_TWCR then
        if bit.isSet(data, avr.TWEN) then
            if isMaster() then
                handleMasterEvent(data)
            else
                handleSlaveEvent(data)
            end
        end
    end
end


return ret
