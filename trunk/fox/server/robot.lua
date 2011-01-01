extmodule(...)

-- Util functions to control robot

function move(dist, speed, dir)
    local cmd = "move " .. tostring(dist)
    if speed then
        cmd = cmd .. " " .. tostring(speed)
        if dir then
            cmd = cmd .. " " .. dir
        end
    end

    exec(cmd)
end

function rotate(angle, speed, dir)
    local cmd = "rotate " .. tostring(angle)
    if speed then
        cmd = cmd .. " " .. tostring(speed)
        if dir then
            cmd = cmd .. " " .. dir
        end
    end

    exec(cmd)
end

function shortrotate(angle, speed)
    speed = speed or 80
    if angle <= 180 then
        rotate(angle, speed, "right")
    else
        rotate(360 - angle, speed, "left")
    end
end

function driverotatetime(angle, speed)
    -- Calibrated for 180 degrees with speed 70, 80, 90 and 100
    return (angle / 180) * (16549.25 - 99.19166667 * speed)
end

function motor.setspeed(left, right)
    exec(string.format("set speed %d %d", left, right))
end

function motor.setdir(dir)
    exec("set dir " .. dir)
end

function motor.stop()
    exec("stop")
end

function setbaseleds(s)
    exec(string.format("set leds1 %s", s))
end

function setm32leds(s)
    exec(string.format("set leds2 %s", s))
end

function setacs(s)
    exec(string.format("set acs %s", s))
end

function setservo(angle)
    exec(string.format("set servo %d", angle))
end

function beep(pitch, time)
    exec(string.format("beep %d %d", pitch, time))
end

function sound(pitch, time, delay)
    exec(string.format("sound %d %d %d", pitch, time, delay))
end

function servoangle(pos)
    -- Servo ranges from 0 (270 deg) to 180 (90 deg)
    -- Angles are converted clockwise
    return math.wrapangle(270 + pos)
end

function sensoroffset()
    return nav.newvector(0, 3)
end

function servospeed()
    return 0.180 -- degrees/millisecond
end
