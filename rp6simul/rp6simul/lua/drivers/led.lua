module(..., package.seeall)

-- UNDONE: m32 leds support

-- Leds (RP6):
--      1: PORTC, pin 4
--      2: PORTC, pin 5
--      3: PORTC, pin 6
--      4: PORTB, pin 7
--      5: PORTB, pin 1
--      6: PORTB, pin 0

handledIORegisters = {
    avr.IO_PORTB,
    avr.IO_PORTC,
    avr.IO_DDRB,
    avr.IO_DDRC
}

local LEDInfo = {
    curPORTB = 0,
    curPORTC = 0,
    curDDRB = 0,
    curDDRC = 0,
    LEDStatus = { false, false, false, false, false, false }
}

local function updateLEDs(port)
    local function update(l, p, fp, d, fd)
        local e = (bit.isSet(p, fp) and bit.isSet(d, fd))
        if e ~= LEDInfo.LEDStatus[l] then
            log(string.format("LED %d %s\n", l, (e and "enabled") or "disabled"))
            LEDInfo.LEDStatus[l] = e
        end
    end

    if port == "C" then
        update(1, LEDInfo.curPORTC, avr.PC4, LEDInfo.curDDRC, avr.DDC4)
        update(2, LEDInfo.curPORTC, avr.PC5, LEDInfo.curDDRC, avr.DDC5)
        update(3, LEDInfo.curPORTC, avr.PC6, LEDInfo.curDDRC, avr.DDC6)
    elseif port == "B" then
        update(4, LEDInfo.curPORTB, avr.PB7, LEDInfo.curDDRB, avr.DDB7)
        update(5, LEDInfo.curPORTB, avr.PB1, LEDInfo.curDDRB, avr.DDB1)
        update(6, LEDInfo.curPORTB, avr.PB0, LEDInfo.curDDRB, avr.DDB0)
    end
end

function handleIOData(type, data)
    local uport

    if type == avr.IO_PORTB then
        LEDInfo.curPORTB = data
        uport = "B"
    elseif type == avr.IO_PORTC then
        LEDInfo.curPORTC = data
        uport = "C"
    elseif type == avr.IO_DDRB then
        LEDInfo.curDDRB = data
        uport = "B"
    elseif type == avr.IO_DDRC then
        LEDInfo.curDDRC = data
        uport = "C"
    end
    updateLEDs(uport)
end
