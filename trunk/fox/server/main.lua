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
end

function rotate(angle, speed, dir)
    local cmd = "rotate " .. tostring(angle)
    if speed then
        cmd = cmd .. " " .. tostring(speed)
        if dir then
            cmd = cmd .. " " .. dir
        end
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


-- Misc functions

local oldprint = print
function print(s, ...)
    oldprint(s, ...)

    if ... then
        sendtext(s .. table.concat({...}, "\t"))
    else
        sendtext(s)
    end
end

local curscript = nil
function runscript(s)
    curscript = assert(loadstring(s))()
    
    curscript.corun = coroutine.create(curscript.run)
    
    if curscript.init then
        curscript.init()
    end
end

function think()
    if curscript then
        assert(coroutine.resume(curscript.corun))
        if coroutine.status(curscript.corun) == "dead" then
            if curscript.finish then
                curscript.finish()
            end
            curscript = nil
        end
    end 
end

function execcmd(cmd, ...)
    print("Executing script cmd:", cmd)
    if curscript and curscript.handlecmd then
        curscript.handlecmd(cmd, ...)
    end
end

function makescript()
    local ret = { }
    setmetatable(ret, { __index = _G })
    setfenv(0, ret)
    return ret
end
