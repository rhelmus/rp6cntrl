local ret = driver(...)

handledIORegisters = {
    avr.IO_TCCR1A,
    avr.IO_TCCR1B,
    avr.IO_OCR1A,
    avr.IO_OCR1AL,
    avr.IO_OCR1AH,
    avr.IO_OCR1B,
    avr.IO_OCR1BL,
    avr.IO_OCR1BH,
    avr.IO_ICR1,
    avr.IO_TIMSK
}

local motorInfo = {
    enabled = true,
    prescaler = 0,
    compareValueA = 0,
    compareValueB = 0,
    inputRegister = 0,
}

local LeftEncTimer, rightEncTimer

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
            leftEncTimer:setPrescaler(ps)
            rightEncTimer:setPrescaler(ps)
        end

        ret = (ps ~= nil)
        if not ret then
            warning("Unsupported PWM prescaler set for timer1\n")
        end
    end

    return checkCond(cond, "Unsupported TCCR1B PWM settings found.\n")
end

local function setCompareRegisterA(data)
--    log(string.format("Setting timer1 PWM compare A to %d\n", data))
    motorInfo.compareValueA = data
    if data == 0 then
        if rightEncTimer:isEnabled() then
            clock.enableTimer(rightEncTimer, false)
        end
    else
        -- UNDONE: cpu speed, comments
        rightEncTimer:setCompareValue(8000000 / (data * 5))
        if not rightEncTimer:isEnabled() then
            clock.enableTimer(rightEncTimer, true)
        end
    end
    return true
end

local function setCompareRegisterB(data)
--    log(string.format("Setting timer1 PWM compare B to %d\n", data))
    motorInfo.compareValueB = data
    if data == 0 then
        if leftEncTimer:isEnabled() then
            clock.enableTimer(leftEncTimer, false)
        end
    else
        -- UNDONE: cpu speed, comments
        leftEncTimer:setCompareValue((8000000 / (data * 5)))
        if not leftEncTimer:isEnabled() then
            clock.enableTimer(leftEncTimer, true)
        end
    end
    return true
end

local function setInputRegister(data)
    log(string.format("Setting timer1 PWM input register to %d\n", data))
    motorInfo.inputRegister = data
    return true
end


function initPlugin()
    leftEncTimer = clock.createTimer()
    leftEncTimer:setTimeOut(avr.ISR_INT0_vect)
    rightEncTimer = clock.createTimer()
    rightEncTimer:setTimeOut(avr.ISR_INT1_vect)
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
            avr.setIORegister(avr.IO_OCR1AL, bit.lower(data, 8))
            avr.setIORegister(avr.IO_OCR1AH, bit.upper(data, 8))
        elseif type == avr.IO_OCR1AL then
            local v = bit.unPack(data, bit.upper(motorInfo.compareValueA, 8), 8)
            setCompareRegisterA(v)
            avr.setIORegister(avr.IO_OCR1A, v)
        elseif type == avr.IO_OCR1AH then
            local v = bit.unPack(motorInfo.compareValueA, data, 8)
            setCompareRegisterA(v)
            avr.setIORegister(avr.IO_OCR1A, v)
        elseif type == avr.IO_OCR1B then
            setCompareRegisterB(data)
            avr.setIORegister(avr.IO_OCR1BL, bit.lower(data, 8))
            avr.setIORegister(avr.IO_OCR1BH, bit.upper(data, 8))
        elseif type == avr.IO_OCR1BL then
            local v = bit.unPack(data, bit.upper(motorInfo.compareValueB, 8), 8)
            setCompareRegisterB(v)
            avr.setIORegister(avr.IO_OCR1B, v)
        elseif type == avr.IO_OCR1BH then
            local v = bit.unPack(motorInfo.compareValueB, data, 8)
            setCompareRegisterB(v)
            avr.setIORegister(avr.IO_OCR1B, v)
        elseif type == avr.IO_ICR1 then
            setInputRegister(data)
        end
    end
end

return ret
