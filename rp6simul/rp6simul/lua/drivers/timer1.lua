module(..., package.seeall)

-- UNDONE: Channel B support?
-- UNDONE: L/H OCR support (and other timers?)
handledIORegisters = {
    avr.IO_TCCR1A,
    avr.IO_TCCR1B,
    avr.IO_OCR1A,
--    avr.IO_OCR1B,
--    avr.IO_OCR1AL,
--    avr.IO_OCR1AH,
--    avr.IO_OCR1BL,
--    avr.IO_OCR1BH,
    avr.IO_TIMSK
}

local timer
local prescaler = 0

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

local function disableTimerCond(cond, reason)
    if not cond then
        warning(reason)
        if timer:isEnabled() then
            warning("Disabling timer1\n")
            clock:setTimerEnabled(timer, false)
        end
    end

    return cond
end

local function setControlRegisterA(data)
    debug("reg1A:", data)
    return disableTimerCond((data == 0), "Unsupported flags for TCCR1A set\n")
end

local function setControlRegisterB(data)
    -- Only prescaler and CTC flags supported
    local ret = ((bit.unset(data, avr.CS10, avr.CS11, avr.CS12, avr.WGM12) == 0) and
                 (bit.isSet(data, avr.WGM12)))

    if ret then
        local ps = getPrescaler(data)
        if ps and ps ~= prescaler then
            prescaler = ps
            timer:setPrescaler(ps)
            log(string.format("Changed timer1 prescaler to %d\n", ps))
        end

        ret = (ps ~= nil)
        if not ret then
            warning("Unsupported prescaler set for timer1")
        end
    end

    return disableTimerCond(ret, "Unsupported flags for TCCR1B set\n")
end

local function setCompareRegisterA(data)
    log(string.format("Setting timer1 compare A to %d\n", data))
    timer:setCompareValue(data)
    return true
end

function initPlugin()
    timer = clock.createTimer()
    timer:setTimeOut(avr.ISR_TIMER1_COMPA_vect)
end

function handleIOData(type, data)
    -- As timer1 is also used for PWM (motors) we only handle settings when
    -- the timer is/gets enabled.

    if type == avr.IO_TIMSK then
        local e = bit.isSet(data, avr.OCIE1A)
        if e ~= timer:isEnabled() then
            if e then
                local stat = setControlRegisterA(avr.getIORegister(avr.IO_TCCR1A))

                if stat then
                    stat = setControlRegisterB(avr.getIORegister(avr.IO_TCCR1B))
                    if stat then
                        stat = setCompareRegisterA(avr.getIORegister(avr.IO_OCR1A))
                    end
                end

                if stat then
                    log("Enabling timer1\n")
                    clock.enableTimer(timer, e)
                else
                    warning("Not enabling timer1\n")
                end
            else
                log("Disabling timer1\n")
                clock.enableTimer(timer, e)
            end
        end
    elseif timer:isEnabled() then
        if type == avr.IO_TCCR1A then
            setControlRegisterA(data)
        elseif type == avr.IO_TCCR1B then
            setControlRegisterB(data)
        elseif type == avr.IO_OCR1A then
            setCompareRegisterA(data)
        end
    end
end

-- UNDONE! Reset timer
