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
        checkNoiseDelay = 0,
    },

    right = {
        type = "right",
        enabled = false,
        IRSensor = nil,
        noPulseTime = 0,
        checkNoiseDelay = 0,
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

local function maybeReceivePulse(sensor)
    -- Simulate that only a fraction of send pulses are bounced back
    -- towards the detector. Receiving pulses happens immidiately after they
    -- are sent. Note that in real life pulses travel at the speed of light,
    -- which is close too instant as well :)

    local curtimems = clock.getTimeMS()

    if curtimems < sensor.noPulseTime then
        return
    end

    local chance

    -- Fraction of pulses that are minimally necessary to trigger
    -- collision detection
    if sensor.type == "left" then
        chance = getACSProperty("recLeft") / getACSProperty("sendLeft")
    else
        chance = getACSProperty("recRight") / getACSProperty("sendRight")
    end

    -- When looking for collisions, the RP6 lib code performs 4 'bursts'
    -- and collects the _total_ amount of signals back. Thus, unlike the config
    -- header suggests, the threshold is actually for all the 4 bursts
    -- However, to make the chance high enough for 100% hits, we increase it
    -- a little bit further.
    chance = chance / 4 * 2

    if powerON and ACSInfo.power ~= "off" then
        local dist = sensor.IRSensor:getHitDistance()

        local maxrange = properties.ACSLeft.distance[ACSInfo.power]
        if dist <= maxrange then
            log(string.format("ACS dist (%s): %d\n", sensor.type, dist * properties.cmPerPixel))

            -- Add noise when near max
            local frdist = dist / maxrange
            if frdist >= 0.9 and sensor.checkNoiseDelay < curtimems then
                local delay = 36 -- UNDONE
                if math.random() < (frdist / 3) then
                    sensor.noPulseTime = curtimems + delay
                    chance = 0
                else
                    sensor.checkNoiseDelay = curtimems + delay
                end
            end

            if math.random() <= chance then
                -- Execute ISR used by RP6 lib to detect IR pulses
                avr.execISR(avr.ISR_INT2_vect)
            end
        end
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
