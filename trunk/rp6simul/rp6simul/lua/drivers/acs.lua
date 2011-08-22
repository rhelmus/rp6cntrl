local ret = driver(...)

description = "Driver for the Anti Collision System (ACS). Does not handle IRCOMM."

handledIORegisters = {
    avr.IO_PORTB,
    avr.IO_PORTC,
    avr.IO_PORTD,
    avr.IO_DDRB,
    avr.IO_DDRD,
}


local ACSFlags = {
    ACS = avr.PINB2,
    ACS_L = avr.PINB6,
    ACS_R = avr.PINC7,
    ACS_PWR = avr.PIND6,
    ACS_PWRH = avr.PINB3,
}

local ACSInfo = {
    power = "off",
    leftEnabled = false,
    rightEnabled = false,
}


local function updateACSPower(type, data)
    local function getreg(t)
        return (t == type and data) or avr.getIORegister(t)
    end

    local DDRB = getreg(avr.IO_DDRB)
    local DDRD = getreg(avr.IO_DDRD)
    local PORTB = getreg(avr.IO_PORTB)
    local PORTD = getreg(avr.IO_PORTD)

    local stat
    if not bit.isSet(DDRD, ACSFlags.ACS_PWR) and
       not bit.isSet(PORTD, ACSFlags.ACS_PWR) and
       not bit.isSet(DDRB, ACSFlags.ACS_PWRH) and
       not bit.isSet(PORTB, ACSFlags.ACS_PWRH) then
        stat = "off"
    elseif bit.isSet(DDRD, ACSFlags.ACS_PWR) and
       bit.isSet(PORTD, ACSFlags.ACS_PWR) and
       not bit.isSet(DDRB, ACSFlags.ACS_PWRH) and
       not bit.isSet(PORTB, ACSFlags.ACS_PWRH) then
        stat = "low"
    elseif not bit.isSet(DDRD, ACSFlags.ACS_PWR) and
       not bit.isSet(PORTD, ACSFlags.ACS_PWR) and
       bit.isSet(DDRB, ACSFlags.ACS_PWRH) and
       bit.isSet(PORTB, ACSFlags.ACS_PWRH) then
        stat = "medium"
    elseif bit.isSet(DDRD, ACSFlags.ACS_PWR) and
       bit.isSet(PORTD, ACSFlags.ACS_PWR) and
       bit.isSet(DDRB, ACSFlags.ACS_PWRH) and
       bit.isSet(PORTB, ACSFlags.ACS_PWRH) then
        stat = "high"
    end

    if stat and stat ~= ACSInfo.power then
        log(string.format("Changed ACS power to %s\n", stat))
        updateRobotStatus("ACS", "power", stat)
        ACSInfo.power = stat
    end
end

local function maybeReceivePulse()
    local chance = 0.05 -- UNDONE
    -- Simulate that only a fraction of send pulses are bounced back
    -- towards the detector. Receiving pulses happens immidiately after they
    -- are sent. Note that in real life pulses travel at the speed of light,
    -- which is close too instant as well :)
    if math.random() <= chance then
        -- Execute ISR used by RP6 lib to detect IR pulses
        avr.execISR(avr.ISR_INT2_vect)
    end
end


function handleIOData(type, data)
    if type == avr.IO_DDRD or type == avr.IO_PORTD or
       type == avr.IO_DDRB or type == avr.IO_PORTB then
        updateACSPower(type, data)
    end

    if type == avr.IO_PORTB then
        local e = not bit.isSet(data, ACSFlags.ACS_L)
        if e ~= ACSInfo.leftEnabled then
            ACSInfo.leftEnabled = e
            log(string.format("Left channel %s\n", (e and "enabled") or "disabled"))
            enableLED("acsl", e)
            if not e then
                maybeReceivePulse()
            end
        end
    elseif type == avr.IO_PORTC then
        local e = not bit.isSet(data, ACSFlags.ACS_R)
        if e ~= ACSInfo.rightEnabled then
            ACSInfo.rightEnabled = e
            log(string.format("Right channel %s\n", (e and "enabled") or "disabled"))
            enableLED("acsr", e)
            if not e then
                maybeReceivePulse()
            end
        end
    end
end


return ret
