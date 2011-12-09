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

    left = {
        type = "left",
        enabled = false,
        IRSensor = nil,
        noPulseTime = 0,
        pulseFraction = nil,
        pulsesSend = 0,
        pulsesReceived = 0,
    },

    right = {
        type = "right",
        enabled = false,
        IRSensor = nil,
        noPulseTime = 0,
        pulseFraction = nil,
        pulsesSend = 0,
        pulsesReceived = 0,
    },
}

local powerON = false


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

function getPulseFraction(sensor, dist, range)
    local ret

    -- Fraction of pulses that are minimally necessary to trigger
    -- collision detection
    if sensor.type == "left" then
        ret = getACSProperty("recLeft") / getACSProperty("sendLeft")
    else
        ret = getACSProperty("recRight") / getACSProperty("sendRight")
    end

    -- When looking for collisions, the RP6 lib code performs 4 'bursts'
    -- and collects the _total_ amount of signals back. Thus, unlike the config
    -- header suggests, the threshold is actually for all the 4 bursts
    -- However, to make the chance high enough for 100% hits, we increase it
    -- a little bit further.
    ret = ret / 4 * 2

    if (dist / range) > 0.9 then
        -- Randomize to simulate scatter when near maximum range
        ret = (math.random(-300 * ret, 300 * ret) / 100)
    end

    return ret
end

local function maybeReceivePulse(sensor)
    -- Simulate that only a fraction of send pulses are bounced back
    -- towards the detector. Receiving pulses happens immidiately after they
    -- are sent. Note that in real life pulses travel at the speed of light,
    -- which is close too instant as well :)

    if not powerON or ACSInfo.power == "off" then
        return
    end

    local curtimems = clock.getTimeMS()
    local dist = sensor.IRSensor:getHitDistance()
    local maxrange = properties.ACSLeft.distance[ACSInfo.power]

    if dist > maxrange then
        return
    end

    if sensor.pulseFraction == nil then
        sensor.pulseFraction = getPulseFraction(sensor, dist, maxrange)
    end

    sensor.pulsesSend = sensor.pulsesSend + 1

    -- Recalculate fraction every 100 pulses
    if sensor.pulsesSend >= 100 then
        log("Received/Send ratio:", sensor.pulsesReceived / sensor.pulsesSend, "\n")
        sensor.pulsesSend, sensor.pulsesReceived = 0, 0
        sensor.pulseFraction = getPulseFraction(sensor, dist, maxrange)
        log("pulseFraction:", sensor.pulseFraction, "\n")
    elseif sensor.pulsesReceived < (sensor.pulseFraction * 100) and
           sensor.noPulseTime <= curtimems then
        avr.execISR(avr.ISR_INT2_vect)
        sensor.pulsesReceived = sensor.pulsesReceived + 1
        sensor.noPulseTime = curtimems + 1
    end
end


function initPlugin()
    ACSInfo.left.IRSensor = createIRSensor(properties.ACSLeft.pos,
                                           properties.ACSLeft.color,
                                           properties.ACSLeft.radius,
                                           properties.ACSLeft.angle,
                                           properties.ACSLeft.distance.high)
    ACSInfo.right.IRSensor = createIRSensor(properties.ACSRight.pos,
                                            properties.ACSRight.color,
                                            properties.ACSRight.radius,
                                            properties.ACSRight.angle,
                                            properties.ACSRight.distance.high)
end

function handleIOData(type, data)
    if type == avr.IO_DDRD or type == avr.IO_PORTD or
       type == avr.IO_DDRB or type == avr.IO_PORTB then
        updateACSPower(type, data)
    end

    if type == avr.IO_PORTB then
        powerON = bit.isSet(data, avr.PB4)

        local e = not bit.isSet(data, ACSFlags.ACS_L)
        if e ~= ACSInfo.left.enabled then
            ACSInfo.left.enabled = e
--            log(string.format("Left channel %s\n", (e and "enabled") or "disabled"))
            if not e then
                maybeReceivePulse(ACSInfo.left)
            end
        end
    elseif type == avr.IO_PORTC then
        local e = not bit.isSet(data, ACSFlags.ACS_R)
        if e ~= ACSInfo.right.enabled then
            ACSInfo.right.enabled = e
--            log(string.format("Right channel %s\n", (e and "enabled") or "disabled"))
            if not e then
                maybeReceivePulse(ACSInfo.right)
            end
        end
    end
end


return ret
