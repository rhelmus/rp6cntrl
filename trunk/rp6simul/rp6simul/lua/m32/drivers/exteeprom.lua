local ret = driver(...)

description = "External SPI controlled EEPROM driver."

handledIORegisters = {
    avr.IO_PORTB,
    avr.IO_SPDR,
}


local EEPROMInfo =
{
    cycleStarted = false,
    state = nil,
    upperAdr = nil,
    address = nil,
    writeEnabled = false,
}

local function handleEEPROMData(data)
    if EEPROMInfo.state == nil then
        if data == 0x03 then EEPROMInfo.state = "read"
        elseif data == 0x02 then EEPROMInfo.state = "write"
        elseif data == 0x06 then EEPROMInfo.writeEnabled = true
        elseif data == 0x04 then EEPROMInfo.writeEnabled = false
        elseif data == 0x05 then EEPROMInfo.state = "readstat"
        elseif data == 0x01 then -- write status
            error("Setting write status register is unsupported!\n");
        end
    elseif (EEPROMInfo.state == "read" or EEPROMInfo.state == "write") and
            not EEPROMInfo.address then
        if not EEPROMInfo.upperAdr then
            EEPROMInfo.upperAdr = data
        else
            EEPROMInfo.address = bit.unPack(data, EEPROMInfo.upperAdr, 8)
        end
    elseif EEPROMInfo.state == "read" then
        if data == 0xFF then
            avr.setIORegister(avr.IO_SPDR, getExtEEPROM(EEPROMInfo.address))
            EEPROMInfo.address = EEPROMInfo.address + 1
        else
            warning("Unknown EEPROM read instruction\n")
        end
    elseif EEPROMInfo.state == "write" then
        log("EEPROM write data:", EEPROMInfo.address, data, "\n")
        if not EEPROMInfo.writeEnabled then
            warning("Ignoring EEPROM write: writing disabled\n")
        else
            -- The EEPROM spec tells us that only the 6 lower bit are
            -- incremented, thus when exceeding the page size (64) a
            -- rollover occurs. Note that reading is always contineous.

            setExtEEPROM(EEPROMInfo.address, data)

            -- Increment, but only keep 6 lowest bits
            inc = bit.bitAnd(EEPROMInfo.address + 1, tonumber(111111, 2))
            -- Clear current 6 bits
            EEPROMInfo.address = bit.bitAnd(EEPROMInfo.address,
                                            tonumber("1111111111000000", 2))
            -- ... and put the 6 bits of the increment back
            EEPROMInfo.address = bit.bitOr(EEPROMInfo.address, inc)
        end
    elseif EEPROMInfo.state == "readstat" then
        if data == 0xFF then
            local stat = 0 -- First bit zero: always ready
            if EEPROMInfo.writeEnabled then
                stat = bit.set(stat, 1)
            end
            avr.setIORegister(avr.IO_SPDR, stat)
        else
            warning("Unknown EEPROM read status instruction\n")
        end
    end
end

function handleIOData(type, data)
    if type == avr.IO_PORTB then
        if not bit.isSet(data, avr.PINB0) then
            EEPROMInfo.cycleStarted = true
        elseif EEPROMInfo.cycleStarted then
            if EEPROMInfo.state == "write" then
                -- Reset automatically according to specs
                EEPROMInfo.writeEnabled = false
            end
            EEPROMInfo.cycleStarted = false
            EEPROMInfo.state = nil
            EEPROMInfo.upperAdr = nil
            EEPROMInfo.address = nil
        end
    elseif type == avr.IO_SPDR and EEPROMInfo.cycleStarted then
        handleEEPROMData(data)
    end
end


return ret
