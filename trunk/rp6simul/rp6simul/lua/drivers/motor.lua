local ret = driver(...)

description = "Driver for the RP6 motor."

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
    avr.IO_TIMSK,
    avr.IO_PORTC
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
        -- Every 200 msec (ie. SPEED_TIMER_BASE) a counter is read and reset.
        -- This counter is incremented from the callback functions of the
        -- motor encoders (ie. INT0_vect/INT1_vect ISRs). For simulation the
        -- encoders are each simply coupled to a timer. The update frequency
        -- (compare value) of these 'encoder timers' are set using the
        -- following formula: CV = MHz / (power * 5)
        -- (1000 msec/200 msec = 5)
        -- Thus the motor power and resulting speed calculated from the RP6
        -- lib code is (mostly) equalized.
        -- UNDONE: cpu speed configurable
        -- UNDONE: 200 msec configurable?
        rightEncTimer:setCompareValue(8000000 / (data * 5))
        if not rightEncTimer:isEnabled() then
            clock.enableTimer(rightEncTimer, true)
        end
    end
    updateRobotStatus("motor", "power", "right", data)
    setMotorPower("right", data)
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
        -- Comments: see setCompareRegisterA
        leftEncTimer:setCompareValue((8000000 / (data * 5)))
        if not leftEncTimer:isEnabled() then
            clock.enableTimer(leftEncTimer, true)
        end
    end
    updateRobotStatus("motor", "power", "left", data)
    setMotorPower("left", data)
    return true
end

local function setInputRegister(data)
    log(string.format("Setting timer1 PWM input register to %d\n", data))
    motorInfo.inputRegister = data
    return true
end

local function setMotorDirection(data)
    local ldir = (bit.isSet(data, avr.PINC2) and "BWD") or "FWD"
    local rdir = (bit.isSet(data, avr.PINC3) and "BWD") or "FWD"
    setMotorDir(ldir, rdir)
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
        elseif type == avr.IO_PORTC then
            setMotorDirection(data)
        end
    end
end

return ret
