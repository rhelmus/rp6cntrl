local ret = driver(...)

description = "Driver for timer2."

handledIORegisters = { avr.IO_TCCR2, avr.IO_OCR2, avr.IO_TIMSK }

local timer
local prescaler = 0

local function getPrescaler(data)
    local clockSet0 = bit.isSet(data, avr.CS20)
    local clockSet1 = bit.isSet(data, avr.CS21)
    local clockSet2 = bit.isSet(data, avr.CS22)

    if not clockSet0 and not clockSet1 and not clockSet2 then
        return 0
    elseif clockSet0 and not clockSet1 and not clockSet2 then
        return 1
    elseif not clockSet0 and clockSet1 and not clockSet2 then
        return 8
    elseif clockSet0 and clockSet1 and not clockSet2 then
        return 32
    elseif not clockSet0 and not clockSet1 and clockSet2 then
        return 64
    elseif clockSet0 and not clockSet1 and clockSet2 then
        return 128
    elseif not clockSet0 and clockSet1 and clockSet2 then
        return 256
    elseif clockSet0 and clockSet1 and clockSet2 then
        return 1024
    end

    -- return nil
end

local function setEnabled(e)
    clock.enableTimer(timer, e)
    updateRobotStatus("timer2", "status", (e and "enabled") or "disabled")
end

local function checkSettings()
    local tccr2 = avr.getIORegister(avr.IO_TCCR2)
    local ret = true

    -- UNDONE: COM20 is set for m32-control's beeper
    if not (not bit.isSet(tccr2, avr.COM20, avr.COM21, avr.WGM20) and
            bit.isSet(tccr2, avr.WGM21)) then
        warning("Incompatible settings for timer2\n")
        ret = false
    end

    if not getPrescaler(tccr2) then
        warning("Unsupported prescaler set for timer2\n")
        ret = false
    end

    return ret
end

function initPlugin()
    timer = clock.createTimer()
    timer:setTimeOut(avr.ISR_TIMER2_COMP_vect)
    setEnabled(false)
end

function handleIOData(type, data)
    if type == avr.IO_TCCR2 then
        local ps = getPrescaler(data)
        if ps and ps ~= prescaler then
            prescaler = ps
            timer:setPrescaler(ps)
            log(string.format("Changed timer2 prescaler to %d\n", ps))
            updateRobotStatus("timer2", "prescaler", ps)
        end

        if timer:isEnabled() and not checkSettings() then
            warning("Disabling timer2\n")
            setEnabled(false)
        end
    elseif type == avr.IO_OCR2 then
        log(string.format("Setting timer2 compare to %d\n", data))
        updateRobotStatus("timer2", "Compare value", data)
        timer:setCompareValue(data)
    elseif type == avr.IO_TIMSK then
        local e = bit.isSet(data, avr.OCIE2)
        if e ~= timer:isEnabled() then
            if e then
                if not checkSettings() then
                    warning("Not enabling timer2\n")
                else
                    log("Enabling timer2\n")
                    setEnabled(true)
                end
            else
                log("Disabling timer2\n")
                setEnabled(false)
            end
        end
    end
end

function closePlugin()
    timer = nil
end


return ret
