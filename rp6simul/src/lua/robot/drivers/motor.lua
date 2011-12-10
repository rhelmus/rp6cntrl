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
    avr.IO_PORTB,
    avr.IO_PORTC
}

local motorInfo = {
    enabled = true,
    prescaler = 0,
    compareValueA = 0,
    compareValueB = 0,
    inputRegister = 0,
    leftPower = 0,
    rightPower = 0,
    leftDirection = "FWD",
    rightDirection = "FWD",
    leftEncDriveCounter = 0,
    rightEncDriveCounter = 0,
    leftTotalEncDriveCounter = 0,
    rightTotalEncDriveCounter = 0,
    leftEncMoveCounter = 0,
    rightEncMoveCounter = 0,
    leftReadOutError = 1,
    rightReadOutError = 1,
    readOutCounter = nil,
    leftDistance = 0,
    rightDistance = 0,
}

local leftEncTimer, rightEncTimer, encReadoutTimer
local powerON = false

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
        local freq = 1000 / getEffectiveSpeedTimerBase()
        rightEncTimer:setCompareValue(properties.clockSpeed / (data * freq))
        if not rightEncTimer:isEnabled() then
            clock.enableTimer(rightEncTimer, true)
        end
        if not encReadoutTimer:isEnabled() then
            clock.enableTimer(encReadoutTimer, true)
        end
    end
    motorInfo.rightPower = data
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
        local freq = 1000 / getEffectiveSpeedTimerBase()
        leftEncTimer:setCompareValue(properties.clockSpeed / (data * freq))
        if not leftEncTimer:isEnabled() then
            clock.enableTimer(leftEncTimer, true)
        end
        if not encReadoutTimer:isEnabled() then
            clock.enableTimer(encReadoutTimer, true)
        end
    end
    motorInfo.leftPower = data
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
    motorInfo.leftDirection = (bit.isSet(data, avr.PINC2) and "BWD") or "FWD"
    motorInfo.rightDirection = (bit.isSet(data, avr.PINC3) and "BWD") or "FWD"
    updateRobotStatus("motor", "direction", "left", motorInfo.leftDirection)
    updateRobotStatus("motor", "direction", "right", motorInfo.rightDirection)
    setMotorDir(motorInfo.leftDirection, motorInfo.rightDirection)
end

local function getEncoderFactor()
    local factors =
    {
        { delta = 120, factor = 0.985 },
        { delta = 140, factor = 0.97 },
        { delta = 160, factor = 0.94 },
        { delta = 200, factor = 0.92 },
        { delta = 220, factor = 0.9 },
        { delta = 260, factor = 0.9 },
        { delta = 300, factor = 0.85 },
        { delta = 320, factor = 0.82 },
        { delta = 340, factor = 0.79 },
        { delta = 360, factor = 0.75 },
        { delta = 380, factor = 0.807 },
        { delta = 400, factor = 0.78 },
    }

    local lp, rp = motorInfo.leftPower, motorInfo.rightPower
    if motorInfo.leftDirection == "BWD" then
        lp = -lp
    end
    if motorInfo.rightDirection == "BWD" then
        rp = -rp
    end
    local delta = math.abs(lp - rp)

    for i, entry in ipairs(factors) do
        if delta <= entry.delta then
            return entry.factor
        elseif i == #factors then
            return entry.factor
        elseif delta < factors[i+1].delta then
            local dy = factors[i+1].factor - entry.factor
            local dx = factors[i+1].delta - entry.delta
            local slope = dy / dx
            log("delta:", delta, slope, entry.delta, factors[i+1].delta, entry.factor + ((delta - entry.delta) * slope), "\n")
            return entry.factor + ((delta - entry.delta) * slope)
        end
    end
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
        if math.random() <= getEncoderFactor() then
            motorInfo.leftEncDriveCounter = motorInfo.leftEncDriveCounter + 1
        end
        if not robotIsBlocked() and powerON then
            avr.execISR(avr.ISR_INT0_vect)
            motorInfo.leftEncMoveCounter = motorInfo.leftEncMoveCounter + 1
        end
    end)

    rightEncTimer = clock.createTimer()
    rightEncTimer:setTimeOut(function()
        if math.random() <= getEncoderFactor() then
            motorInfo.rightEncDriveCounter = motorInfo.rightEncDriveCounter + 1
        end
        if not robotIsBlocked() and powerON then
            avr.execISR(avr.ISR_INT1_vect)
            motorInfo.rightEncMoveCounter = motorInfo.rightEncMoveCounter + 1
        end
    end)

    encReadoutTimer = clock.createTimer()
    encReadoutTimer:setTimeOut(function()
        -- Update error every ~1 sec
        if motorInfo.readOutCounter == nil or motorInfo.readOutCounter >= 3 then
            motorInfo.readOutCounter = 0
--[[
            -- UNDONE: 3
            local avgls = motorInfo.leftTotalEncDriveCounter / 3
            local avgrs = motorInfo.rightTotalEncDriveCounter / 3
            motorInfo.leftTotalEncDriveCounter = 0
            motorInfo.rightTotalEncDriveCounter = 0

            if motorInfo.leftDirection == "BWD" then
                avgls = -avgls
            end
            if motorInfo.rightDirection == "BWD" then
                avgrs = -avgrs
            end

            local delta = math.abs(avgls - avgrs)
            local minnoise = -0.0009253131*delta+0.0610912343
            log("delta/minnoise:", delta, minnoise, "\n")
            local maxnoise = 1337 * (1 - (delta / 500))

            if true or minnoise == maxnoise then
                motorInfo.leftReadOutError, motorInfo.rightReadOutError = 1, 1
            else
--                minnoise, maxnoise = -0.18, -0.24
                motorInfo.leftReadOutError = 1 * 0.7 --+ minnoise -- (math.random(minnoise*100, maxnoise*100) / 100)
                motorInfo.rightReadOutError = 1 * 0.7 --+ minnoise -- (math.random(minnoise*100, maxnoise*100) / 100)
            end
            --]]
            motorInfo.leftReadOutError = 1 --+ (math.random(-5, 5) / 100)
            motorInfo.rightReadOutError = 1-- + (math.random(-5, 5) / 100)
        end

        motorInfo.readOutCounter = motorInfo.readOutCounter + 1

        local drivespeed = motorInfo.leftEncDriveCounter
        drivespeed = drivespeed * motorInfo.leftReadOutError
        motorInfo.leftTotalEncDriveCounter = motorInfo.leftTotalEncDriveCounter + motorInfo.leftEncDriveCounter
        local movespeed = motorInfo.leftEncMoveCounter
        motorInfo.leftDistance = motorInfo.leftDistance + movespeed
        motorInfo.leftEncDriveCounter = 0
        motorInfo.leftEncMoveCounter = 0
        updateRobotStatus("motor", "speed", "left", movespeed)
        setMotorDriveSpeed("left", drivespeed)
        setMotorMoveSpeed("left", movespeed)
        updateRobotStatus("motor", "distance", "left", motorInfo.leftDistance)

        drivespeed = motorInfo.rightEncDriveCounter
        drivespeed = drivespeed * motorInfo.rightReadOutError
        motorInfo.rightTotalEncDriveCounter = motorInfo.rightTotalEncDriveCounter + motorInfo.rightEncDriveCounter
        movespeed = motorInfo.rightEncMoveCounter
        motorInfo.rightDistance = motorInfo.rightDistance + movespeed
        motorInfo.rightEncDriveCounter = 0
        motorInfo.rightEncMoveCounter = 0
        updateRobotStatus("motor", "speed", "right", movespeed)
        setMotorDriveSpeed("right", drivespeed)
        setMotorMoveSpeed("right", movespeed)
        updateRobotStatus("motor", "distance", "right", motorInfo.rightDistance)
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
            warning("Disabling motor control: timer1 enabled")
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
        elseif type == avr.IO_PORTB then
            powerON = bit.isSet(data, avr.PB4)
        elseif type == avr.IO_PORTC then
            setMotorDirection(data)
        end
    end
end

function getADCValue(a)
    -- RP6 library assumes that current ADC values between 150 and 770 are OK
    -- For safety we take a range above/below this
    local mincurrent, maxcurrent = 200, 700
    local currentd = maxcurrent - mincurrent
    local maxpower = 210

    if a == "MCURRENT_L" then
        if motorInfo.leftPower == 0 or not powerON then
            return 0
        end
        return mincurrent + ((motorInfo.leftPower / maxpower) * currentd)
    elseif a == "MCURRENT_R" then
        if motorInfo.rightPower == 0 or not powerON then
            return 0
        end
        return mincurrent + ((motorInfo.rightPower / maxpower) * currentd)
    end

    -- return nil
end

function closePlugin()
    leftEncTimer, rightEncTimer, encReadoutTimer = nil, nil, nil
end

return ret
