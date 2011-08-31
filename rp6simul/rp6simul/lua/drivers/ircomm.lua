local ret = driver(...)

description = "Driver for IRCOMM receiving and transmission."

handledIORegisters = {
    avr.IO_PORTD,
}

local IRCOMMInfo =
{
    robotIsEmitting = false,
    emitCounter = 0,
    lastPulse = 0,

    -- IRCOMM decoding
    pulseTimeMin = math.floor(10e3 * 1.778e-3 * 0.4 + 0.5),
    pulseTime1_2 = math.floor(10e3 * 1.778e-3 * 0.8 + 0.5),
    pulseTimeMax = math.floor(10e3 * 1.778e-3 * 1.2 + 0.5),
    pulseToggle = false,
    pulseTmp = 0,
    pulseTimer = 0,

    -- IRCOMM encoding
    dataToSend = 0,
    curSendBit = 0,
    sendToggle = false,
}

local receiveTimer
local receiveTimerCounter, receiveTimerMSCounter = 0, 0
local sendTimer

local function parseIRCOMM(pulsing)
    -- IRCOMM parsing, based on RP6 lib code
    IRCOMMInfo.pulseTimer = IRCOMMInfo.pulseTimer + 1

    local tmp = IRCOMMInfo.pulseTmp
    if IRCOMMInfo.pulseTimer > IRCOMMInfo.pulseTimeMax then
        if not bit.isSet(tmp, 14) and bit.isSet(tmp, 13) then
            local adr = bit.shiftRight(bit.bitAnd(tmp, 0x7C0), 6)
            local key = bit.bitAnd(tmp, 0x3F)
            local toggle = bit.isSet(tmp, 11)
            logIRCOMM(adr, key, toggle)
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
end

local function receiveTimeOut()
    receiveTimerCounter = receiveTimerCounter + 1

    if receiveTimerCounter == 10 then
        receiveTimerCounter = 0
        receiveTimerMSCounter = receiveTimerMSCounter + 1
    end

    local pulsing = (IRCOMMInfo.emitCounter >= 3)
    IRCOMMInfo.emitCounter = 0

    if pulsing then
        IRCOMMInfo.lastPulse = receiveTimerMSCounter
    elseif (receiveTimerMSCounter - IRCOMMInfo.lastPulse) > 100 then
        -- Shutdown timer after inactivity
        clock.enableTimer(receiveTimer, false)
        receiveTimerCounter, receiveTimerMSCounter = 0, 0
        IRCOMMInfo.lastPulse = 0
        return
    end

    parseIRCOMM(pulsing)
end

local function sendIRCOMM(adr, key, toggle)
    if sendTimer:isEnabled() then
        warning("Not sending IRCOMM data: already active!\n")
        return
    end

    if toggle then
        adr = bit.bitOr(adr, 32)
    end

    -- Convert to single bitwise variable
    IRCOMMInfo.dataToSend = 0x3000 -- Enable first 2 bits
    IRCOMMInfo.dataToSend = bit.bitOr(IRCOMMInfo.dataToSend,
                                      bit.shiftLeft(bit.bitAnd(adr, 0x3F), 6))
    IRCOMMInfo.dataToSend = bit.bitOr(IRCOMMInfo.dataToSend,
                                      bit.bitAnd(key, 0x3F))

    IRCOMMInfo.curSendBit = 13
    IRCOMMInfo.sendToggle = false
    clock.enableTimer(sendTimer, true)
end

local function sendTimeOut()
    -- IRCOMM sending, based on RP6 lib code

    if IRCOMMInfo.curSendBit < 0 then -- Finished sending packet
        avr.setIORegister(avr.IO_PINB,
                          bit.unSet(avr.getIORegister(avr.IO_PINB), avr.PINB2))
        clock.enableTimer(sendTimer, false)
        return
    end

    local dopulse
    if not IRCOMMInfo.sendToggle then -- Actual data
        dopulse = bit.isSet(IRCOMMInfo.dataToSend, IRCOMMInfo.curSendBit)
    else -- Manchester opposite
        dopulse = not bit.isSet(IRCOMMInfo.dataToSend, IRCOMMInfo.curSendBit)
        IRCOMMInfo.curSendBit = IRCOMMInfo.curSendBit - 1
    end

    IRCOMMInfo.sendToggle = not IRCOMMInfo.sendToggle

    local curv = avr.getIORegister(avr.IO_PINB)
    if dopulse then
        avr.setIORegister(avr.IO_PINB, bit.set(curv, avr.PINB2))
    else
        avr.setIORegister(avr.IO_PINB, bit.unSet(curv, avr.PINB2))
    end
end

function initPlugin()
    receiveTimer = clock.createTimer()
    receiveTimer:setCompareValue(99 * 8) -- every ~100 us (similar to timer0)
    receiveTimer:setTimeOut(receiveTimeOut)

    sendTimer = clock.createTimer()
    sendTimer:setCompareValue(7112--[[11779]]) -- called every (1.778/2) ms
    sendTimer:setTimeOut(sendTimeOut)

    setIRCOMMSendHandler(sendIRCOMM)
end

function handleIOData(type, data)
    -- Always PORTD
    local emitting = bit.isSet(data, avr.PIND7)
    if emitting ~= IRCOMMInfo.robotIsEmitting then
        IRCOMMInfo.robotIsEmitting = emitting
        if emitting then
            if not receiveTimer:isEnabled() then
                clock.enableTimer(receiveTimer, true)
            end
            IRCOMMInfo.emitCounter = IRCOMMInfo.emitCounter + 1
        end
    end
end

function closePlugin()
    receiveTimer, sendTimer = nil, nil
end


return ret
