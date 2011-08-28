local ret = driver(...)

description = "Driver for IRCOMM receiving and transmission."

handledIORegisters = {
    avr.IO_PORTD,
}

local IRCOMMInfo =
{
    isEmitting = false,
    emitCounter = 0,
    lastPulse = 0,

    -- IRCOMM decoding
    pulseTimeMin = math.floor(10e3 * 1.778e-3 * 0.4 + 0.5),
    pulseTime1_2 = math.floor(10e3 * 1.778e-3 * 0.8 + 0.5),
    pulseTimeMax = math.floor(10e3 * 1.778e-3 * 1.2 + 0.5),
    pulseToggle = false,
    pulseTmp = 0,
    pulseTimer = 0,
}

local timer
local timerCounter, timerMSCounter = 0, 0

local function parseIRCOMM(pulsing)
    -- IRCOMM parsing, based on RP6 lib code
    IRCOMMInfo.pulseTimer = IRCOMMInfo.pulseTimer + 1
--    log("pulseTimer:", IRCOMMInfo.pulseTimer, "\n")

    local tmp = IRCOMMInfo.pulseTmp
    if IRCOMMInfo.pulseTimer > IRCOMMInfo.pulseTimeMax then
        if not bit.isSet(tmp, 14) and bit.isSet(tmp, 13) then
            log("Received RC5 data:", tmp, IRCOMMInfo.pulseToggle, "\n")
        end
        tmp = 0
    end
    if IRCOMMInfo.pulseToggle ~= pulsing then
        IRCOMMInfo.pulseToggle = not IRCOMMInfo.pulseToggle
        if IRCOMMInfo.pulseTimer < IRCOMMInfo.pulseTimeMin then
            -- Too short pulse
            tmp = 0
        end
        if tmp == 0 or IRCOMMInfo.pulseTimer > IRCOMMInfo.pulseTime1_2 then
            if not bit.isSet(tmp, 14) then
                tmp = bit.shiftLeft(tmp, 1)
            end
            if IRCOMMInfo.pulseToggle then
                tmp = bit.set(tmp, 0)
            end
            IRCOMMInfo.pulseTimer = 0
        end
    end
    IRCOMMInfo.pulseTmp = tmp
--    log("tmp:", tmp, "\n")
end

function initPlugin()
    timer = clock.createTimer()
    timer:setCompareValue(99 * 8) -- every ~100 us (similar to timer0)
    timer:setTimeOut(function()
        timerCounter = timerCounter + 1
        if timerCounter == 10 then
            timerCounter = 0
            timerMSCounter = timerMSCounter + 1
        end
        local pulsing = (IRCOMMInfo.emitCounter >= 3)
        IRCOMMInfo.emitCounter = 0

        if pulsing then
            IRCOMMInfo.lastPulse = timerMSCounter
        elseif (timerMSCounter - IRCOMMInfo.lastPulse) > 100 then
            -- Shutdown timer after inactivity
            clock.enableTimer(timer, false)
            timerCounter, timerMSCounter = 0, 0
            IRCOMMInfo.lastPulse = 0
            return
        end

        parseIRCOMM(pulsing)
    end)
end

function handleIOData(type, data)
    -- Always PORTD
    local emitting = bit.isSet(data, avr.PIND7)
    if emitting ~= IRCOMMInfo.isEmitting then
        IRCOMMInfo.isEmitting = emitting
        if emitting then
            if not timer:isEnabled() then
                clock.enableTimer(timer, true)
            end
            IRCOMMInfo.emitCounter = IRCOMMInfo.emitCounter + 1
        end
    end
end

function closePlugin()
    timer = nil
end


return ret
