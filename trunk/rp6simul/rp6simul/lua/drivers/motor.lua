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
    leftEncCounter = 0,
    rightEncCounter = 0,
}

local leftEncTimer, rightEncTimer, encReadoutTimer

local function getSpeedTimerBase()
    -- This function should return the same value, defined as
    -- 'SPEED_TIMER_BASE' in the plugin (defined in RP6Config.h shipped
    -- with the RP6 library)
    -- UNDONE: Get this from plugin/setting?
    return 200
end

local function getEffectiveSpeedTimerBase()
    --[[
        There is a small bug/glitch in the shipped RP6 library by Arexx
        wrt. to obtaining the motor speed. Although it seems that the
        intention was to obtain the speed every 'SPEED_TIMER_BASE'
        (default 200) msec, this infact happens at a slight (but significant)
        lower frequency. The library code keeps for each motor a counter that
        is incremented after an encoder ISR is executed. By reading and
        resetting these counters after 'SPEED_TIMER_BASE' msec passed, the
        motor speeds are obtained. For this a timer with approx. 100 us
        resolution is used. Within this timer two counters are used, the first
        is incremented each 100 us, the second every ms. It is this code that
        is wrong: the first counter is used to see if one ms has passed, this
        can be done if it has incremented ten times (ie. 10*100 us =  1ms),
        however due the wrong usage of the post increment operator it is infact
        tested if it is incremented eleven times (1.1 ms).
        Similarly the second counter is used to see if 'SPEED_TIMER_BASE' ms
        have elapsed. Besides the same problem with post increment, the if
        test also uses '>' instead of '>=', thus the check is in reality
        if 'SPEED_TIMER_BASE' + 2 ms have passed. Furthermore as the second
        counter depends on the first counter for increments,
        the deviations become significant.

        Normal AVR programs are not really affected by this subtle bug,
        however for accurate motor speed determination we need to correct
        for this, hence this function.
    --]]

    return 1.1 * (getSpeedTimerBase() + 2)
end

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

    if cond then
        local ps = getPrescaler(data)
        if ps and ps ~= motorInfo.prescaler then
            motorInfo.prescaler = ps
            log(string.format("Changed timer1 PWM prescaler to %d\n", ps))
            -- UNDONE: Need this?
--            leftEncTimer:setPrescaler(ps)
--            rightEncTimer:setPrescaler(ps)
        end

        cond = (ps ~= nil)
        if not cond then
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
        --[[
            In the RP6 library motor code, the motor speeds are determined
            every 'SPEED_TIMER_BASE' ms (see comments in
            getEffectiveSpeedTimerBase()). To simulate the encoders, timers are
            used for each motor. These call the ISR that normally is executed by
            the encoders. Although not strictly necessary, the compare value (CV)
            from these timers are set in such a way that motor power and motor
            speed are roughly the same. For this the following formula is used:
            CV = CPU_MHz / (power * effective_encoder_read_frequency)
        --]]
        -- UNDONE: cpu speed configurable
        local freq = 1000 / getEffectiveSpeedTimerBase()
        rightEncTimer:setCompareValue(8000000 / (data * freq))
        if not rightEncTimer:isEnabled() then
            clock.enableTimer(rightEncTimer, true)
        end
        if not encReadoutTimer:isEnabled() then
            clock.enableTimer(encReadoutTimer, true)
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
        -- UNDONE: cpu speed configurable
        local freq = 1000 / getEffectiveSpeedTimerBase()
        leftEncTimer:setCompareValue(8000000 / (data * freq))
        if not leftEncTimer:isEnabled() then
            clock.enableTimer(leftEncTimer, true)
        end
        if not encReadoutTimer:isEnabled() then
            clock.enableTimer(encReadoutTimer, true)
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
    updateRobotStatus("motor", "direction", "left", ldir)
    updateRobotStatus("motor", "direction", "right", rdir)
    setMotorDir(ldir, rdir)
end


function initPlugin()
    --[[
        For simulation of each encoder a timer is used that executes the
        encoder ISR at such intervals that motor power and speed are roughly
        equal (see comments in setCompareRegisterA()). A third timer is used
        to determine the simulated motor speed. In a similar way as the RP6
        library code, at every 'SPEED_TIMER_BASE' ms the speeds are determined
        from counters that are incremented by the encoders (see comments in
        getEffectiveSpeedTimerBase()). In the library code timer0 is used,
        which has a compare value of 99 and a prescaler of 8, resulting in
        a resolution of approx 100 us. To simulate readings every 200 ms, we
        therefore could use a timer with a compare value of:
        CV = 8 * 99 * 10 * SPEED_TIMER_BASE
        However due some subtle bugs (see getEffectiveSpeedTimerBase()), a small
        correction is needed:
        CV = 8 * 99 * 11 * (SPEED_TIMER_BASE+2)
    --]]

    leftEncTimer = clock.createTimer()
    leftEncTimer:setTimeOut(function()
        avr.execISR(avr.ISR_INT0_vect)
        motorInfo.leftEncCounter = motorInfo.leftEncCounter + 1
    end)

    rightEncTimer = clock.createTimer()
    rightEncTimer:setTimeOut(function()
        avr.execISR(avr.ISR_INT1_vect)
        motorInfo.rightEncCounter = motorInfo.rightEncCounter + 1
    end)

    encReadoutTimer = clock.createTimer()
    encReadoutTimer:setTimeOut(function()
        motorInfo.leftSpeed = motorInfo.leftEncCounter
        motorInfo.leftEncCounter = 0
        updateRobotStatus("motor", "speed", "left", motorInfo.leftSpeed)
        setMotorSpeed("left", motorInfo.leftSpeed)

        motorInfo.rightSpeed = motorInfo.rightEncCounter
        motorInfo.rightEncCounter = 0
        updateRobotStatus("motor", "speed", "right", motorInfo.rightSpeed)
        setMotorSpeed("right", motorInfo.rightSpeed)
    end)

    -- See top
    encReadoutTimer:setCompareValue(8 * 99 * 11 * (getSpeedTimerBase()+2))
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
