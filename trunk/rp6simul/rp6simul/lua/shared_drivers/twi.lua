local ret = driver(...)

-- NOTE: Driver currently only supports one slave and one master

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
    isMaster = true,

    -- Master data
    slaveAddress = nil, -- nil: not a slave (uninitialized)
    slaveAcceptsGenCall = nil,

    -- Slave data
    slaveData = nil,
}


local function TWIMSGHandler(msg, ...)
--    log("Received TWI MSG:", TWIInfo.isMaster, msg, ..., "\n")

    local maybeexecisr = false
    local TWCR = avr.getIORegister(avr.IO_TWCR)
    local ack = bit.isSet(TWCR, avr.TWEA)

    if TWIInfo.isMaster then
        if msg == "sadr" then
            TWIInfo.slaveAddress = selectOne(1, ...)
            TWIInfo.slaveAcceptsGenCall = selectOne(2, ...)
        elseif msg == "swriteadrack" and TWIInfo.state == "mwrite" then
            local slack = selectOne(1, ...)
            avr.setIORegister(avr.IO_TWSR, (slack and 0x18) or 0x20)
            maybeexecisr = true
        elseif msg == "swriteack" and TWIInfo.state == "mwrite" then
            local slack = selectOne(1, ...)
--            log("swriteack:", slack, "\n")
            avr.setIORegister(avr.IO_TWSR, (slack and 0x28) or 0x30)
            maybeexecisr = true
        elseif msg == "sreadadrack" and TWIInfo.state == "mread" then
            local slack = selectOne(1, ...)
            avr.setIORegister(avr.IO_TWSR, (slack and 0x40) or 0x48)
            maybeexecisr = true
        elseif msg == "sdata" and TWIInfo.state == "mread" then
            assert(TWIInfo.state == "mread")
            avr.sendTWIMSG("mreadack", ack)
            local d = selectOne(1, ...)
            avr.setIORegister(avr.IO_TWDR, d)
            avr.setIORegister(avr.IO_TWSR, (ack and 0x50) or 0x58)
            log("Received master data:", d, "\n")
            maybeexecisr = true
        end
    else
        if msg == "slavereadadr" then
            avr.sendTWIMSG("sreadadrack", ack)
            if ack then
                TWIInfo.state = "swrite"
                avr.setIORegister(avr.IO_TWSR, 0xA8)
                maybeexecisr = true
            else
                TWIInfo.state = "idle"
            end
        elseif msg == "slavewriteadr" then
            avr.sendTWIMSG("swriteadrack", ack)
            if ack then
                TWIInfo.state = "sread"
                avr.setIORegister(avr.IO_TWSR, 0x60)
                maybeexecisr = true
            else
                TWIInfo.state = "idle"
            end
        elseif msg == "slavewriteadrgen" then
            avr.sendTWIMSG("swriteadrack", ack)
            if ack then
                TWIInfo.state = "sreadgen"
                avr.setIORegister(avr.IO_TWSR, 0x70)
                maybeexecisr = true
            else
                TWIInfo.state = "idle"
            end
        elseif msg == "mdata" and
               (TWIInfo.state == "sread" or TWIInfo.state == "sreadgen") then
            avr.sendTWIMSG("swriteack", ack)
            local d = selectOne(1, ...)
            avr.setIORegister(avr.IO_TWDR, d)
            if TWIInfo.state == "sread" then
                avr.setIORegister(avr.IO_TWSR, (ack and 0x80) or 0x88)
            else -- sreadgen
                avr.setIORegister(avr.IO_TWSR, (ack and 0x90) or 0x98)
            end
            log("Received slave data:", d, ack, "\n")
            maybeexecisr = true
        elseif msg == "mreadack" and TWIInfo.state == "swrite" then
            local ack = selectOne(1, ...)
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
end

local function handleMasterEvent(data)
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
    -- SLA+W / SLA+R / data?
    elseif bit.isSet(data, avr.TWINT) and
           not bit.isSet(data, avr.TWSTA) and not bit.isSet(data, avr.TWSTO) then
        if TWIInfo.state ~= "idle" then
            local d = avr.getIORegister(avr.IO_TWDR)
            if TWIInfo.state == "mstart" then -- SLA?
                if bit.isSet(d, 0) then -- MR read
                    TWIInfo.state = "mread"
                    d = bit.unSet(d, 0)
                    log(string.format("SLA+R: %d\n", d))
                    if TWIInfo.slaveAddress ~= d then
                        -- Wrong slave
                        avr.setIORegister(avr.IO_TWSR, 0x48) -- NACK
                        maybeexecisr = true
                    else
                        avr.sendTWIMSG("slavereadadr")
                    end
                else -- MR write
                    TWIInfo.state = "mwrite"
                    log(string.format("SLA+W: %d\n", d))
                    if TWIInfo.slaveAddress == d then
                        avr.sendTWIMSG("slavewriteadr")
                    elseif d == 0 and TWIInfo.slaveAcceptsGenCall then
                        avr.sendTWIMSG("slavewriteadrgen")
                    else -- Wrong slave
                        avr.setIORegister(avr.IO_TWSR, 0x20) -- NACK
                        maybeexecisr = true
                    end
                end
            elseif TWIInfo.state == "mwrite" then  -- plain data
                avr.sendTWIMSG("mdata", d)
--                log("sending data to slave\n")
            end
        else
            warning("Ignoring data: SLA+R/W received without master start condition!\n")
        end
    -- Master stop condition?
    elseif bit.isSet(data, avr.TWINT, avr.TWSTO) and
           not bit.isSet(data, avr.TWSTA) then
        log("Master stop condition\n")
        avr.sendTWIMSG("stopslave")
        TWIInfo.state = "idle"
    else
        warning(string.format("Ignoring unknown master event: %d\n", data))
    end

    if maybeexecisr and bit.isSet(data, avr.TWIE) then
        avr.execISR(avr.ISR_TWI_vect)
    end
end

local function handleSlaveEvent(data)
    local maybeexecisr = false

    -- Specs say that TWSTO must be zero when writing
    if not bit.isSet(data, avr.TWSTO) then
        if TWIInfo.state == "swrite" and TWIInfo.slaveData ~= nil then -- data set?
            avr.sendTWIMSG("sdata", TWIInfo.slaveData)
            TWIInfo.slaveData = nil
        end
    end

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

    avr.setIORegisterIgnoreEqual(avr.IO_TWDR, true)
    avr.setIORegisterIgnoreEqual(avr.IO_TWCR, true)

    avr.setTWIMSGHandler(TWIMSGHandler)
end

function handleIOData(type, data)
    if type == avr.IO_TWAR then -- Slave address
        TWIInfo.isMaster = false
        local adr = bit.unSet(data, 0)
        local acceptgen = bit.isSet(data, 0)
        log(string.format("Setting slave address to %d\n", adr))
        log(string.format("Slave accepts gen calls: %s\n", tostring(acceptgen)))
        updateRobotStatus("TWI", "slave adr", adr)
        updateRobotStatus("TWI", "accept gen call", tostring(acceptgen))
        avr.sendTWIMSG("sadr", adr, acceptgen)
    elseif type == avr.IO_TWBR and TWIInfo.isMaster then -- Bitrate
        -- From atmega32 docs: freq=CPU_clock/(16+2*TWBR*4^prescaler)
        -- Note that the driver currently does not handle any prescalers
        local freq = properties.clockSpeed / (16+2*data)
        log(string.format("Setting bit rate to %d (%d Hz)\n", data, freq))
        updateRobotStatus("TWI", "bitrate", data)
        updateRobotStatus("TWI", "frequency (Hz)", freq)
    elseif type == avr.IO_TWDR and not TWIInfo.isMaster then
        if bit.isSet(avr.getIORegister(avr.IO_TWCR), avr.TWEN) then
            TWIInfo.slaveData = data
        end
    elseif type == avr.IO_TWSR and TWIInfo.isMaster then -- Prescaler (on write access)
        if data ~= 0 then
            warning("TWI driver does not support any prescaler!")
        end
    elseif type == avr.IO_TWCR then
        if bit.isSet(data, avr.TWINT) then
            avr.setIORegister(avr.IO_TWCR, bit.unSet(data, avr.TWINT))
            if bit.isSet(data, avr.TWEN) then
                if TWIInfo.isMaster then
                    handleMasterEvent(data)
                else
                    handleSlaveEvent(data)
                end
            end
        end
    end
end


return ret
