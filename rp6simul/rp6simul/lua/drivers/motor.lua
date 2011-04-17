local ret = driver(...)

-- UNDONE: Channel B support?
-- UNDONE: L/H OCR support (and other timers?)
handledIORegisters = {
    avr.IO_TCCR1A,
    avr.IO_TCCR1B,
    avr.IO_OCR1A,
--    avr.IO_OCR1B,
    avr.IO_OCR1AL,
    avr.IO_OCR1AH,
--    avr.IO_OCR1BL,
--    avr.IO_OCR1BH,
    avr.IO_ICR1,
    avr.IO_TIMSK
}

local motorInfo = {
    enabled = true,
    prescaler = 0,
    compareValue = 0,
    inputRegister = 0,
}

-- Essentially same function from timer1.lua
local function getPrescaler(data)
    local clockSet0 = bit.isSet(data, avr.CS10)
    local clockSet1 = bit.isSet(data, avr.CS11)
    local clockSet2 = bit.isSet(data, avr.CS12)

    if not clockSet0 and not clockSet1 and not clockSet2 then
        return 0
    elseif clockSet0 and not clockSet1 and not clockSet2 then
        return 1
    elseif not clockSet0 and clockSet1 and not clockSet2 then
        return 8
    elseif clockSet0 and clockSet1 and not clockSet2 then
        return 64
    elseif not clockSet0 and not clockSet1 and clockSet2 then
        return 256
    elseif clockSet0 and not clockSet1 and clockSet2 then
        return 1024
    end

    -- return nil
end

local function checkCond(cond, msg)
    if not cond then
        warning(msg)
        if motorInfo.enabled then
            warning("Disabling motor driver.\n")
            motorInfo.enabled = false
        end
    end
    return cond
end

local function setControlRegisterA(data)
    local cond = (bit.unSet(data, avr.WGM11, avr.COM1A1, avr.COM1B1) == 0)
    return checkCond(cond, "Unsupported TCCR1A PWM settings found.\n")
end

local function setControlRegisterB(data)
    local cond = ((bit.unSet(data, avr.WGM13, avr.CS10, avr.CS11, avr.CS12) == 0) and
                  bit.isSet(data, avr.WGM13))

    if ret then
        local ps = getPrescaler(data)
        if ps and ps ~= motorInfo.prescaler then
            motorInfo.prescaler = ps
            log(string.format("Changed timer1 PWM prescaler to %d\n", ps))
        end

        ret = (ps ~= nil)
        if not ret then
            warning("Unsupported PWM prescaler set for timer1\n")
        end
    end

    return checkCond(cond, "Unsupported TCCR1B PWM settings found.\n")

end

local function setCompareRegisterA(data)
    log(string.format("Setting timer1 PWM compare A to %d\n", data))
    motorInfo.compareValue = data
    return true
end

local function setInputRegister(data)
    log(string.format("Setting timer1 PWM input register to %d\n", data))
    motorInfo.inputRegister = data
    return true
end

function handleIOData(type, data)
    if type == avr.IO_TIMSK then
        -- Disable motor control when timer1 is enabled
        if motorInfo.enabled and (bit.isSet(data, avr.OCIE1A) or
            bit.isSet(data, avr.OCIE1B)) then
            motorInfo.enabled = false
            log("Disabling motor control: timer1 enabled")
        end
    elseif motorInfo.enabled then
        if type == avr.IO_TCCR1A then
            setControlRegisterA(data)
        elseif type == avr.IO_TCCR1B then
            setControlRegisterB(data)
        elseif type == avr.IO_OCR1A then
            setCompareRegisterA(data)
        elseif type == avr.IO_OCR1AL then
            setCompareRegisterA(bit.unPack(data, motorInfo.compareValue, 8))
        elseif type == avr.IO_OCR1AH then
            setCompareRegisterA(bit.unPack(motorInfo.compareValue, data, 8))
        elseif type == avr.IO_ICR1 then
            setInputRegister(data)
        end
    end
end

return ret
