-- Generate access functions
for k, v in pairs(sensortable) do
    _G[string.format("get%s", k)] = function()
        return sensortable[k]
    end
end

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

function stop()
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

-- Misc functions

local oldprint = print
function print(s, ...)
    oldprint(s, ...)

    if ... then
        sendtext(s .. "\t" .. table.concat({...}, "\t") .. "\n")
    else
        sendtext(s .. "\n")
    end
end

-- External script functions
local curscript = nil

local function calloptscriptfunc(f, ...)
    -- Use rawget to avoid obtaining data from global environment
    local func = rawget(curscript, f)
    if func then
        func(...)
    end
end

function runscript(s)
    local stat, ret = pcall(assert(loadstring(s)))

    if stat then
        curscript = ret
    else
        print("Failed to run script:", ret)
        return
    end
    
    curscript.corun = coroutine.create(curscript.run)
    
    calloptscriptfunc("init")

    scriptrunning(true)
end

function think()
    if curscript then
        assert(coroutine.resume(curscript.corun))
        if coroutine.status(curscript.corun) == "dead" then
            calloptscriptfunc("finish")
            curscript = nil
            scriptrunning(false)
        end
    end 
end

function execcmd(cmd, ...)
    print("Executing script cmd:", cmd)
    
    if cmd == "abortcurrentscript" then
        if curscript then
            calloptscriptfunc("finish")
            curscript = nil
            scriptrunning(false)
        end
    elseif curscript then
        calloptscriptfunc("handlecmd", cmd, ...)
    end
end

function initclient()
    print("initclient:", tostring(curscript ~= nil))
    scriptrunning(curscript ~= nil)
    if curscript then
        calloptscriptfunc("initclient")
    end
end

function makescript()
    local ret = { }
    setmetatable(ret, { __index = _G })
    setfenv(2, ret)
    return ret
end
