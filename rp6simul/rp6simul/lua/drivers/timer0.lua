local ret = driver(...)

description = "Driver for timer0"

handledIORegisters = { avr.IO_TCCR0, avr.IO_OCR0, avr.IO_TIMSK }

local timer
local prescaler = 0

local function getPrescaler(data)
    local clockSet0 = bit.isSet(data, avr.CS00)
    local clockSet1 = bit.isSet(data, avr.CS01)
    local clockSet2 = bit.isSet(data, avr.CS02)

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

local function checkSettings()
    local tccr0 = avr.getIORegister(avr.IO_TCCR0)
    local ret = true

    if not (not bit.isSet(tccr0, avr.COM00, avr.COM01, avr.WGM00) and
            bit.isSet(tccr0, avr.WGM01)) then
        warning("Incompatible settings for timer0\n")
        ret = false
    end

    if not getPrescaler(tccr0) then
        warning("Unsupported prescaler set for timer0\n")
        ret = false
    end

    return ret
end

local function setEnabled(e)
    clock.enableTimer(timer, e)
    updateRobotStatus("timer0", "status", (e and "enabled") or "disabled")
end


function initPlugin()
    timer = clock.createTimer()
    timer:setTimeOut(avr.ISR_TIMER0_COMP_vect)
    setEnabled(false)
end

function handleIOData(type, data)
    if type == avr.IO_TCCR0 then
        local ps = getPrescaler(data)
        if ps and ps ~= prescaler then
            prescaler = ps
            timer:setPrescaler(ps)
            log(string.format("Changed timer0 prescaler to %d\n", ps))
            updateRobotStatus("timer0", "prescaler", ps)
        end

        if timer:isEnabled() and not checkSettings() then
            warning("Disabling timer0\n")
            setEnabled(false)
        end
    elseif type == avr.IO_OCR0 then
        log(string.format("Setting timer0 compare to %d\n", data))
        updateRobotStatus("timer0", "Compare value", data)
        timer:setCompareValue(data)
    elseif type == avr.IO_TIMSK then
        local e = bit.isSet(data, avr.OCIE0)
        if e ~= timer:isEnabled() then
            if e then
                if not checkSettings() then
                    warning("Not enabling timer0\n")
                else
                    log("Enabling timer0\n")
                    setEnabled(true)
                end
            else
                log("Disabling timer0\n")
                setEnabled(false)
            end
        end
    end
end

return ret
